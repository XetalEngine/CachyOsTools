# ISO validation record — 2026-09-24

The following results cover this working-tree revision. They are evidence for
the configurations listed, not certification of every Arch derivative or a
complete installed desktop. See [compatibility limits](ISO_COMPATIBILITY.md).

## Automated and build checks

- **46 regression tests passed**, including the compiled Qt script generator,
  online/offline package resolution, signature/download failures, missing
  dependencies, kernel naming, bootloader/initramfs fallbacks, disk protection,
  NetworkManager binding/error handling, and resetting adaptation state when
  cloning a previously restored system.
- **ShellCheck 0.11.0:** no warnings/errors in the production ISO shell scripts
  and offline exporter. Bash syntax checks also passed.
- **Full Qt application Release build passed** with Qt 6.11.1 and GCC 16.1.1.
  Existing warnings in unrelated application code remain.
- GitHub Actions now runs regression tests, ShellCheck and the application
  build. This workflow has not yet been run on GitHub.

## Real package and ISO builds

The host was x86_64 Arch Linux with archiso **89-1**. All test repositories,
keyrings, package databases and install roots were isolated under the ignored
`build/iso-integration/` directory. No host packages were installed or upgraded.
Tests used synthetic source roots; the user's personal system was not cloned.

The **complete releng package list plus the creator's additions** resolved to
**138 explicit targets / 508 package archives**. All packages downloaded and
passed pacman's signature verification, then `mkarchiso` built a **2.4 GiB**
test ISO from the pinned local mirror. The obsolete `broadcom-wl` target was
replaced with `broadcom-wl-dkms` and matching `linux-headers`. DKMS successfully
compiled and installed `updates/dkms/wl.ko.zst` for `7.2.6-arch2-1`.

The offline exporter also produced a complete archive for the smaller test
profile. A new private build verified that archive and built its ISO inside
`unshare --net`, with no network interface or route available. That ISO passed
the UEFI restore and subsequent boot checks below.

## Completed VM checks

Tests used stock QEMU **11.1.1**, KVM, 2 virtual CPUs, 2 GiB RAM, fresh 20 GiB
qcow2 disks, SeaBIOS or OVMF, and **no VM network or physical-disk access**.
The locally installed custom QEMU executable did not boot these fixtures;
a stock package was extracted into the test directory without replacing it.

| Source fixture | Firmware / restored loader | ISO boot + restore | Restored boot | Signed kernel reinstall + clean second boot |
| --- | --- | --- | --- | --- |
| Arch, `linux 7.2.6.arch2-1`, mkinitcpio 42-1 | BIOS / GRUB 2:2.14-1 | PASS | PASS | PASS |
| Arch, same kernel/mkinitcpio, **complete releng live package set** | BIOS / GRUB 2:2.14-1 | PASS | PASS | PASS |
| Arch, same kernel/mkinitcpio, **complete releng live package set** | UEFI / GRUB 2:2.14-1 | PASS | PASS | PASS |
| Arch, same kernel/mkinitcpio, offline-built ISO | UEFI / GRUB 2:2.14-1 | PASS | PASS | PASS |
| Arch, same kernel, dracut 111-1, no GRUB or mkinitcpio | UEFI / systemd-boot 261.3 | PASS | PASS | PASS |
| Manjaro Stable minimal root, release 26.1.2-1, `linux612 6.12.108-1`, mkinitcpio 41.1-1 | UEFI / GRUB 2:2.14-1 | PASS | PASS | PASS |
| Arch base with native CachyOS `linux-cachyos 7.2.5-1`, mkinitcpio 42-1, no GRUB | UEFI / systemd-boot 261.3 | PASS | PASS | PASS |

Manjaro's root was installed from its own Stable repositories with its own
keyring. The CachyOS-kernel fixture used signed packages from the CachyOS
repository, but its base identified as Arch: **this is not a full CachyOS
desktop validation**. No distribution-name fixtures are counted as VM results.

The package transaction reinstalls the signed kernel package already in the
fixture. Logs confirm the production `zz-xetal-boot.hook` regenerated boot
files, followed by a clean shutdown and successful boot from the virtual disk
without the ISO. This checks the hook path, **not an upgrade to a different
kernel version**.

The full releng tests also exercised the `dialog` disk selector. The final
disk-query failure checks and adaptation-state reset were included in the
full releng and offline Arch ISOs; other initial fixture runs preceded those
final changes. Those checks also have dedicated regression tests.

## Reproduction and retained evidence

Use [the VM runner and fixture instructions](../tests/integration/README.md).
The runner refuses existing run directories and waits for clean guest shutdown
before the second boot, avoiding false failures from unflushed guest state.

Local evidence remains under `build/iso-integration/`:

- `full-releng-build.log` and `full-releng/package-files.txt`
- `verified-full-bios/`, `verified-full-uefi/`
- `offline-export.log`, `offline-build.log`, `verified-offline-uefi/`
- `verified-arch-bios/`, `verified-fallback-uefi/`
- `verified-manjaro-uefi/`, `verified-cachyos-uefi/`
- `app-build.log`
- `app-build-final.log`, `unit-tests.log`

Per-VM directories contain installer, kernel-transaction and second-boot
serial logs. Source hashes, tested ISO hashes and the checked VM markers are
also recorded in [the machine-readable result](iso-validation-2026-09-24.json).
Disposable install roots, package caches, ISOs and virtual disks were removed
after validation so that they will not inflate the next personal snapshot.
Local logs remain excluded from Git.

## Still required for broader release claims

- Create a full personal-system snapshot through the GUI on native CachyOS
  and Manjaro hosts, then restore it in a fresh VM. The current native tests
  bootstrap minimal package roots on the Arch test host.
- Test upgrades to a **different** kernel version and multi-kernel removal.
- Test complete CachyOS desktop installations, optimized CPU variants,
  native Limine/rEFInd setups and Manjaro Testing/Unstable separately.
- Exercise actual GPU/Wi-Fi hardware, NetworkManager profile adaptation,
  interactive account changes and SSH/machine-ID options on disposable systems.
- Secure Boot, ARM, preservation of source encryption/partition layouts,
  and Btrfs snapshot history remain outside the supported restore scope.

Do not advertise universal distribution or hardware compatibility from these
results. The supported behavior is capability-based and explicitly refuses
unsupported firmware/source combinations before target-disk erasure.
