"""Boot-menu fixtures only: no host mounts, sudo, firmware or boot writes."""
from contextlib import contextmanager
import json
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import grub_editor as grub


def pe(machine=0x8664):
    value = bytearray(1024); value[:2] = b'MZ'
    struct.pack_into('<I', value, 60, 128); value[128:132] = b'PE\0\0'
    struct.pack_into('<H', value, 132, machine); struct.pack_into('<H', value, 128+24+68, 10)
    return bytes(value)


def part(path='/dev/fixture1', fs='vfat', uuid='ABCD-1234'):
    return {'path':path, 'fstype':fs, 'uuid':uuid, 'partuuid':'a'*8+'-1111-2222-3333-'+'b'*12, 'type':'part', 'pttype':'gpt'}


def disk(partition):
    return {'path':'/dev/fixture', 'type':'disk', 'maj:min':'8:0', 'size':10**12,
            'serial':'fixture-123', 'children':[partition]}


class Fixtures(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='cachyostools-grub-test-')
        self.addCleanup(temporary.cleanup); self.root = Path(temporary.name)

    def file(self, name, data):
        path = self.root/name; path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data if isinstance(data, bytes) else data.encode()); return path


class DiscoveryTests(Fixtures):
    def test_inventory_includes_unmounted_drives_and_partition_properties(self):
        mounted = disk({**part(), 'mountpoints': ['/'], 'size': 10**9, 'label': 'System'})
        other = {**disk({**part('/dev/other1', 'ext4', '1234-5678'),
                        'mountpoints': [], 'size': 2*10**9, 'label': 'Other Linux'}),
                 'path': '/dev/other', 'size': 2*10**12}
        with patch.object(grub, 'command', return_value=json.dumps({'blockdevices': [other, mounted,
                {'path': '/dev/loop0', 'type': 'loop', 'size': 1024}]}).encode()):
            found = grub.inventory()
        self.assertEqual({d['path'] for d in found}, {'/dev/fixture', '/dev/other'})
        summary = grub.drive_summary(other)
        self.assertEqual(summary['mountpoints'], [])
        self.assertFalse(summary['current_system'])
        self.assertEqual(summary['volumes'][0]['label'], 'Other Linux')
        self.assertEqual(summary['volumes'][0]['fstype'], 'ext4')
        self.assertEqual(summary['volumes'][0]['uuid'], '1234-5678')
        self.assertTrue(grub.drive_summary(mounted)['current_system'])

    def test_uefi_arch_on_unmounted_linux_partition_is_detected(self):
        self.file('boot/grub/grub.cfg', "menuentry 'Arch' { linux /boot/vmlinuz-linux root=UUID=1234-5678; }\n")
        self.file('etc/os-release', 'PRETTY_NAME="Arch Linux"\n')
        partition = {**part(fs='ext4', uuid='1234-5678'), 'mountpoints': []}
        drive = disk(partition)
        @contextmanager
        def inspect(p):
            self.assertEqual(p['path'], partition['path']); yield self.root
        result = grub.scan_drive(drive, [drive], 'uefi', inspector=inspect)
        self.assertEqual(len(result['candidates']), 1)
        item = result['candidates'][0]
        self.assertEqual((item['title'], item['kind'], item['loader']),
                         ('Arch Linux', 'config', '/boot/grub/grub.cfg'))
        self.assertTrue(item['available'])
        current = grub.scan_drive(drive, [drive], 'uefi', inspector=inspect,
                                  live_config=(partition['uuid'], item['loader']))
        self.assertFalse(current['candidates'][0]['available'])

    def test_reserved_partition_with_stale_fat_signature_is_not_a_boot_choice(self):
        self.file('EFI/debian/shimx64.efi', pe())
        partition = {**part(), 'parttype': '21686148-6449-6e6f-744e-656564454649'}
        drive = disk(partition)
        def inspect(_): self.fail('A reserved BIOS area must not be mounted as a filesystem')
        self.assertEqual(grub.scan_drive(drive, [drive], 'uefi', inspector=inspect)['candidates'], [])

    def test_uefi_linux_menu_rejects_bios_only_boot_commands(self):
        self.file('boot/grub/grub.cfg', "menuentry 'Linux' {\n linux16 /vmlinuz\n}\n")
        item = grub.detect(self.root, part(fs='ext4'), 'uefi')[0]
        self.assertFalse(item['available']); self.assertIn('BIOS-only', item['reason'])

    def test_windows_and_linux_detected_without_partition_input(self):
        self.file('EFI/Microsoft/Boot/bootmgfw.efi',pe())
        self.file('EFI/ubuntu/shimx64.efi',pe())
        self.file('EFI/ubuntu/grubx64.efi',pe()+b'grub')
        self.file('EFI/BOOT/BOOTX64.EFI',pe())
        candidates = grub.detect(self.root,part(),'uefi','x86_64')
        self.assertEqual([c['title'] for c in candidates],['Windows Boot Manager','Ubuntu'])
        self.assertTrue(all(c['available'] for c in candidates))
        self.assertEqual(candidates[1]['loader'],'/EFI/ubuntu/shimx64.efi')

    def test_invalid_or_wrong_architecture_efi_is_never_offered(self):
        self.file('EFI/arch/grubx64.efi',b'MZ'+bytes(1000))
        self.file('EFI/debian/shimx64.efi',pe(0xaa64))
        self.assertEqual(grub.detect(self.root,part(),'uefi','x86_64'),[])

    def test_fallback_loader_and_firmware_mismatch(self):
        self.file('efi/boot/bootx64.efi',pe())
        item = grub.detect(self.root,part(),'bios','x86_64')[0]
        self.assertFalse(item['available']); self.assertIn('UEFI',item['reason'])
        self.assertTrue(grub.detect(self.root,part(),'uefi','x86_64')[0]['available'])

    def test_bios_linux_separate_boot_and_btrfs_subvolume(self):
        self.file('grub/grub.cfg',"menuentry 'Linux' { echo test; }\n")
        items = grub.detect(self.root,part(fs='ext4',uuid='a'*32),'bios')
        self.assertEqual(items[0]['loader'],'/grub/grub.cfg')
        self.file('@/boot/grub/grub.cfg',"menuentry 'Linux' { echo test; }\n")
        self.file('@/etc/os-release','PRETTY_NAME="CachyOS Linux"\n')
        items = grub.detect(self.root,part(fs='btrfs',uuid='a'*32),'bios')
        self.assertEqual(next(c for c in items if c['title']=='CachyOS Linux')['loader'],'/@/boot/grub/grub.cfg')

    def test_legacy_windows_requires_its_boot_manager_and_bcd_on_mbr(self):
        self.file('bootmgr',b'windows manager'); self.file('Boot/BCD',b'regf fixture')
        partition = part(fs='ntfs',uuid='F'*16)
        self.assertEqual(grub.detect(self.root,partition,'bios'),[])
        partition['pttype']='dos'
        self.assertEqual(grub.detect(self.root,partition,'bios')[0]['kind'],'legacy-windows')
        self.assertEqual(grub.detect(self.root,partition,'uefi'),[])

    def test_external_symlinks_cannot_supply_a_loader(self):
        with tempfile.TemporaryDirectory() as outside:
            external=Path(outside); (external/'arch').mkdir(); (external/'arch/grubx64.efi').write_bytes(pe())
            (self.root/'EFI').symlink_to(external)
            self.assertEqual(grub.detect(self.root,part(),'uefi','x86_64'),[])
            with self.assertRaises(ValueError): grub.small_file(self.root,'EFI/arch/grubx64.efi')

    def test_duplicate_uuids_and_current_loader_are_disabled(self):
        self.file('EFI/arch/grubx64.efi',pe()); partition=part(); drive=disk(partition)
        @contextmanager
        def inspect(_): yield self.root
        other=disk(part('/dev/other1')); other['path']='/dev/other'
        value=grub.scan_drive(drive,[drive,other],'uefi',inspector=inspect)
        self.assertFalse(value['candidates'][0]['available'])
        self.assertIn('same UUID',value['candidates'][0]['reason'])
        value=grub.scan_drive(drive,[drive],'uefi',inspector=inspect,
                              current=(partition['partuuid'],'/efi/arch/grubx64.efi'))
        self.assertIn('already starts',value['candidates'][0]['reason'])

    def test_inspection_is_read_only_and_cleans_its_mount(self):
        with patch.object(grub,'command') as call, patch.object(grub.tempfile,'TemporaryDirectory') as temp:
            temp.return_value.__enter__.return_value='/private/fixture'
            with self.assertRaisesRegex(RuntimeError,'fixture error'):
                with grub.inspect_partition(part(fs='ext4')): raise RuntimeError('fixture error')
            mount=call.call_args_list[0].args[0]
            self.assertIn('ro,nosuid,nodev,noexec,noload',mount)
            self.assertEqual(call.call_args_list[-1].args[0],['umount','--','/private/fixture'])

    def test_current_bios_menu_is_not_offered_as_another_system(self):
        self.file('boot/grub/grub.cfg',"menuentry 'Linux' { echo test; }\n")
        partition=part(fs='ext4',uuid='a'*32);drive=disk(partition)
        @contextmanager
        def inspect(_): yield self.root
        result=grub.scan_drive(drive,[drive],'bios',inspector=inspect,
                               live_config=(partition['uuid'],'/boot/grub/grub.cfg'))
        self.assertFalse(result['candidates'][0]['available'])
        self.assertIn('already used',result['candidates'][0]['reason'])

    def test_efi_current_entry_handles_both_efibootmgr_path_formats(self):
        guid=part()['partuuid']
        for loader in ('File(\\EFI\\ArchLinux\\grubx64.efi)', '\\EFI\\ArchLinux\\grubx64.efi'):
            output=f'BootCurrent: 0001\nBoot0001* ArchLinux\tHD(1,GPT,{guid},0x800,0x100000)/{loader}\n'.encode()
            with patch.object(grub,'command',return_value=output):
                self.assertEqual(grub.current_efi(),(guid,'/efi/archlinux/grubx64.efi'))

    def test_mounted_esp_uses_a_private_read_only_bind(self):
        partition={**part(),'maj:min':'8:1','mountpoints':['/boot/efi']}
        with patch.object(grub,'command') as call,patch.object(grub.tempfile,'TemporaryDirectory') as temp:
            temp.return_value.__enter__.return_value='/private/fixture'
            call.return_value=json.dumps({'filesystems':[{'target':'/boot/efi','fsroot':'/','maj:min':'8:1'}]}).encode()
            with grub.inspect_partition(partition):pass
            calls=[c.args[0] for c in call.call_args_list]
            self.assertEqual(calls[1],['mount','--bind','--','/boot/efi','/private/fixture'])
            self.assertEqual(calls[2],['mount','-o','remount,bind,ro,nosuid,nodev,noexec','--','/private/fixture'])
            self.assertEqual(calls[3],['umount','--','/private/fixture'])

    def test_shared_efi_is_associated_only_with_the_selected_system(self):
        import uuid
        windows=part('/dev/fixture2','ntfs','F'*16)
        windows['parttype']='ebd0a0a2-b9e5-4433-87c0-68b6b72699c7'
        selected=disk(windows)
        esp=part('/dev/other1');esp['partuuid']='c'*8+'-1111-2222-3333-'+'d'*12
        other=disk(esp);other['path']='/dev/other'
        self.file('EFI/Microsoft/Boot/bootmgfw.efi',pe())
        bcd=self.file('EFI/Microsoft/Boot/BCD',b'regf'+bytes(128)+uuid.UUID(windows['partuuid']).bytes_le)
        @contextmanager
        def inspect(p):
            self.assertEqual(p['path'],esp['path']);yield self.root
        result=grub.scan_drive(selected,[selected,other],'uefi',inspector=inspect)
        self.assertEqual(len(result['candidates']),1)
        self.assertEqual(result['candidates'][0]['drive'],selected['path'])
        self.assertEqual(result['candidates'][0]['source_drive'],other['path'])
        bcd.write_bytes(b'regf'+bytes(128))
        self.assertEqual(grub.scan_drive(selected,[selected,other],'uefi',inspector=inspect)['candidates'],[])
        self.file('EFI/ubuntu/shimx64.efi',pe())
        self.file('EFI/ubuntu/grub.cfg','search --fs-uuid '+windows['uuid']+' --set=root\n')
        result=grub.scan_drive(selected,[selected,other],'uefi',inspector=inspect)
        self.assertEqual([item['title'] for item in result['candidates']],['Ubuntu'])

    def test_disconnected_or_replaced_drive_is_rejected(self):
        drive=disk(part()); token=grub.drive_token(drive)
        with self.assertRaises(RuntimeError): grub.selected_drive([],drive['path'],token)
        changed={**drive,'serial':'another-drive'}
        with self.assertRaises(RuntimeError): grub.selected_drive([changed],drive['path'],token)
        with patch.object(grub.os,'stat',return_value=os.stat(self.root)):
            with self.assertRaises(RuntimeError): grub.selected_drive([drive],drive['path'],token)


