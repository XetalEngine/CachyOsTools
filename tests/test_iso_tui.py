"""Exercise the actual installer menus on a PTY, with no disk access."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]


class InstallerMenuTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='xetal-menu-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.clone = self.root / 'clone'
        self.clone.mkdir()
        self.efi = self.root / 'efi'
        self.efi.mkdir()
        self.records = self.root / 'records'
        self.records.mkdir()
        self.archive = self.clone / 'rootfs-snapshot.tar.zst'
        self.archive.write_bytes(b'Test snapshot payload')
        digest = hashlib.sha256(self.archive.read_bytes()).hexdigest()
        (self.clone / 'snapshot.sha256').write_text(f'{digest}  rootfs-snapshot.tar.zst\n')
        script = (REPO / 'iso/installer.sh').read_text()
        script = script.replace('/opt/clone', str(self.clone)).replace('/sys/firmware/efi', str(self.efi))
        script = script.replace('/sys/class/block', str(self.root/'block'))
        self.script = self.root / 'installer.sh'
        self.script.write_text(script)

    def run_tui(self, body='installer_home; echo "MODE=$INSTALL_MODE CHECK=$SNAPSHOT_CHECK"',
                choices=('wipe',), yes_status=0):
        (self.root / 'choices').write_text('\n'.join(choices) + '\n')
        (self.root / 'counter').write_text('0')
        code = r'''
source "$1"
fixture=$2
yes_status=$3
clear() { :; }
sha256sum() { echo CHECKSUM >> "$fixture/events"; command sha256sum "$@"; }
dialog() {
    local n choice
    n=$(<"$fixture/counter"); n=$((n+1))
    printf '%s' "$n" > "$fixture/counter"
    printf '%s\0' "$@" > "$fixture/records/$n"
    echo DIALOG >> "$fixture/events"
    case " $* " in
        *' --menu '*)
            IFS= read -r choice < "$fixture/choices" || return 1
            tail -n +2 "$fixture/choices" > "$fixture/choices.new"
            mv "$fixture/choices.new" "$fixture/choices"
            [[ -n $choice && $choice != cancel ]] || return 1
            printf '%s\n' "$choice" ;;
        *' --yesno '*) return "$yes_status" ;;
        *' --inputbox '*) echo 'UNEXPECTED TYPING PROMPT' >&2; return 2 ;;
    esac
    return 0
}
''' + body
        master, slave = os.openpty()
        try:
            result = subprocess.run(['bash', '-c', code, 'test', str(self.script), str(self.root), str(yes_status)],
                                    stdin=slave, capture_output=True, text=True, timeout=5)
        finally:
            os.close(slave)
            os.close(master)
        dialogs = [p.read_text().split('\0')[:-1]
                   for p in sorted(self.records.iterdir(), key=lambda p: int(p.name))]
        return result, dialogs

    @staticmethod
    def menu_items(dialog):
        return dialog[dialog.index('--menu') + 5:]

    def test_uefi_menu_order_default_and_contributor_credit(self):
        result, dialogs = self.run_tui()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.menu_items(dialogs[0])[::2], ['wipe', 'verify', 'free', 'parts'])
        self.assertEqual(dialogs[0][dialogs[0].index('--default-item')+1], 'wipe')
        descriptions = self.menu_items(dialogs[0])[1::2]
        self.assertIn('devvz (PR #7)', descriptions[2])
        self.assertIn('devvz (PR #7)', descriptions[3])
        self.assertNotIn('CHECKSUM', (self.root / 'events').read_text())

    def test_bios_shows_original_restore_and_optional_check_only(self):
        self.efi.rmdir()
        result, dialogs = self.run_tui()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.menu_items(dialogs[0])[::2], ['wipe', 'verify'])

    def test_menu_is_shown_before_manifest_or_restore_preflight(self):
        # There is intentionally no common.sh or snapshot.meta. Cancelling the
        # first menu must work without opening either file or hashing anything.
        result, dialogs = self.run_tui('installer_main', choices=('cancel',))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(len(dialogs), 1)
        self.assertNotIn('No such file', result.stderr)
        self.assertNotIn('CHECKSUM', (self.root / 'events').read_text())

    def test_check_runs_only_when_selected_and_returns_to_home(self):
        result, dialogs = self.run_tui(choices=('verify', 'wipe'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('MODE=wipe CHECK=passed', result.stdout)
        events = (self.root / 'events').read_text().splitlines()
        self.assertEqual(events.count('CHECKSUM'), 1)
        self.assertEqual(events[0], 'DIALOG')
        menus = [d for d in dialogs if '--menu' in d]
        self.assertEqual(len(menus), 2)
        self.assertIn('Snapshot check: passed', menus[1][menus[1].index('--menu')+1])

    def test_failed_check_blocks_installation_but_keeps_menu_available(self):
        self.archive.write_bytes(b'corrupt snapshot')
        result, dialogs = self.run_tui(choices=('verify', 'wipe', 'cancel'))
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn('MODE=', result.stdout)
        self.assertTrue(any('Snapshot integrity FAILED' in ' '.join(d) for d in dialogs))
        self.assertTrue(any('Run a successful integrity check' in ' '.join(d) for d in dialogs))
        self.assertEqual(sum('--menu' in d for d in dialogs), 3)

    def test_missing_checksum_does_not_prevent_menu_or_optional_skip(self):
        (self.clone / 'snapshot.sha256').unlink()
        result, _ = self.run_tui()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('MODE=wipe CHECK=not checked', result.stdout)

    def test_partition_confirmation_needs_only_two_buttons_even_with_existing_data(self):
        result, dialogs = self.run_tui(
            "installer_confirm_plan 'ROOT /dev/test2 and EFI /dev/test1 will be erased' 'Proceed?' /dev/test2; echo CONFIRMED")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('CONFIRMED', result.stdout)
        self.assertEqual(len(dialogs), 2)
        for d in dialogs:
            self.assertIn('--yesno', d)
            self.assertIn('--defaultno', d)
            self.assertNotIn('--inputbox', d)

    def test_no_escape_and_dialog_error_cancel_partition_confirmation(self):
        for status in (1, 255, 2):
            result, _ = self.run_tui("installer_confirm_plan summary danger; echo CONFIRMED", yes_status=status)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn('CONFIRMED', result.stdout)

    def test_disk_description_lists_labels_and_removes_control_sequences(self):
        code = r'''
lsblk() {
    case "$*" in
        *SIZE,MODEL*) printf '500G Test SSD\n' ;;
        *LABEL,PARTLABEL*) printf 'Windows  Basic data\nBackup  Archive\nBackup  Archive\n\033[31mUnsafe\\Z1\n' ;;
    esac
}
installer_device_description /dev/test
'''
        result, _ = self.run_tui(code)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('Windows  Basic data', result.stdout)
        self.assertIn('Backup  Archive', result.stdout)
        self.assertEqual(result.stdout.count('Backup'), 1)
        self.assertNotIn('\x1b', result.stdout)
        self.assertNotIn('\\Z1', result.stdout)

    def test_firmware_entry_is_appended_without_deleting_or_reordering_existing_entries(self):
        (self.root/'block/mock2').mkdir(parents=True)
        (self.root/'block/mock2/partition').write_text('2\n')
        result, _ = self.run_tui(r'''
iso_disk_ancestors() { echo /dev/mock; }
efibootmgr() {
    if (( $# == 0 )); then printf 'BootOrder: 0001,0005\n';
    else printf '%s\0' "$@" > "$fixture/efi-args"; fi
}
installer_register_efi /dev/mock2
echo "REGISTERED=$EFI_REGISTERED"
''')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('REGISTERED=1', result.stdout)
        args = (self.root/'efi-args').read_text().split('\0')[:-1]
        self.assertEqual(args, ['--create', '--index', '2', '--disk', '/dev/mock', '--part', '2',
                                '--label', 'Xetal Engine', '--loader', '\\EFI\\BOOT\\BOOTX64.EFI'])

    def test_firmware_failure_does_not_claim_an_entry_was_created(self):
        (self.root/'block/mock2').mkdir(parents=True)
        (self.root/'block/mock2/partition').write_text('2\n')
        result, _ = self.run_tui(r'''
iso_disk_ancestors() { echo /dev/mock; }
efibootmgr() { return 1; }
installer_register_efi /dev/mock2 || echo "FAILED REGISTERED=$EFI_REGISTERED"
''')
        self.assertIn('FAILED REGISTERED=0', result.stdout)


if __name__ == '__main__':
    unittest.main()
