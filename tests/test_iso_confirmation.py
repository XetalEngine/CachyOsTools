"""Exercise installer confirmations without accessing or modifying any disks."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

INSTALLER = Path(__file__).resolve().parents[1] / 'iso/installer.sh'


class ConfirmationTests(unittest.TestCase):
    def confirm(self, first=0, second=0, response=None):
        code = r'''
source "$1"
records=$2
first=$3
second=$4
calls=0
lsblk() { printf '20G   Test Disk\n'; }
clear() { :; }
dialog() {
    calls=$((calls + 1))
    printf '%s\0' "$@" > "$records/$calls"
    if (( calls == 1 )); then return "$first"; else return "$second"; fi
}
installer_confirm_disk /dev/test || exit "$?"
echo CONFIRMED
'''
        with tempfile.TemporaryDirectory(prefix='iso-confirm-') as tmp:
            args = ['bash', '-c', code, 'test', str(INSTALLER), tmp,
                    str(first), str(second)]
            if response is None:
                # A terminal selects the TUI path. No characters are supplied.
                master, slave = os.openpty()
                try:
                    result = subprocess.run(args, stdin=slave, capture_output=True,
                                            text=True, timeout=5)
                finally:
                    os.close(slave)
                    os.close(master)
            else:
                result = subprocess.run(args, input=response, capture_output=True,
                                        text=True, timeout=5)
            dialogs = [path.read_text().split('\0')[:-1]
                       for path in sorted(Path(tmp).iterdir())]
        return result, dialogs

    def test_two_yes_answers_confirm_without_typing(self):
        result, dialogs = self.confirm()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('CONFIRMED', result.stdout)
        self.assertEqual(len(dialogs), 2)
        for args, title in zip(dialogs, (' Confirm Target ', ' FINAL WARNING ')):
            self.assertIn('--defaultno', args)
            self.assertIn('--colors', args)
            self.assertEqual(args[args.index('--backtitle') + 1],
                             'XETAL ENGINE - System Installer')
            self.assertEqual(args[args.index('--title') + 1], title)
            self.assertIn('/dev/test', args[args.index('--yesno') + 1])
        self.assertEqual(dialogs[0][-2:], ['14', '66'])
        self.assertEqual(dialogs[1][-2:], ['9', '56'])

    def test_no_escape_or_error_at_first_warning_cancels_immediately(self):
        for status in (1, 255, 2):
            with self.subTest(status=status):
                result, dialogs = self.confirm(first=status)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(len(dialogs), 1)
                self.assertIn('Installation cancelled.', result.stdout)
                self.assertNotIn('CONFIRMED', result.stdout)

    def test_no_escape_or_error_at_final_warning_cancels(self):
        for status in (1, 255, 2):
            with self.subTest(status=status):
                result, dialogs = self.confirm(second=status)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(len(dialogs), 2)
                self.assertIn('Installation cancelled.', result.stdout)
                self.assertNotIn('CONFIRMED', result.stdout)

    def test_text_fallback_accepts_original_wipe_phrase(self):
        result, dialogs = self.confirm(response='WIPE\n')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(dialogs)
        self.assertIn("Type 'WIPE' to confirm:", result.stdout)
        self.assertIn('CONFIRMED', result.stdout)

    def test_text_fallback_rejects_other_answers_and_end_of_input(self):
        for response in ('\n', 'yes\n', 'ERASE /dev/test\n', ''):
            with self.subTest(response=response):
                result, dialogs = self.confirm(response=response)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(dialogs)
                self.assertNotIn('CONFIRMED', result.stdout)


if __name__ == '__main__':
    unittest.main()