@unittest.skipUnless(shutil.which('grub-script-check') or shutil.which('grub2-script-check'), 'GRUB syntax checker is needed')
class TransactionTests(Fixtures):
    def setUp(self):
        super().setUp()
        self.original="set default=0\nset timeout=10\n# Keep my theme and settings\nmenuentry 'Existing Linux' { echo original; }\n"
        self.file('boot/grub/grub.cfg',self.original).chmod(0o600)
        (self.root/'etc/grub.d').mkdir(parents=True)
        self.store=grub.Store(self.root)
        self.item=grub.candidate(part(),'efi','/EFI/Microsoft/Boot/bootmgfw.efi','Windows Boot Manager',pe())
        self.item['name']='Windows 11'; self.item['drive']='/dev/fixture'; self.item['drive_token']='fixture'

    def save(self,items):
        with self.store.locked(): return self.store.commit(items,self.store.revision())

    def test_add_rename_remove_keeps_existing_menu_and_modes(self):
        backup=Path(self.save([self.item]))
        self.assertEqual((backup/'config').read_text(),self.original)
        text=self.store.config.read_text()
        self.assertTrue(text.startswith(self.original)); self.assertIn("menuentry 'Windows 11'",text)
        self.assertEqual(stat.S_IMODE(self.store.config.stat().st_mode),0o600)
        self.assertEqual(stat.S_IMODE(self.store.generator.stat().st_mode),0o755)
        self.item['name']='Work Windows'; self.save([self.item])
        text=self.store.config.read_text();self.assertEqual(text.count(grub.BEGIN),1)
        self.assertNotIn("menuentry 'Windows 11'",text);self.assertIn("menuentry 'Work Windows'",text)
        self.save([]);text=self.store.config.read_text()
        self.assertTrue(text.startswith(self.original));self.assertNotIn('chainloader',text)

    def test_generator_emits_literal_titles_and_valid_grub(self):
        sentinel=self.root/'should-not-exist'
        self.item['name']=f"Bob's Windows; $(touch {sentinel})"[:100]
        self.item['loader']="/EFI/some folder/boot'file.efi";self.item['id']=grub.entry_id(self.item)
        self.save([self.item])
        output=subprocess.check_output(['sh',str(self.store.generator)],text=True)
        self.assertEqual(output,grub.render([self.item]));self.assertFalse(sentinel.exists())
        self.store.check(output)

    def test_undo_restores_original_bytes_and_removes_new_files(self):
        self.save([self.item])
        with self.store.locked(): self.store.undo(self.store.revision())
        self.assertEqual(self.store.config.read_text(),self.original)
        self.assertFalse(self.store.generator.exists());self.assertEqual(self.store.entries(),[])

    def test_stale_menu_and_external_changes_are_never_overwritten(self):
        old=self.store.revision();self.store.config.write_text(self.original+'# external change\n')
        with self.store.locked(),self.assertRaisesRegex(RuntimeError,'changed'):
            self.store.commit([self.item],old)
        self.save([self.item]);self.store.config.write_text(self.store.config.read_text()+'# kernel update\n')
        with self.store.locked(),self.assertRaisesRegex(RuntimeError,'Another tool'):
            self.store.undo(self.store.revision())
        self.assertTrue(self.store.config.read_text().endswith('# kernel update\n'))

    def test_failed_write_restores_all_files(self):
        actual=grub.atomic;failed=False
        def fail(path,data,mode=0o600):
            nonlocal failed
            if path==self.store.config and not failed:
                failed=True;raise OSError('fixture disk full')
            return actual(path,data,mode)
        with patch.object(grub,'atomic',side_effect=fail),self.assertRaises(OSError):self.save([self.item])
        self.assertEqual(self.store.config.read_text(),self.original)
        self.assertFalse(self.store.generator.exists());self.assertEqual(self.store.entries(),[])
        self.assertFalse((self.store.state/'pending.json').exists())

    def test_external_edit_during_validation_is_preserved(self):
        check=self.store.check
        def external_edit(text):
            check(text)
            self.store.config.write_text(self.original+'# concurrent kernel update\n')
        with patch.object(self.store,'check',side_effect=external_edit):
            with self.assertRaisesRegex(RuntimeError,'Another program'):self.save([self.item])
        self.assertTrue(self.store.config.read_text().endswith('# concurrent kernel update\n'))
        self.assertFalse(self.store.generator.exists())

    def test_interrupted_transaction_recovers_on_next_open(self):
        with self.store.locked():
            targets={'config':(b'new config',0o600),'generator':(b'new generator',0o755),'entries':(b'new inventory',0o600)}
            record=self.store.snapshot(targets);grub.atomic(self.store.state/'pending.json',grub.encoded(record))
            grub.atomic(self.store.generator,b'new generator',0o755)
        with self.store.locked(): pass
        self.assertFalse(self.store.generator.exists());self.assertEqual(self.store.config.read_text(),self.original)

    def test_invalid_grub_or_colliding_generator_leaves_configuration_alone(self):
        self.store.generator.write_text('# Some other tool\n')
        with self.assertRaisesRegex(RuntimeError,'Another tool'):self.save([self.item])
        self.store.generator.unlink()
        self.store.config.write_text("menuentry 'broken' {\n")
        with self.assertRaises(RuntimeError):self.save([self.item])
        self.assertFalse(self.store.generator.exists())

    def test_damaged_managed_block_is_not_guessed(self):
        self.store.config.write_text(self.original+grub.BEGIN+'\n'+grub.SIGNATURE+'\n')
        with self.assertRaisesRegex(RuntimeError,'damaged'):self.save([self.item])
        self.assertFalse(self.store.generator.exists())

    def test_closed_client_cannot_start_a_save(self):
        def gone():raise RuntimeError('client closed')
        with self.store.locked(),self.assertRaisesRegex(RuntimeError,'client closed'):
            self.store.commit([self.item],self.store.revision(),gone)
        self.assertEqual(self.store.config.read_text(),self.original);self.assertFalse(self.store.generator.exists())

    def test_unsafe_names_paths_and_uuids_fail_before_writing(self):
        for changes in ({'name':'\n'},{'name':'foo\nbar'},{'uuid':"abc; reboot"},{'loader':'/../etc/passwd'},
                        {'kind':'shell'},{'filesystem':'unknown'}):
            with self.subTest(changes=changes):
                item={**self.item,**changes};item['id']=grub.entry_id(item)
                with self.assertRaises(ValueError):self.save([item])
                self.assertEqual(self.store.config.read_text(),self.original)

    def test_symbolic_configuration_paths_are_rejected(self):
        target=self.root/'outside';target.mkdir();self.store.generator.parent.rmdir()
        self.store.generator.parent.symlink_to(target)
        with self.assertRaisesRegex(RuntimeError,'symbolic'):self.save([self.item])
        self.assertEqual(list(target.iterdir()),[])

    def test_server_rescans_and_deduplicates_an_added_system(self):
        session=grub.Session.__new__(grub.Session);session.store=self.store
        item={**self.item,'available':True};drive=disk(part())
        request={'action':'add','drive':drive['path'],'drive_token':grub.drive_token(drive),
                 'candidate':item['id'],'loader_hash':item['loader_hash'],'name':'First Windows','revision':self.store.revision()}
        with patch.object(grub,'inventory',return_value=[drive]),patch.object(grub,'selected_drive',return_value=drive), \
             patch.object(grub,'scan_drive',return_value={'candidates':[item],'notes':[]}) as scan, \
             patch.object(grub,'client_alive'),patch.object(grub,'current_efi'),patch.object(grub,'current_config'),patch.object(grub,'emit'):
            result=session.handle(request);self.assertEqual(len(result['entries']),1)
            request.update(name='Renamed Windows',revision=result['revision'])
            result=session.handle(request);self.assertEqual(len(result['entries']),1)
            self.assertEqual(result['entries'][0]['name'],'Renamed Windows');self.assertEqual(scan.call_count,2)
            request.update(candidate='untrusted',revision=result['revision'])
            with self.assertRaisesRegex(RuntimeError,'no longer available'):session.handle(request)

    def current_request(self, action, identity=None, **values):
        menu = self.store.status()
        return dict(action=action, id=identity or menu['current_entries'][0]['id'],
                    revision=menu['revision'], menu_revision=menu['menu_revision'], **values)

    def test_current_menu_edit_rename_remove_and_undo_preserve_other_bytes(self):
        self.store.config.write_text(self.original+"menuentry 'Second' --id second { echo second; }\n")
        original = self.store.config.read_bytes()
        with self.store.locked():
            self.store.edit_current(self.current_request('edit-current', name="Bob's Linux"))
        changed = self.store.config.read_text()
        self.assertEqual(grub.menu_entries(changed)[0]['name'], "Bob's Linux")
        self.assertIn("menuentry 'Second' --id second { echo second; }\n", changed)
        self.assertFalse(self.store.generator.exists())
        with self.store.locked(): self.store.undo(self.store.revision())
        self.assertEqual(self.store.config.read_bytes(), original)
        with self.store.locked(): self.store.edit_current(self.current_request('remove-current'))
        self.assertEqual(len(self.store.status()['current_entries']), 1)
        self.assertEqual(self.store.status()['current_entries'][0]['name'], 'Second')

    def test_current_commands_are_checked_and_cannot_escape_the_entry(self):
        original = self.store.config.read_bytes()
        for commands in ("}\nmenuentry 'Injected' { echo bad; }\n#", 'if true; then\n echo unfinished', 'echo bad\0'):
            with self.subTest(commands=commands), self.store.locked(), self.assertRaises((ValueError, RuntimeError)):
                self.store.edit_current(self.current_request('edit-current', name='Changed', commands=commands))
            self.assertEqual(self.store.config.read_bytes(), original)
        with self.store.locked():
            self.store.edit_current(self.current_request('edit-current', name='Changed', commands=' echo changed;'))
        self.assertIn(' echo changed;', self.store.status()['current_entries'][0]['commands'])

    def test_current_menu_refuses_stale_selection_and_last_operating_system_removal(self):
        request = self.current_request('edit-current', name='Changed')
        self.store.config.write_text(self.original+'# another program\n')
        with self.store.locked(), self.assertRaisesRegex(RuntimeError, 'changed'):
            self.store.edit_current(request)
        text = "menuentry 'Linux' { linux /vmlinuz root=UUID=abc; }\nmenuentry 'Firmware' { fwsetup; }\n"
        self.store.config.write_text(text)
        with self.store.locked(), self.assertRaisesRegex(ValueError, 'operating-system'):
            self.store.edit_current(self.current_request('remove-current'))
        self.assertEqual(self.store.config.read_text(), text)

    def test_current_cachyostools_entry_updates_its_generator_and_saved_commands(self):
        self.save([self.item])
        entry = next(e for e in self.store.status()['current_entries'] if e['managed_id'])
        with self.store.locked():
            self.store.edit_current(self.current_request('edit-current', entry['id'], name='Custom Windows',
                                                         commands=' chainloader /EFI/Microsoft/Boot/bootmgfw.efi'))
        output = subprocess.check_output(['sh', str(self.store.generator)], text=True)
        self.assertIn("menuentry 'Custom Windows'", output)
        self.assertIn(' chainloader /EFI/Microsoft/Boot/bootmgfw.efi', output)
        self.assertEqual(self.store.entries()[0]['name'], 'Custom Windows')
        self.save(self.store.entries())
        self.assertIn(' chainloader /EFI/Microsoft/Boot/bootmgfw.efi', self.store.config.read_text())

    def test_current_edit_failure_rolls_back_without_creating_a_generator(self):
        original = self.store.config.read_bytes()
        actual = grub.atomic
        def fail(path, data, mode=0o600):
            if path == self.store.state/'last.json': raise OSError('fixture disk full')
            return actual(path, data, mode)
        with self.store.locked(), patch.object(grub, 'atomic', side_effect=fail), self.assertRaises(OSError):
            self.store.edit_current(self.current_request('edit-current', name='Changed'))
        self.assertEqual(self.store.config.read_bytes(), original)
        self.assertFalse(self.store.generator.exists())


