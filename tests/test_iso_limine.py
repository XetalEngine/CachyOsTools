"""Limine support: detection, generated config, hash pinning, and branch regressions.

restore-boot.sh is run for real inside a throw-away directory tree: every absolute
path under /efi, /boot, /etc and /usr is rewritten to the sandbox, and the system
tools (mkinitcpio, limine, sbctl wrappers, mountpoint) are shell mocks.
"""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
COMMON = REPO / 'iso/common.sh'
RESTORE = REPO / 'iso/restore-boot.sh'
INSTALLER = REPO / 'iso/installer.sh'

ROOT_UUID = '1234abcd-12ab-34cd-56ef-0123456789ab'
HEX128 = re.compile(r'^[0-9a-f]{128}$')


def b2(path):
    return hashlib.blake2b(Path(path).read_bytes()).hexdigest()


class DetectionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso-limine-detect-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def file(self, name, content='x', mode=0o755):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        path.chmod(mode)

    def limine_package(self):
        self.file('usr/bin/limine')
        self.file('usr/share/limine/BOOTX64.EFI')

    def systemd_boot(self):
        self.file('usr/bin/bootctl')
        self.file('usr/lib/systemd/boot/efi/systemd-bootx64.efi')

    def bootloader(self, mode='x86_64-efi'):
        return subprocess.run(['bash', '-c', 'source "$1"; iso_bootloader "$2" "$3"', 'test',
                               str(COMMON), str(self.root), mode], capture_output=True, text=True)

    def test_cachyos_style_limine_source_is_detected(self):
        self.limine_package()
        self.systemd_boot()  # bootctl ships with systemd on every Arch system
        self.file('etc/default/limine')
        self.assertEqual(self.bootloader().stdout.strip(), 'limine')

    def test_each_known_marker_is_enough(self):
        for marker in ('etc/default/limine', 'etc/limine-entry-tool.conf', 'boot/limine.conf',
                       'boot/limine/limine.conf', 'efi/limine.conf', 'boot/EFI/BOOT/limine.conf'):
            with self.subTest(marker=marker):
                self.setUp()
                self.limine_package()
                self.file(marker)
                self.assertEqual(self.bootloader().stdout.strip(), 'limine')

    def test_installed_package_alone_does_not_override_systemd_boot(self):
        self.limine_package()
        self.systemd_boot()
        self.assertEqual(self.bootloader().stdout.strip(), 'systemd-boot')

    def test_grub_keeps_priority_over_limine(self):
        self.limine_package()
        self.file('etc/default/limine')
        self.file('usr/bin/grub-install')
        self.file('usr/lib/grub/x86_64-efi/modinfo.sh')
        self.assertEqual(self.bootloader().stdout.strip(), 'grub')

    def test_marker_without_the_limine_binary_is_not_enough(self):
        self.file('etc/default/limine')
        self.systemd_boot()
        self.assertEqual(self.bootloader().stdout.strip(), 'systemd-boot')
        self.file('usr/bin/limine')  # binary but no EFI file
        self.assertEqual(self.bootloader().stdout.strip(), 'systemd-boot')

    def test_bios_mode_never_selects_limine(self):
        self.limine_package()
        self.file('etc/default/limine')
        result = self.bootloader('i386-pc')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Limine or systemd-boot', result.stderr)


class RestoreBootTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso-limine-boot-')
        self.addCleanup(self.tmp.cleanup)
        self.r = Path(self.tmp.name)
        for directory in ('efi', 'boot', 'etc', 'usr/lib/modules/6.1.0-test', 'usr/share/limine', 'usr/local/lib/xetal-iso'):
            (self.r / directory).mkdir(parents=True)
        (self.r / 'usr/lib/modules/6.1.0-test/vmlinuz').write_bytes(b'KERNEL-IMAGE')
        (self.r / 'usr/share/limine/BOOTX64.EFI').write_bytes(b'LIMINE-BINARY')
        (self.r / 'boot/intel-ucode.img').write_bytes(b'INTEL-MICROCODE')
        (self.r / 'usr/local/lib/xetal-iso/common.sh').write_text(COMMON.read_text() + r'''
iso_initramfs_tool() { echo mkinitcpio; }
iso_kernels() {
    if [[ -n ${MOCK_KERNELS:-} ]]; then
        local v p
        for p in $MOCK_KERNELS; do v=${p#linux-}-ver; printf '%s\t%s\t%s\n' "$v" "$p" "$KERNEL_FILE"; done
    else printf '6.1.0-test\tlinux-test\t%s\n' "$KERNEL_FILE"; fi
}
iso_sbctl_ready() { [[ ${MOCK_SB:-0} == 1 ]]; }
iso_sign_boot_file() {
    iso_signing_enabled || return 0
    echo "sign $1" >> "$MOCK_LOG"
    [[ ${MOCK_FAULT:-} != sign ]] || return 1
    printf SIGNED >> "$1"
}
iso_windows_esp_guid() { printf '%s' "${MOCK_WIN:-}"; }
''')
        text = RESTORE.read_text().replace('[[ $EUID == 0 ]]', 'true')
        text = re.sub(r'(^|[ "\'=(:])/(efi|boot|etc|usr)(?=[/ "\'\n)]|$)', lambda m: f'{m.group(1)}{self.r}/{m.group(2)}',
                      text, flags=re.M)
        self.script = self.r / 'restore-boot.sh'
        self.script.write_text(text)
        self.log = self.r / 'mock.log'
        self.log.write_text('')

    def boot_conf(self, bootloader, extra=''):
        (self.r / 'etc/xetal-boot.conf').write_text(f'ROOT_UUID={ROOT_UUID}\nBOOTLOADER={bootloader}\n{extra}')

    def run_boot(self, bootloader, sb=False, win='', extra='', kernels='', fault='', image='INITRAMFS', unavailable=''):
        self.boot_conf(bootloader, f'SIGN_BOOT={int(sb)}\n' + extra)
        body = r'''
source "$1"
mountpoint() { return 0; }
grub-script-check() { return 0; }
modinfo() { [[ -z ${MOCK_UNAVAILABLE:-} || $2 != "$MOCK_UNAVAILABLE" ]]; }
mkinitcpio() {
    local out='' cfg='' ver='' i
    for ((i=1; i<=$#; i++)); do
        case ${!i} in -g) out=${@:i+1:1} ;; -c) cfg=${@:i+1:1} ;; -k) ver=${@:i+1:1} ;; esac
    done
    [[ -n $out ]] || return 0
    [[ ${MOCK_FAULT:-} != initramfs ]] || return 1
    if [[ -n $cfg ]]; then
        printf '%s: %s\n' "$ver" "$(head -n1 "$cfg")" >> "$MOCK_LOG.modules"
        if [[ -n ${MOCK_UNAVAILABLE:-} && $ver == "$MOCK_UNAVAILABLE" ]] && grep -q nvidia "$cfg"; then return 1; fi
    fi
    printf '%s' "$MOCK_IMAGE" > "$out"
}
limine() {
    echo "enroll $*" >> "$MOCK_LOG"
    [[ ${MOCK_FAULT:-} != enroll ]] || return 1
    printf ENROLLED >> "$2"
}
mv() {
    if [[ ${MOCK_FAULT:-} == commit && $1 == */BOOTX64.EFI.new ]]; then return 1; fi
    command mv "$@"
}
restore_boot_main
'''
        env = dict(os.environ, MOCK_LOG=str(self.log), MOCK_SB='1' if sb else '0', MOCK_WIN=win, MOCK_KERNELS=kernels, MOCK_FAULT=fault, MOCK_IMAGE=image, MOCK_UNAVAILABLE=unavailable,
                   KERNEL_FILE=str(self.r / 'usr/lib/modules/6.1.0-test/vmlinuz'))
        return subprocess.run(['bash', '-c', body, 'test', str(self.script)],
                              capture_output=True, text=True, timeout=30, env=env)

    def efi(self, name):
        if name.startswith('Xetal/') and not name.startswith('Xetal/generation-'):
            conf = self.r / 'efi/EFI/BOOT/limine.conf'
            if conf.exists():
                match = re.search(r'/Xetal/(generation-[A-Za-z0-9]+)/', conf.read_text())
                if match: name = 'Xetal/' + match[1] + '/' + name.removeprefix('Xetal/')
        return self.r / 'efi' / name

    # ---- Limine ----------------------------------------------------------
    def test_limine_config_binary_and_files_without_secure_boot(self):
        result = self.run_boot('limine')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        conf = self.efi('EFI/BOOT/limine.conf').read_text()
        conf = re.sub(r'/generation-[A-Za-z0-9]+/', '/', conf)
        self.assertEqual(conf.splitlines()[0], 'timeout: 5')
        for line in ('/Cloned system - linux-test', '    protocol: linux',
                     '    path: boot():/Xetal/vmlinuz-6.1.0-test',
                     '    module_path: boot():/Xetal/intel-ucode.img',
                     '    module_path: boot():/Xetal/initramfs-6.1.0-test.img',
                     f'    cmdline: root=UUID={ROOT_UUID} rw'):
            self.assertIn(line + '\n', conf)
        self.assertNotIn('#', conf)  # no hash pinning without sbctl keys
        # microcode must load before the initramfs
        self.assertLess(conf.index('intel-ucode.img'), conf.index('initramfs-6.1.0-test.img'))
        self.assertEqual(self.efi('EFI/BOOT/BOOTX64.EFI').read_bytes(), b'LIMINE-BINARY')
        self.assertEqual(self.efi('Xetal/vmlinuz-6.1.0-test').read_bytes(), b'KERNEL-IMAGE')
        self.assertEqual(self.efi('Xetal/initramfs-6.1.0-test.img').read_bytes(), b'INITRAMFS')
        self.assertEqual(self.efi('Xetal/intel-ucode.img').read_bytes(), b'INTEL-MICROCODE')
        self.assertEqual(self.log.read_text(), '')  # nothing enrolled or signed
        self.assertFalse(self.efi('loader').exists())  # no systemd-boot leftovers
        self.assertFalse((self.efi('EFI/BOOT/limine.conf.new')).exists())
        self.assertFalse((self.efi('EFI/BOOT/BOOTX64.EFI.new')).exists())

    def test_default_entry_prefers_the_first_non_lts_kernel(self):
        result = self.run_boot('limine', kernels='linux-lts linux linux-zen')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        lines = self.efi('EFI/BOOT/limine.conf').read_text().splitlines()
        self.assertEqual(lines[:2], ['timeout: 5', 'default_entry: 2'])
        titles = [l for l in lines if l.startswith('/')]
        self.assertEqual(titles[1], '/Cloned system - linux')

    def test_no_default_entry_when_the_first_kernel_is_already_the_default(self):
        for kernels in ('', 'linux linux-lts', 'linux-lts'):
            with self.subTest(kernels=kernels):
                self.assertEqual(self.run_boot('limine', kernels=kernels).returncode, 0)
                self.assertNotIn('default_entry', self.efi('EFI/BOOT/limine.conf').read_text())

    def test_limine_hash_pinning_enrolls_the_final_config_then_signs(self):
        result = self.run_boot('limine', sb=True)
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        conf_path = self.efi('EFI/BOOT/limine.conf')
        conf = conf_path.read_text()
        pinned = dict(re.findall(r'(?:path|module_path): boot\(\):/Xetal/generation-[A-Za-z0-9]+/(\S+?)#(\S+)', conf))
        self.assertEqual(set(pinned), {'vmlinuz-6.1.0-test', 'intel-ucode.img', 'initramfs-6.1.0-test.img'})
        for name, digest in pinned.items():
            self.assertRegex(digest, HEX128)
            self.assertEqual(digest, b2(self.efi('Xetal/' + name)), name)
        # every path line carries a hash (Limine panics on an unpinned path when enrolled)
        for line in conf.splitlines():
            if re.match(r'\s+(path|module_path):', line):
                self.assertIn('#', line)
        log = self.log.read_text().splitlines()
        self.assertEqual(len(log), 3, log)
        # the kernel copy is signed first, before its hash is pinned (signing changes the file)
        self.assertEqual(log[0], f'sign {self.efi("Xetal/vmlinuz-6.1.0-test")}')
        enroll, sign = log[1:]
        self.assertTrue(enroll.startswith('enroll enroll-config '), enroll)
        self.assertTrue(enroll.split()[2].endswith('BOOTX64.EFI'))
        self.assertEqual(enroll.split()[3], b2(conf_path))  # hash of the config exactly as written
        self.assertEqual(sign, f'sign {self.efi("Xetal/BOOTX64.EFI")}')
        # the enrolled binary is what ended up at the final path
        self.assertEqual(self.efi('EFI/BOOT/BOOTX64.EFI').read_bytes(), b'LIMINE-BINARYENROLLEDSIGNED')

    # ---- early GPU modules (an NVIDIA system needs its driver in the initramfs) --------
    def test_gpu_modules_listed_by_the_source_are_kept_and_layout_modules_dropped(self):
        (self.r / 'etc/mkinitcpio.conf.d').mkdir()
        (self.r / 'etc/mkinitcpio.conf').write_text('MODULES=(btrfs)\nHOOKS=(base udev)\n')
        (self.r / 'etc/mkinitcpio.conf.d/10-nvidia.conf').write_text(
            'MODULES+=(nvidia nvidia_modeset nvidia_uvm nvidia_drm dm_crypt nvidia)\nHOOKS+=(sd-btrfs-overlayfs)\n')
        result = self.run_boot('limine')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        conf = (self.r / 'etc/mkinitcpio-xetal.conf').read_text().splitlines()
        self.assertEqual(conf[0], 'MODULES=(ext4 nvidia nvidia_modeset nvidia_uvm nvidia_drm)')
        self.assertIn('HOOKS=(base udev modconf keyboard block filesystems fsck)', conf)  # hooks untouched

    def test_without_source_gpu_modules_the_initramfs_config_is_unchanged(self):
        self.assertEqual(self.run_boot('limine').returncode, 0)
        conf = (self.r / 'etc/mkinitcpio-xetal.conf').read_text()
        self.assertEqual(conf, 'MODULES=(ext4)\nBINARIES=()\nFILES=()\n'
                               'HOOKS=(base udev modconf keyboard block filesystems fsck)\nCOMPRESSION="gzip"\n')

    def test_limine_graphics_no_is_written_and_covered_by_the_enrolled_hash(self):
        result = self.run_boot('limine', sb=True, extra='LIMINE_GRAPHICS=no\n')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        conf_path = self.efi('EFI/BOOT/limine.conf')
        self.assertEqual(conf_path.read_text().splitlines()[:2], ['timeout: 5', 'graphics: no'])
        enroll = self.log.read_text().splitlines()[1]
        self.assertEqual(enroll.split()[3], b2(conf_path))

    def test_limine_graphics_is_not_written_by_default_or_for_other_values(self):
        for extra in ('', 'LIMINE_GRAPHICS=yes\n', 'LIMINE_GRAPHICS=nope\n'):
            self.setUp()  # fresh sandbox for each case
            self.assertEqual(self.run_boot('limine', extra=extra).returncode, 0, extra)
            self.assertNotIn('graphics', self.efi('EFI/BOOT/limine.conf').read_text(), extra)

    WIN_GUID = '1b2c3d4e-0000-4000-8000-aabbccddeeff'

    def test_windows_entry_is_added_unpinned_and_covered_by_the_enrolled_config(self):
        result = self.run_boot('limine', sb=True, win=self.WIN_GUID)
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        conf_path = self.efi('EFI/BOOT/limine.conf')
        conf = conf_path.read_text()
        self.assertIn(f'\n/Windows\n    protocol: efi\n    path: guid({self.WIN_GUID}):/EFI/Microsoft/Boot/bootmgfw.efi\n', conf)
        self.assertNotIn('bootmgfw.efi#', conf)  # Windows updates would stale a pinned hash
        enroll = self.log.read_text().splitlines()[1]
        self.assertEqual(enroll.split()[3], b2(conf_path))  # the enrolled hash includes the Windows entry

    def test_no_windows_entry_when_none_found(self):
        self.assertEqual(self.run_boot('limine').returncode, 0)
        self.assertNotIn('Windows', self.efi('EFI/BOOT/limine.conf').read_text())

    def test_windows_entry_is_limine_only(self):
        self.assertEqual(self.run_boot('systemd-boot', win=self.WIN_GUID).returncode, 0)
        self.assertNotIn('Windows', ''.join(p.read_text() for p in self.efi('loader').rglob('*.conf')))

    def test_keeps_previous_generation_and_removes_only_older_owned_generations(self):
        (self.r / 'efi/Xetal').mkdir()
        unrelated = self.r / 'efi/Xetal/unrelated.txt'
        unrelated.write_text('keep')
        self.assertEqual(self.run_boot('limine').returncode, 0)
        first = self.efi('Xetal/vmlinuz-6.1.0-test').parent
        self.assertEqual(self.run_boot('limine', image='SECOND').returncode, 0)
        second = self.efi('Xetal/vmlinuz-6.1.0-test').parent
        self.assertTrue(first.exists())
        self.assertEqual(self.run_boot('limine', image='THIRD').returncode, 0)
        self.assertFalse(first.exists())
        self.assertTrue(second.exists())
        self.assertEqual(unrelated.read_text(), 'keep')
        previous = self.efi('EFI/XetalPrevious/limine.conf').read_text()
        self.assertIn(second.name, previous)

    def test_failed_updates_preserve_old_config_binary_and_referenced_files(self):
        self.assertEqual(self.run_boot('limine', sb=True).returncode, 0)
        files = [self.efi('EFI/BOOT/limine.conf'), self.efi('EFI/BOOT/BOOTX64.EFI'),
                 self.efi('Xetal/vmlinuz-6.1.0-test'), self.efi('Xetal/initramfs-6.1.0-test.img')]
        before = {str(p): p.read_bytes() for p in files}
        for fault in ('initramfs', 'sign', 'enroll', 'commit'):
            with self.subTest(fault=fault):
                result = self.run_boot('limine', sb=True, fault=fault, image='UPDATED')
                self.assertNotEqual(result.returncode, 0)
                self.assertNotIn('successfully', result.stdout)
                self.assertEqual({str(p): p.read_bytes() for p in files}, before)
                self.assertEqual(len(list((self.r/'efi/Xetal').glob('generation-*'))), 1)

    def test_missing_optional_gpu_modules_do_not_break_any_loader_or_other_kernels(self):
        (self.r/'etc/mkinitcpio.conf').write_text('MODULES=(nvidia nvidia_drm)\n')
        for loader in ('grub', 'systemd-boot', 'limine'):
            result = self.run_boot(loader, kernels='linux-main linux-lts', unavailable='lts-ver')
            self.assertEqual(result.returncode, 0, result.stderr)
        modules = Path(str(self.log)+'.modules').read_text()
        self.assertIn('main-ver: MODULES=(ext4 nvidia nvidia_drm)', modules)
        self.assertIn('lts-ver: MODULES=(ext4)', modules)

    def test_missing_limine_package_files_fail_cleanly_without_touching_the_binary(self):
        (self.r / 'usr/share/limine/BOOTX64.EFI').unlink()
        self.efi('EFI/BOOT').mkdir(parents=True)
        self.efi('EFI/BOOT/BOOTX64.EFI').write_bytes(b'OLD-WORKING-BINARY')
        result = self.run_boot('limine')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('limine package files are missing', result.stderr)
        self.assertEqual(self.efi('EFI/BOOT/BOOTX64.EFI').read_bytes(), b'OLD-WORKING-BINARY')
        self.assertFalse(self.efi('EFI/BOOT/limine.conf').exists())

    def test_requires_the_efi_partition_to_be_mounted(self):
        self.boot_conf('limine')
        body = 'source "$1"\nmountpoint() { return 1; }\nrestore_boot_main'
        result = subprocess.run(['bash', '-c', body, 'test', str(self.script)], capture_output=True, text=True,
                                env=dict(os.environ, KERNEL_FILE='x', MOCK_LOG=str(self.log)))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Mount the EFI system partition', result.stderr)

    # ---- the branches that existed before must still behave ----------------
    def test_systemd_boot_branch_is_unchanged(self):
        result = self.run_boot('systemd-boot')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        entry = self.efi('loader/entries/xetal-6.1.0-test.conf').read_text()
        self.assertIn('title Cloned system - linux-test', entry)
        self.assertIn('linux /Xetal/vmlinuz-6.1.0-test', entry)
        self.assertIn(f'options root=UUID={ROOT_UUID} rw', entry)
        self.assertEqual(self.efi('loader/loader.conf').read_text(), 'default xetal-*\ntimeout 5\n')
        self.assertFalse(self.efi('EFI/BOOT/limine.conf').exists())
        self.assertFalse(self.efi('EFI/BOOT/BOOTX64.EFI').exists())

    def test_grub_branch_is_unchanged(self):
        result = self.run_boot('grub')
        self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        cfg = (self.r / 'boot/grub/grub.cfg').read_text()
        self.assertIn("menuentry 'Cloned system - linux-test'", cfg)
        self.assertIn(f'root=UUID={ROOT_UUID} rw', cfg)
        self.assertFalse(self.efi('EFI/BOOT/limine.conf').exists())
        self.assertFalse((self.r / 'efi/loader').exists())


