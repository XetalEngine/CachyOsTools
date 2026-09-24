"""Capability, firmware, disk protection, archive and generated-script checks."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
COMMON = REPO / 'iso/common.sh'

class CompatibilityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso-compat-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def file(self, name, content='fixture', executable=False):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        if executable: path.chmod(0o755)
        return path

    def shell(self, code, *args):
        return subprocess.run(['bash','-c','set -euo pipefail\nsource "$1"\nshift\n'+code,
                               'test',str(COMMON),*map(str,args)], capture_output=True, text=True, timeout=15)

    def kernel(self, version, pkgbase, boot=False):
        self.file(f'usr/lib/modules/{version}/pkgbase', pkgbase+'\n')
        self.file(f'usr/lib/modules/{version}/modules.dep')
        self.file(f'boot/vmlinuz-{pkgbase}' if boot else f'usr/lib/modules/{version}/vmlinuz')

    def test_kernel_discovery_for_arch_cachyos_and_manjaro_names(self):
        for version, base in [('6.12-arch1','linux'),('6.12-cachyos','linux-cachyos'),('6.12-MANJARO','linux612')]:
            self.kernel(version, base, boot='MANJARO' in version)
        result = self.shell('iso_kernels "$1"', self.root)
        self.assertEqual(result.returncode,0,result.stderr)
        for base in ('linux','linux-cachyos','linux612'): self.assertIn('\t'+base+'\t',result.stdout)

    def test_incomplete_kernel_is_rejected(self):
        self.file('usr/lib/modules/broken/pkgbase','linux\n')
        self.assertNotEqual(self.shell('iso_kernels "$1"',self.root).returncode,0)

    def test_manjaro_versioned_boot_filename_without_pkgbase(self):
        self.file('usr/lib/modules/6.12.4-1-MANJARO/modules.dep')
        self.file('boot/vmlinuz-6.12-x86_64')
        result=self.shell('file() { echo "Linux kernel x86 boot executable bzImage, version 6.12.4-1-MANJARO (build)"; }\niso_kernels "$1"',self.root)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('/boot/vmlinuz-6.12-x86_64',result.stdout)

    def test_kernel_with_different_embedded_version_is_rejected(self):
        self.file('usr/lib/modules/6.12.4-1-MANJARO/modules.dep')
        self.file('boot/vmlinuz-6.12-x86_64')
        result=self.shell('file() { echo "Linux kernel x86 boot executable bzImage, version 6.12.40-1-MANJARO (build)"; }\niso_kernels "$1"',self.root)
        self.assertNotEqual(result.returncode,0)

    def test_non_x86_64_is_rejected(self):
        self.assertNotEqual(self.shell('iso_host_supported "$1" aarch64',self.root).returncode,0)

    def test_pacman_systemd_hosts_are_accepted_by_capability(self):
        (self.root/'var/lib/pacman/local').mkdir(parents=True)
        self.file('usr/lib/systemd/systemd')
        self.assertEqual(self.shell('iso_host_supported "$1" x86_64',self.root).returncode,0)

    def test_mkinitcpio_and_dracut_fallback(self):
        self.file('usr/bin/dracut',executable=True)
        self.assertEqual(self.shell('iso_initramfs_tool "$1"',self.root).stdout.strip(),'dracut')
        self.file('usr/bin/mkinitcpio',executable=True)
        self.assertEqual(self.shell('iso_initramfs_tool "$1"',self.root).stdout.strip(),'mkinitcpio')

    def test_missing_initramfs_tool_is_rejected(self):
        self.assertNotEqual(self.shell('iso_initramfs_tool "$1"',self.root).returncode,0)

    def test_grub_supports_only_installed_firmware_modules(self):
        self.file('usr/bin/grub-install',executable=True)
        (self.root/'usr/lib/grub/i386-pc').mkdir(parents=True)
        self.assertEqual(self.shell('iso_bootloader "$1" i386-pc',self.root).stdout.strip(),'grub')
        self.assertNotEqual(self.shell('iso_bootloader "$1" x86_64-efi',self.root).returncode,0)

    def test_uefi_falls_back_to_systemd_boot_for_grubless_sources(self):
        self.file('usr/bin/bootctl',executable=True)
        self.file('usr/lib/systemd/boot/efi/systemd-bootx64.efi')
        self.assertEqual(self.shell('iso_bootloader "$1" x86_64-efi',self.root).stdout.strip(),'systemd-boot')
        self.assertNotEqual(self.shell('iso_bootloader "$1" i386-pc',self.root).returncode,0)

    def test_partition_names_for_sata_nvme_mmc_and_loop(self):
        for disk, expected in [('/dev/sda','/dev/sda3'),('/dev/nvme0n1','/dev/nvme0n1p3'),('/dev/mmcblk0','/dev/mmcblk0p3'),('/dev/loop0','/dev/loop0p3')]:
            self.assertEqual(self.shell('iso_partition_path "$1" 3',disk).stdout.strip(),expected)

    def test_host_mirrors_and_custom_repositories_are_not_used(self):
        path=self.root/'pacman.conf'
        result=self.shell('iso_write_pacman_conf "$1" "$2/cache" "$2/keys" \'https://geo.mirror.pkgbuild.com/$repo/os/$arch\'',path,self.root)
        self.assertEqual(result.returncode,0,result.stderr)
        config=path.read_text()
        self.assertNotIn('Include',config)
        self.assertNotIn('manjaro',config)
        self.assertNotIn('cachyos',config)
        self.assertIn('mirrors.kernel.org',config)
        self.assertIn('SigLevel = Required',config)

    def disk_check(self, protected='', mount='', child='part', readonly='0', disk_type='disk', consumers_fail='0', swaps_fail='0'):
        mock='''
iso_is_block() { return 0; }
lsblk() {
    case "$*" in
        '-dnro TYPE /dev/test') printf '%s\\n' "$DISK_TYPE" ;;
        '-dnro RO /dev/test') printf '%s\\n' "$READONLY" ;;
        '-nrpo MOUNTPOINTS /dev/test') printf '%s' "$MOUNT" ;;
        '-nrpo NAME,TYPE /dev/test') [[ $CONSUMERS_FAIL == 0 ]] || return 1; printf '/dev/test disk\\n/dev/test1 %s\\n' "$CHILD" ;;
        *) return 1 ;;
    esac
}
swapon() { [[ $SWAPS_FAIL == 0 ]]; }
'''
        assignments='\n'.join(k+'='+shlex.quote(v) for k,v in dict(PROTECTED=protected,MOUNT=mount,CHILD=child,READONLY=readonly,DISK_TYPE=disk_type,CONSUMERS_FAIL=consumers_fail,SWAPS_FAIL=swaps_fail).items())
        return self.shell(assignments+'\n'+mock+'\niso_validate_disk /dev/test "$PROTECTED"')

    def test_unused_whole_disk_is_accepted(self): self.assertEqual(self.disk_check().returncode,0)
    def test_live_media_is_rejected(self): self.assertNotEqual(self.disk_check(protected='/dev/test').returncode,0)
    def test_mounted_target_is_rejected(self): self.assertNotEqual(self.disk_check(mount='/home').returncode,0)
    def test_active_swap_target_is_rejected(self): self.assertNotEqual(self.disk_check(mount='[SWAP]').returncode,0)
    def test_mapper_target_is_rejected(self): self.assertNotEqual(self.disk_check(child='crypt').returncode,0)
    def test_raid_member_is_rejected(self): self.assertNotEqual(self.disk_check(child='raid1').returncode,0)
    def test_readonly_target_is_rejected(self): self.assertNotEqual(self.disk_check(readonly='1').returncode,0)
    def test_partition_target_is_rejected(self): self.assertNotEqual(self.disk_check(disk_type='part').returncode,0)
    def test_failed_consumer_query_is_rejected(self): self.assertNotEqual(self.disk_check(consumers_fail='1').returncode,0)
    def test_failed_swap_query_is_rejected(self): self.assertNotEqual(self.disk_check(swaps_fail='1').returncode,0)

    def test_recloning_resets_previous_firstboot_choices_and_completion(self):
        self.file('etc/xetal-firstboot.conf', 'CHANGE_USER=1\n')
        self.file('etc/systemd/system/multi-user.target.wants/xetal-firstboot.service')
        self.file('var/lib/xetal-firstboot/done/ssh')
        self.file('var/lib/xetal-firstboot/completed.conf', 'previous choices')
        (self.root/'etc/xetal-source').mkdir()
        result=self.shell('source "$1"\ninstaller_reset_firstboot "$2"', REPO/'iso/installer.sh', self.root)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertFalse((self.root/'etc/xetal-firstboot.conf').exists())
        self.assertFalse((self.root/'etc/systemd/system/multi-user.target.wants/xetal-firstboot.service').exists())
        self.assertFalse((self.root/'var/lib/xetal-firstboot/done').exists())
        self.assertEqual((self.root/'etc/xetal-source/firstboot.conf').read_text(),'CHANGE_USER=1\n')
        self.assertTrue((self.root/'var/lib/xetal-firstboot/completed.conf').exists())

    def network_check(self, mode):
        records=self.root/'network-changes'
        code=r'''
source "$1"
NM_MODE=$2
NM_RECORD=$3
nmcli() {
    case "$*" in
        'general status') [[ $NM_MODE != inactive ]] ;;
        '-t -f UUID connection show') [[ $NM_MODE != failed ]] || return 1; printf 'stale\ncurrent\nbridge\nvpn\n' ;;
        '-g connection.type connection show uuid stale'|'-g connection.type connection show uuid current') echo 802-3-ethernet ;;
        '-g connection.type connection show uuid bridge') echo bridge ;;
        '-g connection.type connection show uuid vpn') echo vpn ;;
        '-g connection.interface-name connection show uuid stale') echo old-interface ;;
        '-g connection.interface-name connection show uuid current') echo current-interface ;;
        'connection modify uuid stale connection.interface-name ') printf '%s\n' "$@" >> "$NM_RECORD" ;;
        *) return 1 ;;
    esac
}
ip() { [[ $* == 'link show current-interface' ]]; }
firstboot_network
'''
        return self.shell(code, REPO/'iso/firstboot.sh', mode, records), records

    def test_network_adaptation_only_removes_stale_physical_binding(self):
        result,records=self.network_check('active')
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(records.read_text().splitlines(),['connection','modify','uuid','stale','connection.interface-name',''])

    def test_network_adaptation_reports_inactive_service(self):
        result,records=self.network_check('inactive')
        self.assertNotEqual(result.returncode,0)
        self.assertFalse(records.exists())

    def test_network_adaptation_reports_failed_profile_query(self):
        result,records=self.network_check('failed')
        self.assertNotEqual(result.returncode,0)
        self.assertFalse(records.exists())

    def test_offline_export_is_complete_and_never_overwrites(self):
        self.file('build/db/sync/core.db'); self.file('build/db/sync/extra.db')
        self.file('build/package-files.txt','linux-1-1-x86_64.pkg.tar.zst\n')
        self.file('build/profile/packages.x86_64','linux\n')
        self.file('build/pkg/linux-1-1-x86_64.pkg.tar.zst')
        archive=self.root/'packages.tar.gz'
        args=['bash',str(REPO/'create_offline_package.sh'),str(self.root/'build'),str(archive)]
        result=subprocess.run(args,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        listing=subprocess.check_output(['tar','-tf',str(archive)],text=True)
        self.assertIn('offline-packages/pkg/linux-1-1-x86_64.pkg.tar.zst',listing)
        before=archive.read_bytes()
        self.assertNotEqual(subprocess.run(args,capture_output=True).returncode,0)
        self.assertEqual(archive.read_bytes(),before)

    def test_offline_export_rejects_missing_packages(self):
        self.file('build/db/sync/core.db'); self.file('build/db/sync/extra.db')
        self.file('build/package-files.txt','linux-1-1-x86_64.pkg.tar.zst\n')
        self.file('build/profile/packages.x86_64','linux\n')
        archive=self.root/'packages.tar.gz'
        result=subprocess.run(['bash',str(REPO/'create_offline_package.sh'),str(self.root/'build'),str(archive)],capture_output=True,text=True)
        self.assertNotEqual(result.returncode,0)
        self.assertFalse(archive.exists())

    def test_every_shell_script_parses(self):
        for script in list((REPO/'iso').glob('*.sh'))+[REPO/'create_offline_package.sh']:
            result=subprocess.run(['bash','-n',str(script)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,str(script)+result.stderr)

if __name__=='__main__': unittest.main()
