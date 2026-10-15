# System ISO compatibility and release checks

This feature creates a **file-based snapshot**, then restores it to a new,
**unencrypted ext4 root filesystem**. It is not a disk image. Source partitions,
encryption, Btrfs snapshots, bootloader settings and auxiliary filesystem mounts
are not recreated. The installer tells the user this before disk erasure.

Do not describe a distribution as fully supported based only on the automated
fixture tests. A successful ISO build also does not prove that its restored
system boots. Record a real install, reboot and kernel-update test for each
claimed configuration before release.

## Distribution and repository boundaries

The build host needs x86_64, pacman, systemd, an upstream-compatible `archiso`
installation with its `releng` profile, and `archlinux-keyring`. The dependency
panel checks tools and profile/keyring files. Install these through the host
distribution's supported package sources; the creator never adds Arch
repositories to the host.

The small live installer uses **Arch core and extra**, explicit Arch mirrors,
its own keyring, database and package cache. This is necessary because Manjaro
has its own repositories and kernel packaging; its mirrorlist cannot supply an
Arch `releng` profile. CachyOS repository overrides must likewise not change the
live installer's kernel or package set.

All live packages and dependencies are resolved, downloaded and verified before
snapshotting. `mkarchiso` then uses a local mirror containing those exact
repository databases and packages. Missing packages, invalid signatures or
incomplete offline archives stop the build before the snapshot. Only the known
legacy `broadcom-wl` name is migrated automatically; unknown missing packages
are never silently dropped. Older offline archives can keep their available
prebuilt Broadcom package.

The source system's package database, distribution repositories, installed
packages and networking service are copied as part of the snapshot. Moving a
CachyOS system to a less capable CPU does not make its optimized binaries
compatible with that CPU. Hardware drivers must also support the target.

## Boot and restore behavior

| Source/target capability | Restore behavior |
| --- | --- |
| UEFI, source GRUB with EFI modules | GRUB installed at the removable EFI path; writable NVRAM is not required |
| UEFI, no usable GRUB, configured Limine available | Generated Limine menu, dedicated boot-file generations, optional Windows entry |
| UEFI, no usable source GRUB or configured Limine, systemd-boot available | systemd-boot fallback with generated kernel entries |
| BIOS, source GRUB with `i386-pc` modules | GPT BIOS boot partition and GRUB BIOS installation |
| BIOS without source GRUB BIOS support | Refused before disk erasure; use UEFI or prepare the source with GRUB |
| mkinitcpio | Fresh generic ext4 initramfs for each discovered installed kernel |
| dracut without mkinitcpio | Fresh generic, non-host-only initramfs |
| Manjaro/custom kernel filenames | Kernel release matched to module directories, including versioned `/boot/vmlinuz-*` images |
| Live installer booted with Secure Boot enabled | Restore refused; disable it while installing |
| non-x86_64 or non-systemd source | Not supported by this restore workflow |

Restored systems use `/efi` for the EFI system partition. Original boot menus,
rEFInd configurations and source disk layouts are not preserved as the active boot path.
A pacman hook calls `/usr/local/sbin/xetal-update-boot` after kernel, microcode
or Limine changes. It maintains the generated GRUB, Limine or systemd-boot menu and reports any
initramfs error. Source encryption/resume/root UUIDs are not reused. Original
fstab and crypttab files are retained under `/etc/xetal-source/` for reference.

Do not delete `/etc/xetal-boot.conf`, `/usr/local/lib/xetal-iso/`, the update
script or its pacman hook unless replacing this boot arrangement deliberately.
Custom kernel arguments and unusual systemd mount units need manual review.
Encryption-preserving restores, reconstruction of source multi-disk layouts,
Btrfs rollback history, UKI-only sources without discoverable kernel images,
and CPU-instruction-set conversion are outside the supported scope.

The installer opens its TUI immediately. Its menu is ordered as follows:

