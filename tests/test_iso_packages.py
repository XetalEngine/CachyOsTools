"""Package preparation tests: isolated repositories, pinning, and offline completeness."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[1]
COMMON = REPO / 'iso/common.sh'
FAKE_PACMAN = r'''#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
args = sys.argv[1:]
state = json.loads(Path(os.environ['TEST_STATE']).read_text())
with open(os.environ['TEST_CALLS'], 'a') as f: f.write(json.dumps(args) + '\n')
run = Path(os.environ['RUN'])
db = Path(args[args.index('--dbpath') + 1])
assert db == run / 'db' and not list((db / 'local').iterdir())
assert Path(args[args.index('--config') + 1]) == Path(os.environ['PROFILE']) / 'pacman.conf'
assert Path(args[args.index('--logfile') + 1]).is_relative_to(run)
assert '--noconfirm' in args
if '-Sy' in args:
    if state['refresh_fails']: sys.exit(1)
    (db / 'sync').mkdir(exist_ok=True)
    for repo in ['core', 'extra']: (db / 'sync' / (repo + '.db')).write_text('database')
    sys.exit(0)
if '-Si' in args: sys.exit(0 if args[-1] in state['available'] else 1)
assert '-Sp' in args or '-Sw' in args, 'Installing packages on host is forbidden'
targets = args[args.index('--')+1:]
assert targets
missing = [p for p in targets if p not in state['available']]
for pkg in missing: print('error: target not found: ' + pkg, file=sys.stderr)
if missing: sys.exit(1)
if state['dependency_fails']: print('error: could not satisfy dependencies', file=sys.stderr); sys.exit(1)
if 'broadcom-wl-dkms' in targets: targets.append('dkms')
files = [p + '-1-1-x86_64.pkg.tar.zst' for p in targets]
if '-Sp' in args:
    print('\n'.join(files))
else:
    if state['signature_fails']: print('error: invalid package signature', file=sys.stderr); sys.exit(1)
    for name in files: (run / 'pkg' / name).write_text('package')
'''

class PackageTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='iso packages ')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.run = self.root / 'run'
        self.profile = self.run / 'profile'
        self.profile.mkdir(parents=True)
        self.packages = self.profile / 'packages.x86_64'
        self.calls_file = self.root / 'calls'
        self.bin = self.root / 'bin'
        self.bin.mkdir()
        (self.bin / 'pacman').write_text(FAKE_PACMAN)
        (self.bin / 'pacman').chmod(0o755)
        self.cache = self.root / 'offline-cache'
        self.marker = self.root / 'snapshot-started'

    def run_check(self, packages='linux\nbroadcom-wl\nrsync\n', *, offline=False,
                  available=None, refresh_fails=False, dependency_fails=False,
                  signature_fails=False, omit_archive=None, omit_db=False):
        available = available or ['linux', 'broadcom-wl-dkms', 'linux-headers', 'rsync', 'dkms']
        self.packages.write_text(packages)
        state = self.root / 'state'
        state.write_text(json.dumps(dict(available=available, refresh_fails=refresh_fails,
            dependency_fails=dependency_fails, signature_fails=signature_fails)))
        if offline:
            sync = self.cache / 'offline-packages/sync'
            pkg = self.cache / 'offline-packages/pkg'
            pkg.mkdir(parents=True)
            if not omit_db:
                sync.mkdir()
                for name in ('core', 'extra'): (sync / (name+'.db')).write_text('db')
            for name in available:
                if name != omit_archive: (pkg / (name+'-1-1-x86_64.pkg.tar.zst')).write_text('package')
        env = dict(os.environ, RUN=str(self.run), PROFILE=str(self.profile),
            OFFLINE_CACHE_DIR=str(self.cache), ISO_OFFLINE=str(int(offline)),
            TEST_STATE=str(state), TEST_CALLS=str(self.calls_file),
            TEST_MARKER=str(self.marker), PATH=str(self.bin)+os.pathsep+os.environ['PATH'])
        result = subprocess.run(['bash', '-c', '''set -euo pipefail
source "$1"
run_sudo() { "$@"; }
iso_write_pacman_conf "$PROFILE/pacman.conf" "$RUN/pkg" "$RUN/gnupg" 'https://geo.mirror.pkgbuild.com/$repo/os/$arch'
iso_prepare_packages
touch "$TEST_MARKER"
''', 'test', str(COMMON)], env=env, capture_output=True, text=True, timeout=15)
        return result

    def calls(self):
        return [json.loads(x) for x in self.calls_file.read_text().splitlines()] if self.calls_file.exists() else []

    def passed(self, result):
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertTrue(self.marker.exists())
        conf = (self.profile/'pacman.conf').read_text()
        self.assertIn('file://', conf)
        self.assertNotIn('https://', conf)
        self.assertIn('SigLevel = Required DatabaseOptional', conf)
        self.assertNotIn('/etc/pacman', conf)

    def failed(self, result):
        self.assertNotEqual(result.returncode, 0, result.stdout+result.stderr)
        self.assertFalse(self.marker.exists())

    def test_online_replaces_broadcom_and_pins_dependencies(self):
        self.passed(self.run_check())
        self.assertEqual(self.packages.read_text().splitlines(), ['linux','broadcom-wl-dkms','rsync','linux-headers'])
        self.assertIn('dkms-1-1-x86_64.pkg.tar.zst', (self.run/'package-files.txt').read_text())
        self.assertIn('-Sy', self.calls()[0])
        self.assertIn('-Sw', self.calls()[-1])

    def test_updated_profile_adds_headers_without_duplicates(self):
        self.passed(self.run_check('# drivers\n linux \nbroadcom-wl # old\nbroadcom-wl-dkms\nlinux-headers\nlinux-headers\n'))
        self.assertEqual(self.packages.read_text().splitlines(), ['linux','broadcom-wl-dkms','linux-headers'])

    def test_no_driver_is_added_when_not_requested(self):
        self.passed(self.run_check('linux\nrsync\n'))
        self.assertEqual(self.packages.read_text(), 'linux\nrsync\n')

    def test_unknown_packages_are_not_silently_removed(self):
        result = self.run_check('linux\nmissing-tool\n')
        self.failed(result)
        self.assertIn('target not found: missing-tool', result.stderr)
        self.assertIn('missing-tool', self.packages.read_text())

    def test_missing_headers_stop_before_snapshot(self):
        result = self.run_check(available=['linux','broadcom-wl-dkms','rsync'])
        self.failed(result)
        self.assertIn('linux-headers', result.stderr)

    def test_dependency_failure_stops_before_snapshot(self):
        self.failed(self.run_check(dependency_fails=True))

    def test_bad_signatures_stop_before_snapshot(self):
        result = self.run_check(signature_fails=True)
        self.failed(result)
        self.assertIn('invalid package signature', result.stderr)

    def test_refresh_failure_stops_before_snapshot(self):
        self.failed(self.run_check(refresh_fails=True))
        self.assertEqual(len(self.calls()), 1)

    def test_current_offline_archive_uses_no_network_refresh(self):
        self.passed(self.run_check(offline=True))
        self.assertTrue(all('-Sy' not in x for x in self.calls()))

    def test_legacy_offline_archive_keeps_available_driver(self):
        self.passed(self.run_check(offline=True, available=['linux','rsync','broadcom-wl']))
        self.assertIn('broadcom-wl\n', self.packages.read_text())
        self.assertNotIn('linux-headers', self.packages.read_text())

    def test_missing_offline_dependency_stops_before_snapshot(self):
        result = self.run_check(offline=True, omit_archive='dkms')
        self.failed(result)
        self.assertIn('Missing offline package: dkms', result.stderr)
        self.assertTrue(all('-Sw' not in x for x in self.calls()))

    def test_missing_offline_databases_fail(self):
        self.failed(self.run_check(offline=True, omit_db=True))
        self.assertEqual(self.calls(), [])

    def test_empty_profile_is_rejected(self):
        self.failed(self.run_check('# no packages\n'))

if __name__ == '__main__': unittest.main()