class InstallerWiringTests(unittest.TestCase):
    def test_hook_retriggers_on_limine_package_updates(self):
        self.assertIn('Target = usr/share/limine/BOOTX64.EFI', INSTALLER.read_text())

    def test_limine_never_falls_into_the_systemd_boot_install_branch(self):
        text = INSTALLER.read_text()
        self.assertIn("elif [[ $loader == limine ]]; then", text)
        self.assertIn("elif [[ $loader == limine ]]; then", text.split("bootctl --esp-path")[0])
        # bootctl must only be reachable in the final (systemd-boot) else branch
        before_bootctl = text.split('bootctl --esp-path')[0]
        self.assertLess(before_bootctl.rindex("elif [[ $loader == limine ]]"), before_bootctl.rindex('    else\n'))


class NoSignSwitchTests(unittest.TestCase):
    def ready(self, no_sign):
        with tempfile.TemporaryDirectory(prefix='iso-nosign-') as d:
            root = Path(d)
            (root / 'usr/bin').mkdir(parents=True)
            (root / 'usr/bin/sbctl').write_text('#!/bin/sh\n')
            (root / 'usr/bin/sbctl').chmod(0o755)
            (root / 'var/lib/sbctl/keys/db').mkdir(parents=True)
            (root / 'var/lib/sbctl/keys/db/db.key').write_text('KEY')
            (root / 'var/lib/sbctl/keys/db/db.pem').write_text('CERT')
            env = {k: v for k, v in os.environ.items() if k != 'XETAL_NO_SIGN'}
            if no_sign:
                env['XETAL_NO_SIGN'] = '1'
            return subprocess.run(['bash', '-c', f'source {COMMON}; iso_sbctl_ready "$1"', 'x', str(root)],
                                  env=env, capture_output=True, text=True, timeout=15).returncode

    def test_keys_present_means_ready(self):
        self.assertEqual(self.ready(False), 0)

    def test_xetal_no_sign_turns_signing_and_pinning_off(self):
        self.assertNotEqual(self.ready(True), 0)


