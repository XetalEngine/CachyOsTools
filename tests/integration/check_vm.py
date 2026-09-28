#!/usr/bin/env python3
"""Boot a serial-enabled test ISO using a NEW virtual disk, then boot the clone.

The test ISO must contain the synthetic boot-check service; this is not a test
for an arbitrary personal snapshot. No physical disks or network are exposed.
"""
import argparse
import os
from pathlib import Path
import select
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--run-dir', type=Path, required=True,
                        help='New directory; existing paths are refused')
    parser.add_argument('--firmware', choices=['bios', 'uefi'], required=True)
    parser.add_argument('--qemu', default='qemu-system-x86_64')
    parser.add_argument('--qemu-data', default='/usr/share/qemu')
    parser.add_argument('--ovmf-dir', type=Path, default=Path('/usr/share/edk2/x64'))
    parser.add_argument('--accel', choices=['kvm', 'tcg'], default='kvm')
    parser.add_argument('--kernel-transaction', action='store_true')
    parser.add_argument('--timeout', type=int, default=300)
    args = parser.parse_args()
    iso = args.iso.resolve(strict=True)
    if not iso.is_file(): parser.error('ISO must be a regular file')
    case = args.run_dir.resolve()
    case.mkdir(parents=True, exist_ok=False)
    disk = case / 'target.qcow2'
    subprocess.run(['qemu-img', 'create', '-f', 'qcow2', str(disk), '20G'], check=True)
    firmware = []
    if args.firmware == 'uefi':
        variables = case / 'OVMF_VARS.fd'
        shutil.copy(args.ovmf_dir / 'OVMF_VARS.4m.fd', variables)
        firmware = ['-drive', 'if=pflash,format=raw,readonly=on,file=' +
                    str(args.ovmf_dir / 'OVMF_CODE.4m.fd'),
                    '-drive', 'if=pflash,format=raw,file=' + str(variables)]
    stages = ['install', 'boot']
    if args.kernel_transaction: stages.append('updated-boot')
    for stage in stages:
        command = [args.qemu, '-L', args.qemu_data, '-machine', 'q35', '-accel', args.accel,
                   '-cpu', 'host' if args.accel == 'kvm' else 'max', '-m', '2048', '-smp', '2',
                   '-display', 'none', '-monitor', 'none', '-nic', 'none', '-no-reboot',
                   '-serial', 'stdio', '-drive', f'file={disk},if=virtio,format=qcow2'] + firmware
        if stage == 'install': command += ['-cdrom', str(iso), '-boot', 'd']
        else: command += ['-boot', 'c']
        print(args.firmware, stage, 'starting', flush=True)
        with (case / (stage + '-qemu.log')).open('wb') as errors:
            process = subprocess.Popen(command, stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE, stderr=errors)
            try:
                deadline = time.monotonic() + args.timeout
                buffer = b''
                selected = confirmed = final_confirmed = passed = False
                with (case / (stage + '.log')).open('wb') as output:
                    while time.monotonic() < deadline:
                        ready, _, _ = select.select([process.stdout], [], [], 1)
                        if ready:
                            chunk = os.read(process.stdout.fileno(), 65536)
                            if not chunk: break
                            output.write(chunk)
                            output.flush()
                            buffer = (buffer + chunk)[-20000:]
                            if stage == 'install':
                                if b'Whole target disk (for example /dev/sda):' in buffer and not selected:
                                    process.stdin.write(b'/dev/vda\n')
                                    process.stdin.flush()
                                    selected = True
                                elif b'Select disk to ERASE' in buffer and not selected:
                                    # This VM exposes exactly one writable disk.
                                    process.stdin.write(b'\r')
                                    process.stdin.flush()
                                    selected = True
                                if b'Confirm Target' in buffer and not confirmed:
                                    # Both warnings default to No; Tab selects Yes.
                                    process.stdin.write(b'\t\r')
                                    process.stdin.flush()
                                    confirmed = True
                                if b'FINAL WARNING' in buffer and not final_confirmed:
                                    process.stdin.write(b'\t\r')
                                    process.stdin.flush()
                                    final_confirmed = True
                                if b"Type 'WIPE' to confirm:" in buffer and not confirmed:
                                    process.stdin.write(b'WIPE\n')
                                    process.stdin.flush()
                                    confirmed = True
                                if b'[6/6] Installation complete.' in buffer:
                                    passed = True
                                    break  # The installer has already unmounted the disk.
                                if b'[ERROR]' in buffer:
                                    raise RuntimeError(buffer[-3000:].decode(errors='replace'))
                            else:
                                marker = (b'XETAL_UPDATED_BOOT_OK' if stage == 'updated-boot' else
                                          b'XETAL_KERNEL_TRANSACTION_OK' if args.kernel_transaction else
                                          b'XETAL_RESTORE_BOOT_OK')
                                if marker in buffer: passed = True
                                # Wait for guest poweroff: killing at the marker
                                # loses unflushed filesystem/package state.
                        if process.poll() is not None: break
                if not passed:
                    raise RuntimeError('Stage failed; last output:\n' + buffer[-3000:].decode(errors='replace'))
                if stage != 'install' and process.wait(timeout=20) != 0:
                    raise RuntimeError('Guest did not shut down cleanly')
                print('PASS', args.firmware, stage, flush=True)
            finally:
                if process.poll() is None:
                    process.terminate()
                    try: process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
                process.stdin.close()
                process.stdout.close()
    print('Logs:', case, flush=True)


if __name__ == '__main__':
    main()
