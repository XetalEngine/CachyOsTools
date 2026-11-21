"""Appearance fixtures: no host boot writes, authentication or mounted drives."""
import base64
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import grub_appearance as appearance
import grub_editor as grub


CONFIG = '''set default=1
set timeout=7
set theme=/old-theme/theme.txt
menuentry 'Linux' --id=linux {
  search --fs-uuid --set=root 1234-abcd
  linux /vmlinuz root=UUID=1234-abcd quiet
  initrd /initramfs.img
}
menuentry 'Firmware' { fwsetup; }
'''


class AppearanceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='cachyostools-grub-appearance-')
        self.addCleanup(temporary.cleanup); self.root = Path(temporary.name)
        (self.root/'boot/grub').mkdir(parents=True); (self.root/'etc/grub.d').mkdir(parents=True)
        self.config = self.root/'boot/grub/grub.cfg'; self.config.write_text(CONFIG)
        self.store = appearance.AppearanceStore(self.root, shutil.which('grub-script-check') or '/bin/true')
        from PIL import Image
        picture = self.root/'wallpaper.png'
        Image.new('RGB', (64, 36), '#173e34').save(picture)
        self.options = {**appearance.presets()[0]['options'], 'background': str(picture)}
        self.real_command = grub.command

    def command(self, args, **kwargs):
        if Path(args[0]).name in ('grub-probe', 'grub2-probe'): return b'1234-abcd\n'
        if Path(args[0]).name in ('grub-mkrelpath', 'grub2-mkrelpath'):
            return ('/'+str(Path(args[-1]).relative_to(self.root))+'\n').encode()
        return self.real_command(args, **kwargs)

    def apply(self, **overrides):
        with self.store.locked(), patch.object(grub, 'command', side_effect=self.command):
            plan = self.store.plan_apply({**self.options, **overrides}, self.store.revision(),
                grub.digest(self.config.read_bytes()), prepare_fn=appearance.prepare)
            return self.store.commit_plan(plan)

    def plan(self):
        with self.store.locked(), patch.object(grub, 'command', side_effect=self.command):
            return self.store.plan_apply(self.options, self.store.revision(), grub.digest(self.config.read_bytes()),
                                         prepare_fn=appearance.prepare)

    def test_review_creates_exact_backups_before_any_boot_file_changes(self):
        before = self.config.read_bytes(); plan = self.plan(); review = plan['review']
        self.assertEqual(self.config.read_bytes(), before)
        self.assertFalse(self.store.generator.exists()); self.assertFalse(self.store.assets.exists())
        self.assertFalse((self.store.state/'last.json').exists())
        rows = {row['path']: row for row in review['files']}
        self.assertEqual(Path(rows[str(self.config)]['backup']).read_bytes(), before)
        self.assertEqual(rows[str(self.store.generator)]['change'], 'Create')
        self.assertEqual(rows[str(self.store.generator)]['backup'], '')
        self.assertEqual(set(review['created']), {str(plan['assets']['path']/n) for n in plan['assets']['files']})
        self.assertIn('Take a picture of the screen. Here are your backups.', review['report'])
        self.assertIn('Recovery Instructions:', review['instructions'])
        self.assertIn('/mnt/boot', review['instructions'])
        self.assertEqual(Path(review['recovery_file']).read_text(), review['report'])
        backup = Path(plan['record']['backup'])
        self.assertEqual(subprocess.run(['sh', '-n', backup/'restore.sh']).returncode, 0)
        with self.store.locked(): returned = self.store.commit_plan(plan)
        self.assertEqual(returned, str(backup))
        self.assertEqual(json.loads((self.store.state/'last.json').read_text())['backup'], str(backup))
        self.assertEqual(len(list((self.store.state/'backups').iterdir())), 1)

    def test_reviewed_backup_and_current_file_tampering_block_apply(self):
        for what in ('config', 'backup', 'mode'):
            with self.subTest(what=what):
                plan = self.plan(); original = self.config.read_bytes()
                if what == 'config': self.config.write_text(CONFIG+'# changed while reviewing\n')
                elif what == 'mode': self.config.chmod(0o600)
                else: (Path(plan['record']['backup'])/'config').write_text('tampered backup')
                current = self.config.read_bytes()
                with self.store.locked(), self.assertRaisesRegex(RuntimeError, 'changed|backup'):
                    self.store.commit_plan(plan)
                self.assertEqual(self.config.read_bytes(), current); self.assertFalse(self.store.generator.exists())
                self.config.write_bytes(original); self.config.chmod(0o644)

    def test_recovery_script_restores_all_existing_files_and_permissions(self):
        self.apply(); original = {key: path.read_bytes() for key, path in self.store.files().items()}
        history = (self.store.state/'last.json').read_bytes()
        plan = self.plan(); backup = Path(plan['record']['backup'])
        self.assertEqual((backup/'undo-history.json').read_bytes(), history)
        with self.store.locked(): self.store.commit_plan(plan)
        subprocess.run(['sh', str(backup/'restore.sh')], check=True, capture_output=True)
        for key, path in self.store.files().items():
            self.assertEqual(path.read_bytes(), original[key])
            self.assertEqual(path.stat().st_mode & 0o777, plan['record']['before'][key]['mode'])
        self.assertEqual((self.store.state/'last.json').read_bytes(), history)

    def test_live_usb_recovery_uses_mounted_root_and_verifies_all_backups_first(self):
        plan = self.plan(); backup = Path(plan['record']['backup'])
        with self.store.locked(): self.store.commit_plan(plan)
        # The fixture itself is the installed system; a second prefix simulates
        # a live USB's mount point, without accessing /boot on the test machine.
        mount = self.root/'live'; mounted_config = mount/str(self.config).lstrip('/')
        mounted_config.parent.mkdir(parents=True); mounted_config.write_bytes(self.config.read_bytes())
        mounted_gen = mount/str(self.store.generator).lstrip('/')
        mounted_gen.parent.mkdir(parents=True); mounted_gen.write_bytes(self.store.generator.read_bytes())
        (backup/'config').write_text('corrupted')
        failed = subprocess.run(['sh', str(backup/'restore.sh'), str(mount)], capture_output=True)
        self.assertNotEqual(failed.returncode, 0); self.assertTrue(mounted_gen.exists())
        (backup/'config').write_text(CONFIG)
        subprocess.run(['sh', str(backup/'restore.sh'), str(mount)], check=True, capture_output=True)
        self.assertEqual(mounted_config.read_text(), CONFIG); self.assertFalse(mounted_gen.exists())

    def test_system_and_undo_also_prepare_backups_before_changes(self):
        self.apply(); old = self.config.read_bytes()
        with self.store.locked():
            plan = self.store.plan_system(self.store.revision(), grub.digest(old))
            self.assertEqual(self.config.read_bytes(), old); self.assertTrue(self.store.generator.exists())
            self.assertEqual((Path(plan['record']['backup'])/'generator').read_bytes(), self.store.generator.read_bytes())
            self.store.commit_plan(plan)
            undo = self.store.plan_undo(self.store.revision())
            self.assertEqual(self.config.read_text(), CONFIG)
            self.store.commit_plan(undo)
        self.assertEqual(self.config.read_bytes(), old)

    def test_authenticated_load_reads_existing_menu_and_flags_without_changing_design(self):
        self.apply(heading='Already applied')
        original = {str(p): p.read_bytes() for p in [*self.store.files().values(), self.store.state/'last.json']}
        backups = set((self.store.state/'backups').iterdir())
        session = grub.Session.__new__(grub.Session); session.store = grub.Store(self.root); session.appearance_plan = None
        with patch.object(appearance, 'AppearanceStore', return_value=self.store), patch.object(grub, 'client_alive'):
            result = session.handle({'action': 'appearance-load', 'menu_revision': ''})
        self.assertEqual(result['menu_revision'], grub.digest(self.config.read_bytes()))
        self.assertEqual([e['name'] for e in result['current_entries']], ['Linux', 'Firmware'])
        self.assertTrue(result['appearance']['active']); self.assertTrue(result['appearance']['can_undo'])
        self.assertNotIn('options', result['appearance'])  # The GUI keeps the user's new draft.
        self.assertIsNone(session.appearance_plan)
        for name, contents in original.items(): self.assertEqual(Path(name).read_bytes(), contents)
        self.assertEqual(set((self.store.state/'backups').iterdir()), backups)

    def test_session_requires_review_and_single_use_unexpired_confirmation(self):
        session = grub.Session.__new__(grub.Session); session.store = grub.Store(self.root); session.appearance_plan = None
        request = {'action': 'appearance-review', 'operation': 'apply', 'options': self.options,
                   'menu_revision': grub.digest(self.config.read_bytes())}
        with patch.object(appearance, 'AppearanceStore', return_value=self.store), \
             patch.object(grub, 'client_alive'), patch.object(grub, 'emit'), \
             patch.object(grub, 'command', side_effect=self.command):
            result = session.handle(request); token = result['appearance_review']['token']
            self.assertEqual(self.config.read_text(), CONFIG)
            session.handle({**request, 'action': 'appearance-cancel', 'token': token})
            with self.assertRaisesRegex(RuntimeError, 'expired'):
                session.handle({**request, 'action': 'appearance-confirm', 'token': token})
            self.assertEqual(self.config.read_text(), CONFIG)
            with self.assertRaisesRegex(ValueError, 'Review'):
                session.handle({**request, 'action': 'appearance-apply'})
            result = session.handle(request); token = result['appearance_review']['token']
            session.appearance_plan['created_at'] -= 301
            with self.assertRaisesRegex(RuntimeError, 'expired'):
                session.handle({**request, 'action': 'appearance-confirm', 'token': token})
            self.assertEqual(self.config.read_text(), CONFIG)
            result = session.handle(request); token = result['appearance_review']['token']
            result = session.handle({**request, 'action': 'appearance-confirm', 'token': token})
            self.assertTrue(result['appearance']['active'])
            with self.assertRaisesRegex(RuntimeError, 'expired'):
                session.handle({'action': 'appearance-confirm', 'token': token, 'menu_revision': result['menu_revision']})

    def test_presets_and_custom_options_render_without_touching_boot_files(self):
        for preset in appearance.presets():
            with self.subTest(theme=preset['id']):
                value = appearance.preview(preset['options'], ['My system', 'Firmware settings'])
                self.assertTrue(base64.b64decode(value['image']).startswith(b'\x89PNG'))
                self.assertEqual(value['background_sha'], '')
        self.assertEqual(self.config.read_text(), CONFIG)
        self.assertFalse(self.store.state.exists())
        for position in ('left', 'center', 'right'):
            value = appearance.preview({**self.options, 'background': '', 'layout': position,
                                        'heading': 'A "quoted" title', 'hints': False})
            self.assertEqual(value['background_sha'], '')
            text = appearance.theme_text({**self.options, 'layout': position, 'heading': 'A "quoted" title'})
            self.assertIn('A \\"quoted\\" title', text)

    def test_apply_preserves_every_boot_entry_and_survives_generator_use(self):
        self.apply(heading='My boot screen')
        current = self.config.read_text()
        self.assertEqual(appearance.replace_appearance(current, ''), CONFIG)
        self.assertEqual([(e['name'], e['commands']) for e in grub.menu_entries(current)],
                         [(e['name'], e['commands']) for e in grub.menu_entries(CONFIG)])
        block = current[len(CONFIG):]
        self.assertNotIn('--set=root', block); self.assertNotIn('set default=', block); self.assertNotIn('set timeout=', block)
        self.assertIn('--set=cachyostools_theme_device', block)
        self.assertIn('insmod gfxterm_background', block)
        self.assertIn('background_image -m stretch ($cachyostools_theme_device)', block)
        self.assertIn('set color_normal=white/black', block)
        self.assertEqual(subprocess.check_output([str(self.store.generator)]).decode(), block)
        state = self.store.status(); self.assertTrue(state['active']); self.assertTrue(state['can_undo'])
        source = Path(state['options']['background']); self.assertTrue(source.is_file())
        self.assertTrue((source.parent/'theme.txt').is_file())
        self.assertIn('My boot screen', (source.parent/'theme.txt').read_text())
        theme = (source.parent/'theme.txt').read_text()
        for setting in ('terminal-left: "0"', 'terminal-top: "0"',
                        'terminal-width: "100%"', 'terminal-height: "100%"'):
            self.assertIn(setting, theme)
        self.assertFalse((self.root/'etc/default/grub').exists())

    def test_reapply_then_undo_retains_old_design_assets(self):
        self.apply(); first = self.config.read_bytes(); original_source = self.store.status()['options']['background']
        self.apply(layout='right', size=28)
        self.assertEqual(self.config.read_text().count(appearance.BEGIN), 1)
        with self.store.locked(): self.store.commit_plan(self.store.plan_undo(self.store.revision()))
        self.assertEqual(self.config.read_bytes(), first); self.assertTrue(Path(original_source).is_file())
        self.assertEqual(self.store.status()['options']['layout'], 'left')

    def test_system_appearance_and_undo_keep_original_theme(self):
        self.apply(); applied = self.config.read_bytes()
        with self.store.locked():
            self.store.commit_plan(self.store.plan_system(self.store.revision(), grub.digest(applied)))
        self.assertEqual(self.config.read_text(), CONFIG); self.assertFalse(self.store.generator.exists())
        with self.store.locked(): self.store.commit_plan(self.store.plan_undo(self.store.revision()))
        self.assertEqual(self.config.read_bytes(), applied); self.assertTrue(self.store.generator.exists())

    def test_external_menu_change_rejected_before_assets_are_prepared(self):
        revision = self.store.revision(); old = grub.digest(self.config.read_bytes())
        self.config.write_text(CONFIG+'# kernel update\n')
        with self.store.locked(), patch.object(appearance, 'prepare') as prepare:
            with self.assertRaisesRegex(RuntimeError, 'changed'):
                self.store.plan_apply(self.options, revision, old, prepare_fn=prepare)
            prepare.assert_not_called()
        self.assertFalse(self.store.assets.exists())

    def test_undo_does_not_overwrite_an_entry_edit(self):
        self.apply(); self.config.write_text(self.config.read_text()+'# another editor\n')
        current = self.config.read_bytes()
        with self.store.locked(), self.assertRaisesRegex(RuntimeError, 'Another tool'):
            self.store.commit_plan(self.store.plan_undo(self.store.revision()))
        self.assertEqual(self.config.read_bytes(), current)

    def test_failed_validation_and_publication_leave_original_menu(self):
        original = self.config.read_bytes()
        calls = 0
        checker = self.store.check
        def fail_candidate(text):
            nonlocal calls
            calls += 1
            if calls == 2: raise RuntimeError('candidate rejected')
            checker(text)
        with patch.object(self.store, 'check', side_effect=fail_candidate), self.assertRaisesRegex(RuntimeError, 'candidate'):
            self.apply()
        self.assertEqual(self.config.read_bytes(), original)
        self.assertFalse(self.store.generator.exists())
        self.assertFalse(self.store.assets.exists())
        atomic = grub.atomic
        def fail_config(path, data, mode=0o600):
            if path == self.config and appearance.BEGIN.encode() in data: raise OSError('write failed')
            return atomic(path, data, mode)
        with patch.object(grub, 'atomic', side_effect=fail_config), self.assertRaisesRegex(OSError, 'write failed'):
            self.apply()
        self.assertEqual(self.config.read_bytes(), original); self.assertFalse(self.store.generator.exists())
        self.assertFalse((self.store.state/'pending.json').exists())

    def test_interrupted_transaction_is_recovered(self):
        with self.store.locked():
            targets = self.store.unchanged_targets(); targets['config'] = ((CONFIG+'# pending\n').encode(), 0o644)
            targets['generator'] = (b'#!/bin/sh\n# pending\n', 0o755)
            record = self.store.snapshot(targets)
            grub.atomic(self.store.state/'pending.json', grub.encoded(record))
            grub.atomic(self.store.generator, targets['generator'][0], 0o755)
        with self.store.locked(): pass
        self.assertEqual(self.config.read_text(), CONFIG); self.assertFalse(self.store.generator.exists())

    def test_incomplete_assets_unknown_settings_and_changed_image_are_rejected(self):
        for value in ({'size': 200}, {'accent': 'red"; set root=bad'}, {'heading': 'bad\ntext'}, {'background': 'relative.png'}):
            with self.assertRaises(ValueError): appearance.normalize({**self.options, **value})
        with self.assertRaisesRegex(ValueError, 'changed'): appearance.prepare(self.options, 'wrong-image-hash')
        with self.store.locked(), self.assertRaisesRegex(ValueError, 'incomplete'):
            self.store.plan_apply(self.options, self.store.revision(), grub.digest(self.config.read_bytes()),
                prepare_fn=lambda *_: {'files': {}})
        self.assertEqual(self.config.read_text(), CONFIG)

    def test_forwarding_config_and_symlink_destination_rejected(self):
        self.config.write_text('configfile /real/menu/grub.cfg\n')
        with self.assertRaisesRegex(RuntimeError, 'forwarding'): self.apply()
        self.config.write_text(CONFIG)
        self.store.assets.parent.mkdir(); self.store.assets.symlink_to(self.root/'elsewhere')
        with self.assertRaisesRegex(RuntimeError, 'symbolic link'): self.apply()

    def test_damaged_or_nested_markers_are_not_silently_rewritten(self):
        with self.assertRaises(ValueError): appearance.replace_appearance(CONFIG+appearance.BEGIN+'\n', '')
        with self.assertRaises(ValueError): appearance.appearance_block('bad;halt', '/boot/themes/theme')
        with self.assertRaises(ValueError): appearance.appearance_block('1234-abcd', '/boot/$(evil)')

    def test_picture_preparation_drops_root_privileges(self):
        result = subprocess.CompletedProcess([], 0, b'{"files": {}}', b'')
        uid = os.getuid()
        if uid == 0: self.skipTest('This fixture expects the desktop test user.')
        with patch.object(os, 'geteuid', return_value=0), patch.dict(os.environ, {'PKEXEC_UID': str(uid)}), \
             patch.object(subprocess, 'run', return_value=result) as child:
            appearance.prepared_as_user(self.options, 'sha')
        self.assertEqual(child.call_args.kwargs['user'], uid)
        self.assertIn('group', child.call_args.kwargs)
        self.assertIn('extra_groups', child.call_args.kwargs)
        self.assertEqual(child.call_args.args[0][1], '-I')
        with patch.object(os, 'geteuid', return_value=0), patch.dict(os.environ, {'PKEXEC_UID': '0'}):
            with self.assertRaisesRegex(RuntimeError, 'desktop user'): appearance.prepared_as_user(self.options, '')


if __name__ == '__main__': unittest.main()