class WindowsDetectionTests(unittest.TestCase):
    """The real iso_windows_esp_guid with lsblk/findmnt/mount/umount mocked."""
    ESP = 'c12a7328-f81f-11d2-ba4b-00a0c93ec93b'

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso-win-')
        self.addCleanup(self.tmp.cleanup)
        self.t = Path(self.tmp.name)

    def esp(self, name, with_windows):
        d = self.t / name
        (d / 'EFI/Microsoft/Boot').mkdir(parents=True)
        if with_windows:
            (d / 'EFI/Microsoft/Boot/bootmgfw.efi').write_bytes(b'MZ')

    def detect(self, rows, own='/dev/sda1'):
        body = r'''
source @COMMON@
lsblk() { cat <<'ROWS'
@ROWS@
ROWS
}
findmnt() { echo @OWN@; }
mount() { echo "mount $*" >> "$LOG"; local dev=${@: -2:1} mnt=${@: -1}; cp -a "$FAKE/${dev##*/}/." "$mnt"; }
umount() { echo "umount $*" >> "$LOG"; rm -rf "${1:?}"/*; }
iso_windows_esp_guid
'''.replace('@COMMON@', str(COMMON)).replace('@ROWS@', rows).replace('@OWN@', own)
        env = dict(os.environ, FAKE=str(self.t), LOG=str(self.t / 'log'))
        (self.t / 'log').write_text('')
        return subprocess.run(['bash', '-c', body], capture_output=True, text=True, env=env, timeout=30)

    def test_finds_the_other_esp_with_bootmgfw_and_mounts_read_only(self):
        self.esp('sda1', False); self.esp('nvme0n1p1', True)
        r = self.detect(f'/dev/sda1 {self.ESP} own-guid\n/dev/nvme0n1p1 {self.ESP} win-guid\n/dev/nvme0n1p3 0fc63daf-8483-4772-8e79-3d69d8477de4 data')
        self.assertEqual((r.returncode, r.stdout.strip()), (0, 'win-guid'), r.stderr)
        log = (self.t / 'log').read_text()
        self.assertIn('ro,noexec,nosuid,nodev /dev/nvme0n1p1', log)
        self.assertNotIn('/dev/sda1', log)  # our own ESP is never touched
        self.assertNotIn('/dev/nvme0n1p3', log)  # non-ESP partitions are never mounted

    def test_nothing_found_prints_nothing(self):
        self.esp('nvme0n1p1', False)
        r = self.detect(f'/dev/nvme0n1p1 {self.ESP} other-guid')
        self.assertEqual((r.returncode, r.stdout), (0, ''))

    def test_two_windows_esps_are_ambiguous_so_nothing_is_added(self):
        self.esp('nvme0n1p1', True); self.esp('nvme1n1p1', True)
        r = self.detect(f'/dev/nvme0n1p1 {self.ESP} a-guid\n/dev/nvme1n1p1 {self.ESP} b-guid')
        self.assertEqual((r.returncode, r.stdout), (0, ''))


if __name__ == '__main__':
    unittest.main()
