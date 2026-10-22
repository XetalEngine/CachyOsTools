"""Dummy previews: real ISO remastering and installer controls, without host disk writes."""
import hashlib
import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
DUMMY = REPO / 'iso/dummy.sh'
INSTALLER = REPO / 'iso/installer.sh'


class DummyImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not all(shutil.which(tool) for tool in ('xorriso', 'tar', 'zstd')):
            raise unittest.SkipTest('xorriso, tar and zstd are needed')

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso-dummy-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.cache = self.root/'dummy-cache-v1/live-base.iso'
        self.cache.parent.mkdir()
        tree = self.root/'seed'
        tree.mkdir()
        # Minimal boot catalog fixtures. This verifies replaying both firmware
        # entries; these placeholder boot images do not boot a live OS.
        (tree/'bios.img').write_bytes(bytes(4096))
        (tree/'efi.img').write_bytes(bytes(1024 * 1024))
        self.run_command(['xorriso', '-as', 'mkisofs', '-o', str(self.cache),
                          '-b', 'bios.img', '-no-emul-boot', '-boot-load-size', '4',
                          '-eltorito-alt-boot', '-e', 'efi.img', '-no-emul-boot',
                          '--protective-msdos-label', str(tree)])
        self.original_hash = hashlib.sha256(self.cache.read_bytes()).hexdigest()
        build = (REPO/'iso/build.sh').read_text().split('# Run the selected ISO build.', 1)[0]
        self.helpers = self.root/'build-helpers.sh'
        self.helpers.write_text(build)

    def run_command(self, args, **kwargs):
        result = subprocess.run(args, capture_output=True, text=True, timeout=30, **kwargs)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def build(self, payload='first payload', cold=False):
        if cold:
            self.cache.rename(self.root/'seed.iso')
        code = r'''
set -Eeuo pipefail
source "$1/iso/common.sh"
source "$1/iso/dummy.sh"
source "$2/build-helpers.sh"
BASE="$2/base"
OUTPUT_DIR="$2/output"
ISO_NAME=xetal-dummy
ISO_OFFLINE=1
OFFLINE_PACKAGE="$2/intentionally-missing.tar.gz"
DUMMY_CACHE="$2/dummy-cache-v1/live-base.iso"
stage_iso_payload() {
    mkdir -p "$PROFILE/airootfs/opt/clone" "$PROFILE/airootfs/usr/local/bin"
    printf '%s' "$PAYLOAD" > "$PROFILE/airootfs/usr/local/bin/installer.sh"
    printf 'fixture logo' > "$PROFILE/airootfs/opt/clone/logo.png"
}
run_sudo() { echo 'UNEXPECTED SUDO' >&2; exit 90; }
rsync() { echo 'UNEXPECTED SNAPSHOT' >&2; exit 91; }
iso_prepare_packages() { echo 'UNEXPECTED PACKAGE DOWNLOAD' >&2; exit 92; }
# The fake cold build only supplies the boot catalog fixture; it never runs mkarchiso.
iso_dummy_live_base() {
    echo BASE_BUILD >> "$FIXTURE/base-calls"
    cp "$FIXTURE/seed.iso" "$DUMMY_CACHE"
}
mkdir -p "$BASE/xiso"
printf 'real-clone-build' > "$BASE/xiso/last-build.txt"
iso_dummy_main
'''
        result = self.run_command(['bash', '-c', code, 'test', str(REPO), str(self.root)],
                                 env=dict(os.environ, PAYLOAD=payload, FIXTURE=str(self.root)))
        self.assertEqual((self.root/'base/xiso/last-build.txt').read_text(), 'real-clone-build')
        self.assertEqual(hashlib.sha256(self.cache.read_bytes()).hexdigest(), self.original_hash)
        run = Path((self.root/'base/xiso/last-dummy-build.txt').read_text().strip())
        images = list((self.root/'output').glob(f'xetal-dummy-{run.name}-*.iso'))
        self.assertEqual(len(images), 1)
        return images[0], result

    def extract_payload(self, image):
        path = self.root/(image.stem + '.tar')
        self.run_command(['xorriso', '-osirrox', 'on', '-indev', str(image),
                          '-extract', '/xetal-preview.tar', str(path)])
        with tarfile.open(path) as archive:
            names = archive.getnames()
            self.assertTrue(all(name == 'opt/clone' or name.startswith('opt/clone/') or
                                name == 'usr/local/bin/installer.sh' for name in names))
            self.assertIn('opt/clone/dummy-iso', names)
            snapshot = archive.extractfile('opt/clone/rootfs-snapshot.tar.zst').read()
            result = subprocess.run(['zstd', '-d', '-c'], input=snapshot,
                                    capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            with tarfile.open(fileobj=io.BytesIO(result.stdout)) as empty:
                self.assertEqual(empty.getmembers(), [])
            checksum = archive.extractfile('opt/clone/snapshot.sha256').read().decode().split()[0]
            self.assertEqual(hashlib.sha256(snapshot).hexdigest(), checksum)
            self.assertIn(b'DUMMY=1\n', archive.extractfile('opt/clone/snapshot.meta').read())
            return archive.extractfile('usr/local/bin/installer.sh').read().decode()

    def test_cached_refresh_keeps_boot_catalog_and_never_takes_a_snapshot(self):
        image, result = self.build()
        self.assertIn('no downloads or sudo needed', result.stdout)
        self.assertFalse((self.root/'base-calls').exists())
        report = self.run_command(['xorriso', '-indev', str(image), '-report_el_torito', 'plain',
                                   '-report_system_area', 'plain'])
        self.assertRegex(report.stdout, r'El Torito boot img\s*:\s*1\s+BIOS')
        self.assertRegex(report.stdout, r'El Torito boot img\s*:\s*2\s+UEFI')
        self.assertIn('/bios.img', report.stdout)
        self.assertIn('/efi.img', report.stdout)
        self.assertIn('MBR', report.stdout)
        self.assertEqual(self.extract_payload(image), 'first payload')

    def test_repeat_refresh_uses_latest_payload_and_keeps_previous_iso(self):
        first, _ = self.build('first')
        first_hash = hashlib.sha256(first.read_bytes()).hexdigest()
        second, _ = self.build('updated installer and logo')
        self.assertNotEqual(first, second)
        self.assertEqual(hashlib.sha256(first.read_bytes()).hexdigest(), first_hash)
        self.assertEqual(self.extract_payload(second), 'updated installer and logo')

    def test_only_first_request_builds_the_live_base(self):
        first, _ = self.build(cold=True)
        second, _ = self.build('fresh')
        self.assertTrue(first.exists() and second.exists())
        self.assertEqual((self.root/'base-calls').read_text(), 'BASE_BUILD\n')

    def test_live_base_preparation_never_reads_or_copies_the_source_system(self):
        profile = self.root/'releng'
        profile.mkdir()
        (profile/'profiledef.sh').write_text("bootmodes=('bios.syslinux' 'uefi.systemd-boot')\n")
        (profile/'packages.x86_64').write_text('base\nlinux\n')
        keyring = self.root/'archlinux.gpg'
        keyring.write_text('test keyring')
        helpers = self.root/'dummy-helpers.sh'
        helpers.write_text(DUMMY.read_text().replace('/usr/share/pacman/keyrings/archlinux.gpg', str(keyring)))
        code = r'''
set -Eeuo pipefail
source "$1/iso/common.sh"
source "$2/dummy-helpers.sh"
RUN="$2/cold-run"; RELENG="$2/releng"; DUMMY_CACHE="$2/prepared/live-base.iso"
ISO_OFFLINE=0
mkdir -p "$RUN"
iso_need() { :; }
iso_prepare_packages() {
    test -s "$PROFILE/pacman.conf"
    test -f "$PROFILE/profiledef.sh"
    grep -qx dialog "$PROFILE/packages.x86_64"
    grep -qx chafa "$PROFILE/packages.x86_64"
}
stage_iso_payload() {
    mkdir -p "$PROFILE/airootfs/usr/local/bin" "$PROFILE/airootfs/opt/clone"
    printf '#!/bin/bash\n' > "$PROFILE/airootfs/usr/local/bin/installer.sh"
}
iso_unmount_tree() { :; }
run_sudo() {
    case "$1" in
        true|pacman-key) : ;;
        mkarchiso)
            # The production profile must already contain a valid empty archive
            # and the payload launcher before invoking the external ISO builder.
            test -f "$PROFILE/airootfs/opt/clone/dummy-iso"
            test -z "$(tar -I zstd -tf "$PROFILE/airootfs/opt/clone/rootfs-snapshot.tar.zst")"
            grep -q /run/archiso/bootmnt/xetal-preview.tar "$PROFILE/airootfs/xetal.sh"
            cp "$FIXTURE/dummy-cache-v1/live-base.iso" "$RUN/base-output/xetal-dummy-base-test.iso"
            ;;
        chown|rm) "$@" ;;
        *) echo "Forbidden privileged operation: $*" >&2; return 99 ;;
    esac
}
rsync() { echo 'Forbidden source snapshot' >&2; return 99; }
iso_host_supported() { return 99; }
iso_kernels() { return 99; }
iso_bootloader() { return 99; }
iso_check_source_mounts() { return 99; }
iso_dummy_live_base
'''
        self.run_command(['bash', '-c', code, 'test', str(REPO), str(self.root)],
                         input='fixture-password\n', env=dict(os.environ, FIXTURE=str(self.root)))
        self.assertEqual(hashlib.sha256((self.root/'prepared/live-base.iso').read_bytes()).hexdigest(),
                         self.original_hash)


class DummyInstallerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='dummy-installer-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.clone = self.root/'clone'
        self.clone.mkdir()
        (self.root/'efi').mkdir()
        (self.clone/'rootfs-snapshot.tar.zst').write_bytes(b'fixture')
        (self.clone/'common.sh').write_text(r'''
iso_die() { echo "$*" >&2; return 1; }
iso_need() { :; }
iso_validate_disk() { :; }
iso_validate_partition() { :; }
iso_validate_esp_target() { :; }
iso_signing_enabled() { return 1; }
''')
        source = INSTALLER.read_text().replace('/opt/clone', str(self.clone))
        source = source.replace('/sys/firmware/efi', str(self.root/'efi')).replace('[[ $EUID == 0 ]]', 'true')
        self.script = self.root/'installer.sh'
        self.script.write_text(source)

    def run_preview(self, mode='wipe', marker=True, manifest=True, confirm=0, bios=False, extra_setup=''):
        (self.clone/'dummy-iso').unlink(missing_ok=True)
        if marker:
            (self.clone/'dummy-iso').touch()
        metadata = 'FORMAT=1\nARCH=x86_64\nBYTES=0\nKERNELS=1\nUEFI=1\nBIOS=1\n'
        (self.clone/'snapshot.meta').write_text(metadata + ('DUMMY=1\n' if manifest else ''))
        if bios:
            (self.root/'efi').rmdir()
        code = r'''
source "$1"
fixture=$2
installer_show_logo() { :; }
installer_menu() {
    case "$1" in ' XETAL ENGINE ') echo "$MODE" ;; *) echo /dev/test ;; esac
}
installer_protected_disks() { :; }
installer_target_identity() { echo 'stable fixture disk'; }
installer_select_free() { SEL_DISK=/dev/test; SEL_START=4096; SEL_END=20480; }
installer_select_parts() { SEL_DISK=/dev/test; SEL_ROOT=/dev/test2; SEL_ESP=/dev/test1; }
lsblk() {
    case "$*" in
        *NAME,TYPE*) echo '/dev/test disk' ;;
        *-bdnro*) echo 53687091200 ;;
        *SIZE,MODEL*) echo '50G Fixture Disk' ;;
        *LABEL,PARTLABEL*) echo 'Fixture' ;;
    esac
}
clear() { :; }
dialog() {
    printf '%s\n' "$*" >> "$fixture/dialogs"
    case " $* " in *' --yesno '*) return "$CONFIRM" ;; esac
}
for cmd in parted partprobe udevadm wipefs mkfs.fat mkfs.ext4 mount umount tar arch-chroot genfstab; do
    eval "$cmd() { echo $cmd >> \"\$fixture/forbidden\"; return 99; }"
done
# Even if the dummy guard regresses, stop before the real installation area.
mktemp() { echo INSTALLATION_STARTED >> "$fixture/forbidden"; return 98; }
''' + extra_setup + '\ninstaller_main\n'
        master, slave = os.openpty()
        try:
            result = subprocess.run(['bash', '-c', code, 'test', str(self.script), str(self.root)],
                                    stdin=slave, capture_output=True, text=True, timeout=5,
                                    env=dict(os.environ, MODE=mode, CONFIRM=str(confirm)))
        finally:
            os.close(slave)
            os.close(master)
        return result

    def test_all_install_modes_confirm_and_simulate_without_disk_writes(self):
        for mode in ('wipe', 'free', 'parts'):
            with self.subTest(mode=mode):
                (self.root/'dialogs').unlink(missing_ok=True)
                result = self.run_preview(mode)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn('[6/6] Preview complete.', result.stdout)
                dialogs = (self.root/'dialogs').read_text()
                self.assertEqual(dialogs.count('--yesno'), 2)
                self.assertIn('Dummy ISO preview complete', dialogs)
                self.assertFalse((self.root/'forbidden').exists())

    def test_bios_wipe_preview_also_cannot_write_disks(self):
        result = self.run_preview(bios=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.root/'forbidden').exists())

    def test_marker_or_manifest_independently_forces_simulation(self):
        for marker, manifest in ((True, False), (False, True)):
            with self.subTest(marker=marker, manifest=manifest):
                result = self.run_preview(marker=marker, manifest=manifest)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn('Preview complete.', result.stdout)
                self.assertFalse((self.root/'forbidden').exists())

    def test_cancelling_a_confirmation_does_not_simulate_or_install(self):
        result = self.run_preview(confirm=1)
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('Preview complete.', result.stdout)
        self.assertFalse((self.root/'forbidden').exists())

    def test_normal_iso_still_reaches_real_installation_after_confirmation(self):
        result = self.run_preview(marker=False, manifest=False)
        self.assertEqual(result.returncode, 98, result.stdout + result.stderr)
        self.assertEqual((self.root/'forbidden').read_text(), 'INSTALLATION_STARTED\n')
        self.assertNotIn('Preview complete.', result.stdout)

    def test_back_from_target_returns_to_actions_and_preserves_verified_status(self):
        result = self.run_preview(extra_setup=r'''
SNAPSHOT_CHECK=passed
installer_menu() {
    if [[ $1 == ' XETAL ENGINE ' ]]; then
        echo "$SNAPSHOT_CHECK" >> "$fixture/home-visits"
        echo wipe
    elif [[ ! -f $fixture/target-visited ]]; then
        touch "$fixture/target-visited"
        return 10
    else
        echo /dev/test
    fi
}
''')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((self.root/'home-visits').read_text(), 'passed\npassed\n')
        self.assertFalse((self.root/'forbidden').exists())

    def test_back_from_final_warning_requires_both_confirmations_again(self):
        result = self.run_preview(extra_setup=r'''
installer_ui() {
    [[ $1 == confirm ]] || return 2
    echo "$2" >> "$fixture/modern-confirmations"
    if [[ $2 == 'FINAL WARNING' && ! -f $fixture/warning-visited ]]; then
        touch "$fixture/warning-visited"
        return 10
    fi
}
''')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((self.root/'modern-confirmations').read_text().splitlines(),
                         ['Confirm target', 'FINAL WARNING'] * 2)
        self.assertFalse((self.root/'forbidden').exists())


if __name__ == '__main__':
    unittest.main()