1. Restore entire hard drive (the original erase-and-install flow; selected by default).
2. Check snapshot integrity (optional; returns to this menu with the result).
3. Use unused space — devvz (PR #7), on UEFI machines.
4. Use prepared partitions (advanced) — devvz (PR #7), on UEFI machines.

No checksum scan runs automatically at startup or before restore. Selecting
the check verifies the complete archive against its SHA-256 manifest. A failed
check blocks restoration in that installer session until a check succeeds.
Skipping the check avoids the extra full read of the archive; corruption may
then only be discovered during extraction, after target formatting.

Drive selectors show filesystem labels and GPT partition names alongside the
device, size and model. Confirmation uses two default-No Yes/No dialogs, with
mouse/keyboard navigation and Escape to cancel. There are no typed confirmation
phrases or device-path challenges. A terminal and `dialog` are required for
installation; they are provided by the live ISO.

Firmware and disk checks run after a restore mode is selected and before writes.
The installer excludes identifiable live-media disks, rejects mounted disks,
active swap, readonly devices and active mapper/RAID/LVM consumers, checks disk
capacity, and revalidates the selection after the two warning dialogs. It
never runs a host-wide `swapoff -a` or unmounts an unrelated `/mnt` tree. If it
cannot identify the live medium (including some copy-to-RAM arrangements), it
refuses to erase a disk.

The new partition and Limine support is adapted from
[devvz's PR #7](https://github.com/XetalEngine/CachyOsTools/pull/7). Free-space
mode uses an already unallocated region on a GPT disk; it never shrinks an
existing filesystem. Prepared-partition mode formats the selected root and
an empty, dedicated GPT EFI system partition. EFI partitions containing files
are rejected, protecting Windows and shared boot partitions. Both modes require
UEFI. They append an Xetal Engine firmware entry without changing the order of
existing entries; if registration fails, the installer reports the fallback
EFI path to select. `installer.sh --dry-run` previews the selected plan without
writing it.

Limine updates build a separate generation before activating it. Signing,
initramfs-generation and activation failures preserve or restore the preceding
boot files. Current and previous generations are retained. The previous EFI
binary and its configuration are also available at
`/efi/EFI/XetalPrevious/BOOTX64.EFI` and `limine.conf` for recovery. FAT does not
provide an atomic transaction across the two live boot-file renames; sudden
power loss during activation can require selecting that recovery EFI binary.
Early GPU modules are included only for kernels that actually provide them.

Advanced settings:

- `XETAL_ESP_MIB=4096`: override the EFI partition size (512–16384 MiB;
  otherwise at least 2048 MiB, scaled by kernel count). Limine keeps two
  generations; allow extra space for large initramfs images or many kernels.
- `XETAL_SIGN_BOOT=1`: explicitly request signing with the snapshot's existing
  sbctl key and certificate. The choice is saved as `SIGN_BOOT=1` in
  `/etc/xetal-boot.conf` for subsequent updates. Signing and verification must
  both succeed. This does not create or enroll firmware keys, and Secure Boot
  still needs to be disabled during installation. Validate firmware key
  enrollment and an actual boot before enabling Secure Boot.
- `XETAL_NO_SIGN=1`: disable signing for the current invocation; during an
  installation this saves `SIGN_BOOT=0` in the restored system.
- `LIMINE_GRAPHICS=no` in `/etc/xetal-boot.conf`: use Limine's text menu; this
  setting survives regeneration and recloning.

The existing ISO build, offline packaging, exclusions, first-boot options and
Xetal Engine logo assets/renderer remain in place. The historical VM results in
`ISO_VALIDATION.md` predate these additions and do not certify the new modes.

## Snapshot and recovery limitations

Close VMs, databases and applications with changing data before snapshotting.
Rsync is not an atomic filesystem snapshot. Unreadable files abort the build;
vanished files get one retry. A package transaction or a package-list change
during copying also aborts. Separately configured `/boot`, `/efi`, `/home`,
`/usr` and `/var` filesystems must be mounted. Standard virtual/runtime mounts,
`/mnt`, `/media`, build files and root/home `.snapshots` contents are excluded.
Review application data stored on auxiliary mounts before relying on a clone.

Every attempt has a private directory under `~/iso/xiso/builds/`; previous
snapshots and output ISOs are not deleted by a retry. The directory contains
`build.log`, the prepared profile, package database/cache and manifest. Failed
builds retain their files for diagnosis. `last-build.txt` points to the latest
attempt. Successful builds remove their temporary snapshot and work root;
the prepared profile and package inputs remain and consume disk space.
Remove an old build directory only after confirming it has no mounted children.

Sudo credentials are sent through the process input pipe and are not embedded
in temporary scripts, command arguments or logs. Generated scripts are private
and use shell-quoted paths. Optional first-boot actions record completed steps
so retries do not repeatedly regenerate SSH identity. GPU adaptation uses
already-installed drivers; install missing drivers using the distribution's
supported procedure. It does not perform partial package upgrades.

## Offline use

After a successful online package preparation/build, run:

```bash
./create_offline_package.sh
# Or choose the exact build and destination:
./create_offline_package.sh /path/to/build-directory /path/to/packages.tar.gz
```

The exporter includes only the selected Arch repository databases and resolved
package archives. It rejects missing packages and existing output archives.
Select the resulting archive with **Choose Offline Archive**. The builder
extracts it into its private build directory, checks every dependency and
verifies signatures with the installed Arch keyring. It does not replace the
host's pacman cache or database. A legacy archive lacking databases or current
required packages needs to be regenerated. Offline creation still requires
the host's archiso tools and Arch signing keys to be installed.

## Automated checks

```bash
python3 -m unittest discover -s tests -v
cmake -S . -B build/iso-check -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/iso-check --parallel 2
```

Tests run on temporary files with mocked package and block-device interfaces;
they never erase a real disk. The Qt generator tests compile the actual
production generator and resources, test all four online/offline/adaptation
combinations, check private file permissions and shell metacharacter handling,
and parse/stage the emitted payloads. They require Qt6 development tools,
`pkg-config`, `g++` and `rcc`; skipping those tests is not a full validation.
CI runs these checks, but does not certify a real installation.

## Required VM release matrix

For each row, record distribution release/branch, archiso version, source
kernel, initramfs generator, source bootloader, build result, live boot, restore,
restored boot, and a subsequent kernel update plus reboot. Use fresh disposable
VM disks and retain build/serial logs. Never substitute a real disk for a test.

| Configuration | Required coverage |
| --- | --- |
| Arch, GRUB, mkinitcpio | BIOS and UEFI; online and exported offline archive |
| Arch/EndeavourOS, dracut | UEFI restore, kernel update and reboot |
| CachyOS, current default boot arrangement | UEFI fallback; compatible CPU features; kernel update and reboot |
| CachyOS, GRUB | UEFI and BIOS when source has BIOS modules |
| Manjaro Stable | Native versioned kernel; UEFI; kernel update and reboot |
| Manjaro Testing/Unstable | Separate validation for each branch being advertised |
| Failure cases | Corrupt archive, missing offline dependency, unavailable mirror, interrupted build, live-disk selection, unsupported firmware |

Current validation results are recorded in [ISO_VALIDATION.md](ISO_VALIDATION.md). Rows without
real results remain unverified. Do not infer Manjaro or CachyOS VM success from
an Arch VM result or from fixtures using their kernel names.

## References

- [Archiso profile and package configuration](https://wiki.archlinux.org/title/Archiso)
- [mkarchiso options](https://man.archlinux.org/man/mkarchiso.1.en)
- [Manjaro repositories and servers](https://wiki.manjaro.org/index.php?title=Repositories_and_Servers%2Fen)
- [CachyOS boot managers](https://wiki.cachyos.org/installation/boot_managers/)
- [mkinitcpio](https://man.archlinux.org/man/mkinitcpio.8)
- [dracut](https://man.archlinux.org/man/dracut.8)
