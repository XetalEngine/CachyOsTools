# Optional real ISO/VM checks

These tests require an Archiso-capable Linux host, stock QEMU, `qemu-img`,
SeaBIOS/OVMF, and optionally KVM. They are separate from the unprivileged unit
tests. The runner refuses existing run directories and exposes only a new
20 GiB qcow2 disk and the supplied ISO. It attaches no network or host disks.

## Prepare a disposable source and test ISO

1. Bootstrap a **new test root** using the distribution's signed packages and
   isolated pacman configuration/keyring. Include `base`, its native kernel,
   and either mkinitcpio or dracut. Include GRUB for BIOS/GRUB tests; omit GRUB
   for the systemd-boot fallback test. Keep all package operations inside this
   root. Never use a personal system snapshot as this test fixture.
2. Create `/etc/xetal-test-marker` in that root. Copy its signed kernel package
   and signature to `/opt/xetal-test/`, and initialize its distribution keyring.
   Install `boot_check.sh` at `/usr/local/sbin/xetal-test-boot` with mode 755.
   Enable this unit in the test root:

   ```ini
   [Unit]
   Description=Disposable VM restore validation
   After=local-fs.target
   [Service]
   Type=oneshot
   ExecStart=/usr/local/sbin/xetal-test-boot
   [Install]
   WantedBy=multi-user.target
   ```

3. Build a releng profile with the production `iso/common.sh`, `installer.sh`
   and `restore-boot.sh`, the test root archive, checksum and snapshot metadata
   in `/opt/clone/`. Use the production package preparation functions and
   `mkarchiso`. The archive and metadata format are in `iso/build.sh`.
4. For this **test ISO only**, start `/usr/local/bin/installer.sh` using a
   oneshot service with `StandardInput=tty`, `StandardOutput=tty`,
   `StandardError=tty`, `TTYPath=/dev/ttyS0`, `TimeoutStartSec=0`,
   `After=systemd-user-sessions.service` and
   `Conflicts=serial-getty@ttyS0.service`. Enable it in `multi-user.target`.
5. Append `console=ttyS0,115200n8` to live kernel command lines. For Syslinux,
   modify only lines beginning `APPEND archisobasedir=`, not every `APPEND`
   directive: releng also uses `APPEND` for its firmware selector. For UEFI,
   modify the loader entries' `options` lines.

## Run

```bash
python3 tests/integration/check_vm.py \
  --iso /path/to/test.iso \
  --run-dir build/iso-vm-new-run \
  --firmware uefi --kernel-transaction
```

Use `--firmware bios` for the BIOS case. `--qemu`, `--qemu-data`, `--ovmf-dir`
and `--accel tcg` support different host installations. KVM requires access to
`/dev/kvm`; the runner itself does not need root. Logs are retained per stage.

Success requires installer completion, the restored root reaching its test
service, a real signed kernel package transaction invoking the Xetal hook,
clean poweroff, and a second boot reaching the completion marker. Reinstalling
the same kernel exercises pacman hooks; it does **not** demonstrate upgrading
to a different kernel version. Test that separately before making that claim.

For offline checks, export the online build using `create_offline_package.sh`,
prepare a new private build from that archive, and run its package preparation
and `mkarchiso` inside `unshare --net` as root. Then test the resulting ISO with
the same runner. This proves that a successful build did not use the network.