class CurrentMenuTests(unittest.TestCase):
    def test_nested_menus_quotes_variables_comments_and_multiline_headers(self):
        text = '''# menuentry 'Not real' { }
### BEGIN /etc/grub.d/10_linux ###
menuentry "Linux {primary}" --id main {
    echo "${root} is not a brace" # } ignored
    linux /vmlinuz root=UUID=abc
}
submenu 'Advanced options' $menuentry_id_option 'advanced'
{
    menuentry 'Fallback' --id fallback { linux /fallback; }
    menuentry "${generated_name}" { echo dynamic; }
}
### END /etc/grub.d/10_linux ###
menuentry 'Manual' { echo manual; }
'''
        entries = grub.menu_entries(text)
        self.assertEqual([e['name'] for e in entries], ['Linux {primary}', 'Advanced options', 'Fallback', '"${generated_name}"', 'Manual'])
        self.assertEqual(entries[2]['parent_id'], entries[1]['id'])
        self.assertEqual(entries[2]['index_path'], [1, 0])
        self.assertEqual(entries[1]['menu_id'], 'advanced')
        self.assertFalse(entries[3]['editable'])
        self.assertEqual(entries[4]['source'], '')
        changed = grub.edit_menu(text, entries[2]['id'], name='Fallback renamed')
        self.assertEqual(changed, text.replace("'Fallback'", "'Fallback renamed'"))
        removed = grub.edit_menu(text, entries[1]['id'], remove=True)
        self.assertEqual([e['name'] for e in grub.menu_entries(removed)], ['Linux {primary}', 'Manual'])
        self.assertNotIn('start', grub.public_menu(text)[0])

    def test_dynamic_titles_and_malformed_blocks_are_not_rewritten(self):
        text = 'menuentry "$name" { echo dynamic; }\n'
        with self.assertRaisesRegex(ValueError, 'generated title'):
            grub.edit_menu(text, grub.menu_entries(text)[0]['id'], name='Changed')
        for damaged in ("menuentry 'Bad' {", "menuentry 'Bad { }", "menuentry 'Bad' { echo ${root; }"):
            with self.subTest(damaged=damaged), self.assertRaises(ValueError): grub.menu_entries(damaged)


class MenuAccessTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='cachyostools-grub-access-')
        self.addCleanup(temporary.cleanup); self.root = Path(temporary.name)
        config = self.root/'boot/grub/grub.cfg'; config.parent.mkdir(parents=True)
        config.write_text("menuentry 'Fixture Linux' { linux /vmlinuz; }\n")
        self.store = grub.Store(self.root)
        for name, value in [('Store', self.store), ('inventory', []), ('firmware', 'uefi'), ('secure_boot', False)]:
            mock = patch.object(grub, name, return_value=value); mock.start(); self.addCleanup(mock.stop)

    def test_unreadable_boot_menu_requests_authentication_and_clears_stale_entries(self):
        if os.geteuid() == 0: self.skipTest('Requires an unprivileged reader.')
        self.store.config.chmod(0)
        try: result = grub.status_payload()
        finally: self.store.config.chmod(0o644)
        self.assertTrue(result['supported']); self.assertEqual(result['menu_revision'], '')
        self.assertEqual(result['current_entries'], [])
        self.assertIn('authenticate', result['message'])
        loaded = grub.status_payload()
        self.assertTrue(loaded['menu_revision'])
        self.assertEqual(loaded['current_entries'][0]['name'], 'Fixture Linux')

    def test_private_boot_directory_requires_authentication_during_discovery(self):
        if os.geteuid() == 0: self.skipTest('Requires an unprivileged reader.')
        self.store.config.parent.chmod(0)
        try:
            with patch.object(grub, 'Store', side_effect=lambda: type(self.store)(self.root)):
                result = grub.status_payload()
        finally: self.store.config.parent.chmod(0o755)
        self.assertTrue(result['supported']); self.assertEqual(result['menu_revision'], '')
        self.assertIn('authenticate', result['message'])

    def test_private_editor_metadata_does_not_hide_a_readable_menu(self):
        if os.geteuid() == 0: self.skipTest('Requires an unprivileged reader.')
        self.store.state.mkdir(parents=True); self.store.state.chmod(0)
        try: result = grub.status_payload()
        finally: self.store.state.chmod(0o700)
        self.assertTrue(result['supported']); self.assertTrue(result['menu_revision'])
        self.assertEqual(result['current_entries'][0]['name'], 'Fixture Linux')

    def test_unavailable_grub_clears_cached_menu_state(self):
        with patch.object(grub, 'Store', side_effect=RuntimeError('No installed GRUB configuration')):
            result = grub.status_payload()
        self.assertFalse(result['supported']); self.assertEqual(result['menu_revision'], '')
        self.assertEqual(result['current_entries'], [])
        self.assertEqual(result['message'], 'No installed GRUB configuration')


if __name__=='__main__':unittest.main()
