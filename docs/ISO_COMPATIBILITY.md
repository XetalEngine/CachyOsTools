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
| UEFI, no usable source GRUB, systemd-boot available | systemd-boot fallback with generated kernel entries |
| BIOS, source GRUB with `i386-pc` modules | GPT BIOS boot partition and GRUB BIOS installation |
| BIOS without source GRUB BIOS support | Refused before disk erasure; use UEFI or prepare the source with GRUB |
| mkinitcpio | Fresh generic ext4 initramfs for each discovered installed kernel |
| dracut without mkinitcpio | Fresh generic, non-host-only initramfs |
| Manjaro/custom kernel filenames | Kernel release matched to module directories, including versioned `/boot/vmlinuz-*` images |
| Secure Boot, non-x86_64 or non-systemd source | Not supported by this restore workflow |

Restored systems use `/efi` for the EFI system partition. Existing Limine,
rEFInd or other boot configurations are not preserved as the active boot path.
A pacman hook calls `/usr/local/sbin/xetal-update-boot` after kernel changes.
It maintains the generated GRUB menu or systemd-boot entries and reports any
initramfs error. Source encryption/resume/root UUIDs are not reused. Original
fstab and crypttab files are retained under `/etc/xetal-source/` for reference.

Do not delete `/etc/xetal-boot.conf`, `/usr/local/lib/xetal-iso/`, the update
script or its pacman hook unless replacing this boot arrangement deliberately.
Custom kernel arguments and unusual systemd mount units need manual review.
Secure Boot signing, encryption-preserving restores, multi-disk layouts,
Btrfs rollback history, UKI-only sources without discoverable kernel images,
and CPU-instruction-set conversion are outside the supported scope.

The installer checks snapshot integrity and firmware capability before disk
selection. It excludes identifiable live-media disks, rejects mounted disks,
active swap, readonly devices and active mapper/RAID/LVM consumers, checks disk
capacity, and revalidates the selection after a typed erase confirmation. It
never runs a host-wide `swapoff -a` or unmounts an unrelated `/mnt` tree. If it
cannot identify the live medium (including some copy-to-RAM arrangements), it
refuses to erase a disk.

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
