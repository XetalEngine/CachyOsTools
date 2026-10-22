"""Compile the real Qt generator, then parse and stage every emitted script variant."""
import os
from pathlib import Path
import shlex
import shutil
import stat
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]

class GeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not all(shutil.which(x) for x in ('g++','pkg-config','rcc')):
            raise unittest.SkipTest('Qt development tools are needed for generator tests')
        query = subprocess.run(['pkg-config','--cflags','--libs','Qt6Widgets'],capture_output=True,text=True)
        if query.returncode: raise unittest.SkipTest('Qt6Widgets development files are unavailable')
        cls.temp = tempfile.TemporaryDirectory(prefix='iso-generator-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.root = Path(cls.temp.name)
        header = (REPO/'current_system_iso_creator.h').read_text()
        function = 'QString MainWindow::createIsoScript(' + header.split('QString MainWindow::createIsoScript(',1)[1].split('// Helper function to format size',1)[0]
        prelude = '''#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTextStream>
#include "script_helpers.h"
struct IsoFirstBootOptions {
 bool fixNetwork=false,fixGpu=false,changeUser=false,regenSsh=false,regenMachineId=false;
 bool any() const { return fixNetwork||fixGpu||changeUser||regenSsh||regenMachineId; }
};
class MainWindow: public QWidget {
public:
 QString offlinePackagePath;
 QString createIsoScript(const QString&,const QString&,bool,const QStringList&,const IsoFirstBootOptions&,bool);
};
'''
        main = '''
int main(int argc,char **argv) {
 QApplication app(argc,argv);
 MainWindow window;
 window.offlinePackagePath=QString::fromUtf8(argv[3]);
 IsoFirstBootOptions options;
 options.fixNetwork=options.fixGpu=options.changeUser=options.regenSsh=options.regenMachineId=QString(argv[2])=="1";
 QString path=window.createIsoScript("test-clone",QDir::currentPath(),QString(argv[1])=="1",{QString::fromUtf8(argv[3])},options,QString(argv[4])=="1");
 if(path.isEmpty()) return 1;
 QTextStream(stdout)<<path<<"\\n";
}
'''
        source=cls.root/'generator.cpp'
        source.write_text(prelude+function+main)
        qrc=cls.root/'resources.cpp'
        subprocess.run(['rcc',str(REPO/'resources.qrc'),'-o',str(qrc)],check=True,capture_output=True,text=True)
        cls.binary=cls.root/'generator'
        result=subprocess.run(['g++','-std=c++17','-fPIC','-I',str(REPO),str(source),str(qrc),'-o',str(cls.binary)]+shlex.split(query.stdout),capture_output=True,text=True,timeout=90)
        if result.returncode: raise AssertionError(result.stderr)

    def verify_variant(self, offline, compat, dummy=0):
        with tempfile.TemporaryDirectory(prefix='iso-stage-') as tmp:
            root=Path(tmp)
            injected=root/'must-not-be-created'
            tricky=str(root/('archive $(touch '+str(injected)+") ' `id` [1]*?.tar.gz"))
            env=dict(os.environ,QT_QPA_PLATFORM='offscreen',TMPDIR=tmp)
            result=subprocess.run([str(self.binary),str(offline),str(compat),tricky,str(dummy)],cwd=tmp,env=env,capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)
            script=Path(result.stdout.strip())
            self.assertEqual(stat.S_IMODE(script.stat().st_mode),0o700)
            content=script.read_text()
            self.assertNotIn('SUDO_PASS=',content)
            self.assertIn('IFS= read -r -s SUDO_PASS',content)
            self.assertEqual(subprocess.run(['bash','-n',str(script)],capture_output=True).returncode,0)
            self.assertIn('iso_prepare_packages',content)
            self.assertIn(f'ISO_DUMMY={dummy}\n', content)
            # Stage payloads only. Never call the builder, sudo, rsync, or mkarchiso.
            content=content.split('# Run the selected ISO build.',1)[0]
            profile=root/'profile'
            content+='\nPROFILE='+shlex.quote(str(profile))+'\nstage_iso_payload\n'
            if dummy:
                content += 'iso_dummy_snapshot\niso_dummy_launcher\n'
            # Like production, execute a file: embedded scripts plus the logo
            # can exceed Linux's per-argument limit for bash -c.
            staging_script=root/'stage-payload.sh'
            staging_script.write_text(content)
            staged=subprocess.run(['bash',str(staging_script)],cwd=tmp,env=env,capture_output=True,text=True,timeout=10)
            self.assertEqual(staged.returncode,0,staged.stderr)
            self.assertFalse(injected.exists())
            for name in ('common.sh','installer-ui.py','restore-boot.sh','firstboot.sh','firstboot.service'):
                path=profile/'airootfs/opt/clone'/name
                self.assertEqual(path.read_text().strip(),(REPO/'iso'/name).read_text().strip())
            # Dollar signs, backslashes and whitespace must survive shell staging.
            self.assertEqual((profile/'airootfs/opt/clone/installer-logo.txt').read_bytes(),
                             (REPO/'iso/installer-logo.txt').read_bytes())
            self.assertEqual((profile/'airootfs/usr/local/bin/installer.sh').read_text().strip(),(REPO/'iso/installer.sh').read_text().strip())
            self.assertEqual((profile/'airootfs/opt/clone/firstboot.conf').exists(),bool(compat and not dummy))
            if dummy:
                clone = profile/'airootfs/opt/clone'
                self.assertTrue((clone/'dummy-iso').exists())
                self.assertIn('DUMMY=1\n', (clone/'snapshot.meta').read_text())
                listing = subprocess.run(['tar', '-I', 'zstd', '-tf', str(clone/'rootfs-snapshot.tar.zst')],
                                         capture_output=True, text=True, timeout=10)
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(listing.stdout, '')
                checksum = subprocess.run(['sha256sum', '-c', 'snapshot.sha256'], cwd=clone,
                                          capture_output=True, text=True, timeout=10)
                self.assertEqual(checksum.returncode, 0, checksum.stderr)
                self.assertIn('/run/archiso/bootmnt/xetal-preview.tar', (profile/'airootfs/xetal.sh').read_text())

    def test_online_default(self): self.verify_variant(0,0)
    def test_online_adaptation(self): self.verify_variant(0,1)
    def test_offline_default(self): self.verify_variant(1,0)
    def test_offline_adaptation(self): self.verify_variant(1,1)
    def test_dummy_has_empty_snapshot_and_no_firstboot_changes(self): self.verify_variant(0,1,1)
    def test_dummy_can_prepare_an_offline_live_environment(self): self.verify_variant(1,0,1)

if __name__=='__main__': unittest.main()
