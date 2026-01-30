// KVM VM tab slots
void MainWindow::on_multiGpuXorgToggle_clicked() {
    QString instructions = R"(
# Multi-GPU Xorg Configuration Instructions
# =======================================
# 
# This creates a hybrid X11 configuration for systems with both Intel and NVIDIA GPUs.
# 
# FIRST: Detect your actual hardware BusIDs:
# 
# 1. Find your Intel GPU BusID:
#    lspci | grep -i "vga\|display" | grep -i intel
#    Example output: 00:02.0 VGA compatible controller: Intel Corporation ...
#    BusID would be: "PCI:0:2:0" (convert 00:02.0 to PCI:0:2:0)
# 
# 2. Find your NVIDIA GPU BusID:
#    lspci | grep -i "vga\|display" | grep -i nvidia
#    Example output: 01:00.0 VGA compatible controller: NVIDIA Corporation ...
#    BusID would be: "PCI:1:0:0" (convert 01:00.0 to PCI:1:0:0)
# 
# 3. Create /etc/X11/xorg.conf.d/10-hybrid.conf with your actual BusIDs:

Section "ServerLayout"
    Identifier "layout"
    Screen 0 "intel"
    Screen 1 "nvidia" RightOf "intel"
EndSection

Section "Device"
    Identifier "intel"
    Driver "modesetting"
    BusID "PCI:0:2:0"  # REPLACE WITH YOUR INTEL GPU BUSID
    Option "AccelMethod" "glamor"
    Option "DRI" "3"
    Screen 0
EndSection

Section "Device"
    Identifier "nvidia"
    Driver "nvidia"
    BusID "PCI:1:0:0"  # REPLACE WITH YOUR NVIDIA GPU BUSID
    Option "AllowEmptyInitialConfiguration" "true"
    Option "Coolbits" "28"
    Option "TripleBuffer" "true"
    Screen 1
EndSection

Section "Screen"
    Identifier "intel"
    Device "intel"
EndSection

Section "Screen"
    Identifier "nvidia"
    Device "nvidia"
EndSection

# After creating the file, restart X11 or reboot:
# sudo systemctl restart display-manager
# 
# To verify the configuration:
# nvidia-smi
# xrandr --listproviders
# 
# TIP: If you have different GPU vendors (AMD instead of Intel), 
# replace "modesetting" with "amdgpu" or "radeon" as appropriate.
)";
    
    showTweakInstructions("Multi-GPU Xorg Configuration", instructions);
}

void MainWindow::on_multiGpuXorgConfigButton_clicked() {
    openConfigInNano("/etc/X11/xorg.conf.d/10-hybrid.conf");
}

void MainWindow::on_multiGpuXorgBackupButton_clicked() {
    backupConfigFile("/etc/X11/xorg.conf.d/10-hybrid.conf", "Multi-GPU Xorg Configuration");
}

void MainWindow::on_grubConfigToggle_clicked() {
    QString instructions = R"(
# GRUB Configuration for IOMMU Instructions
# ========================================
# 
# This configures GRUB to enable IOMMU for GPU passthrough.
# 
# FIRST: Determine your CPU vendor for the correct IOMMU parameter:
# 
# 1. Check your CPU vendor:
#    lscpu | grep "Vendor ID"
#    - If it shows "GenuineIntel" → use "intel_iommu=on"
#    - If it shows "AuthenticAMD" → use "amd_iommu=on"
# 
# 2. Edit /etc/default/grub and add these parameters to GRUB_CMDLINE_LINUX_DEFAULT:

GRUB_CMDLINE_LINUX_DEFAULT="loglevel=3 quiet intel_iommu=on iommu=pt nvidia.NVreg_EnableGpuUvm=1"

# NOTE: Replace "intel_iommu=on" with "amd_iommu=on" if you have an AMD CPU
# 
# After editing, regenerate GRUB config:
# sudo grub-mkconfig -o /boot/grub/grub.cfg
# 
# ⚠️  IMPORTANT: You MUST reboot after this step before proceeding to the next step!
# sudo reboot
# 
# To verify IOMMU is enabled after reboot:
# dmesg | grep -e DMAR -e IOMMU
# 
# Expected output should show "DMAR: IOMMU enabled"
)";
    
    showTweakInstructions("GRUB IOMMU Configuration", instructions);
}

void MainWindow::on_grubConfigButton_clicked() {
    openConfigInNano("/etc/default/grub");
}

void MainWindow::on_grubConfigBackupButton_clicked() {
    backupConfigFile("/etc/default/grub", "GRUB Configuration");
}

void MainWindow::on_verifyIommuToggle_clicked() {
    QString instructions = R"(
# IOMMU and GPU Detection Verification Instructions
# ===============================================
# 
# Run these commands to verify your system is ready for GPU passthrough:
# 
# 1. Check IOMMU status:
# dmesg | grep -e DMAR -e IOMMU
# 
# Expected output should show:
# [    0.005227] ACPI: DMAR 0x0000000079D3DC80 0000A8 (v01 INTEL  EDK2     00000002      01000013)
# [    0.005241] ACPI: Reserving DMAR table memory at [mem 0x79d3dc80-0x79d3dd27]
# [    0.041309] DMAR: IOMMU enabled
# [    0.105448] DMAR: Host address width 39
# [    0.105448] DMAR: DRHD base: 0x000000fed90000 flags: 0x0
# [    0.105452] DMAR: dmar0: reg_base_addr fed90000 ver 1:0 cap 1c0000c40660462 ecap 19e2ff0505e
# [    0.105454] DMAR: DRHD base: 0x000000fed91000 flags: 0x1
# [    0.105456] DMAR: dmar1: reg_base_addr fed91000 ver 1:0 cap d2008c40660462 ecap f050da
# [    0.105457] DMAR: RMRR base: 0x0000007a5d6000 end: 0x0000007a81ffff
# [    0.105458] DMAR: RMRR base: 0x0000007b800000 end: 0x0000007fffffff
# [    0.105459] DMAR-IR: IOAPIC id 2 under DRHD base  0xfed91000 IOMMU 1
# [    0.105460] DMAR-IR: HPET id 0 under DRHD base 0xfed91000
# [    0.105461] DMAR-IR: Queued invalidation will be enabled to support x2apic and Intr-remapping.
# [    0.107017] DMAR-IR: Enabled IRQ remapping in x2apic mode
# [    0.354861] DMAR: No ATSR found
# [    0.354861] DMAR: No SATC found
# [    0.354862] DMAR: dmar0: Using Queued invalidation
# [    0.354864] DMAR: dmar1: Using Queued invalidation
# [    0.355195] DMAR: Intel(R) Virtualization Technology for Directed I/O
# 
# 2. Check your GPU detection (replace "nvidia" with your GPU vendor):
# lspci -nn | grep -i nvidia    # For NVIDIA GPUs
# lspci -nn | grep -i amd       # For AMD GPUs
# lspci -nn | grep -i "vga\|display"  # For all GPUs
# 
# Expected output for NVIDIA should show:
# 01:00.0 VGA compatible controller [0300]: NVIDIA Corporation TU102 [GeForce RTX 2080 Ti] [10de:1e04] (rev a1)
# 01:00.1 Audio device [0403]: NVIDIA Corporation TU102 High Definition Audio Controller [10de:10f7] (rev a1)
# 01:00.2 USB controller [0c03]: NVIDIA Corporation TU102 USB 3.1 Host Controller [10de:1ad6] (rev a1)
# 01:00.3 Serial bus controller [0c80]: NVIDIA Corporation TU102 USB Type-C UCSI Controller [10de:1ad7] (rev a1)
# 
# 3. Check if your GPU supports passthrough:
# lspci -nnk -d [vendor:device]
# Example: lspci -nnk -d 10de:1e04
# 
# If both IOMMU and GPU detection show expected output, your system is ready for GPU passthrough.
# 
# ⚠️  NOTE: If you don't see "DMAR: IOMMU enabled", you need to go back to Step 2 and ensure
# you used the correct IOMMU parameter for your CPU vendor.
)";
    
    showTweakInstructions("IOMMU and GPU Verification", instructions);
}

void MainWindow::on_verifyIommuConfigButton_clicked() {
    QString command = "dmesg | grep -e DMAR -e IOMMU && echo '---' && lspci -nn | grep -i nvidia";
    runSudoCommandInTerminal(command);
}

void MainWindow::on_verifyIommuBackupButton_clicked() {
    QString command = "dmesg | grep -e DMAR -e IOMMU > /tmp/iommu_verification.txt && lspci -nn | grep -i nvidia >> /tmp/iommu_verification.txt && echo 'Verification results saved to /tmp/iommu_verification.txt'";
    runSudoCommandInTerminal(command);
}

void MainWindow::on_isolateGpuToggle_clicked() {
    QString instructions = R"(
# GPU Isolation Using VFIO Instructions
# ====================================
# 
# This step isolates your GPU from the host system so it can be passed to a VM.
# 
# FIRST: Determine your GPU vendor and create the appropriate blacklist file:
# 
# 1. Check your GPU vendor:
#    lspci | grep -i "vga\|display"
#    - If it shows "NVIDIA" → use blacklist-nvidia.conf
#    - If it shows "AMD" → use blacklist-amd.conf
# 
# 2. For NVIDIA GPUs, create /etc/modprobe.d/blacklist-nvidia.conf:
blacklist nouveau
blacklist nvidia
blacklist nvidia_drm
blacklist nvidia_modeset

# 3. For AMD GPUs, create /etc/modprobe.d/blacklist-amd.conf:
blacklist amdgpu
blacklist radeon
blacklist amdgpu_drm
blacklist radeon_drm

# This prevents the host from loading GPU drivers, making the GPU available for passthrough.
# 
# After creating this file, you'll need to:
# 1. Reboot the system
# 2. Verify the GPU is no longer in use by the host
# 3. Proceed to the next step (Bind GPU to VFIO)
# 
# To verify the blacklist is working:
# For NVIDIA: lspci -nnk -d 10de:1e04
# For AMD: lspci -nnk -d 1002:xxxx (replace xxxx with your AMD device ID)
# Should show "Kernel driver in use: vfio-pci" instead of "nvidia" or "amdgpu"
# 
# TIP: If you have multiple GPUs, you may need to blacklist both vendors.
)";
    
    showTweakInstructions("GPU Isolation Configuration", instructions);
}

void MainWindow::on_isolateGpuConfigButton_clicked() {
    // Check if NVIDIA or AMD blacklist exists and open accordingly
    QFile nvidiaBlacklist("/etc/modprobe.d/blacklist-nvidia.conf");
    QFile amdBlacklist("/etc/modprobe.d/blacklist-amd.conf");
    
    if (nvidiaBlacklist.exists()) {
        openConfigInNano("/etc/modprobe.d/blacklist-nvidia.conf");
    } else if (amdBlacklist.exists()) {
        openConfigInNano("/etc/modprobe.d/blacklist-amd.conf");
    } else {
        // Ask user which type to create
        QMessageBox::StandardButton reply = QMessageBox::question(this, "GPU Type", 
            "Which GPU vendor do you want to blacklist?\n\n"
            "Yes = NVIDIA\n"
            "No = AMD", 
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            openConfigInNano("/etc/modprobe.d/blacklist-nvidia.conf");
        } else {
            openConfigInNano("/etc/modprobe.d/blacklist-amd.conf");
        }
    }
}

void MainWindow::on_isolateGpuBackupButton_clicked() {
    // Check if NVIDIA or AMD blacklist exists and backup accordingly
    QFile nvidiaBlacklist("/etc/modprobe.d/blacklist-nvidia.conf");
    QFile amdBlacklist("/etc/modprobe.d/blacklist-amd.conf");
    
    if (nvidiaBlacklist.exists()) {
        backupConfigFile("/etc/modprobe.d/blacklist-nvidia.conf", "NVIDIA Blacklist Configuration");
    } else if (amdBlacklist.exists()) {
        backupConfigFile("/etc/modprobe.d/blacklist-amd.conf", "AMD Blacklist Configuration");
    } else {
        QMessageBox::information(this, "No Blacklist Found", 
            "No GPU blacklist configuration found. Create one first using the Edit button.");
    }
}

void MainWindow::on_bindGpuToggle_clicked() {
    QString instructions = R"(
# Bind GPU to VFIO Instructions
# =============================
# 
# This step binds your GPU to the VFIO driver for passthrough.
# 
# FIRST: Find your actual GPU device IDs:
# 
# 1. Run this command to see your GPU and its associated devices:
#    lspci -nn | grep -i "vga\|display"
# 
# 2. For each GPU device, also check for associated audio/USB controllers:
#    lspci -nn | grep -i [vendor_name]
#    Example: lspci -nn | grep -i nvidia
# 
# 3. Extract the vendor:device IDs from the output:
#    Example output: 01:00.0 VGA compatible controller [0300]: NVIDIA Corporation ... [10de:1e04]
#    The ID is: 10de:1e04 (vendor:device format)
# 
# 4. Create /etc/modprobe.d/vfio.conf with your actual device IDs:

options vfio-pci ids=10de:1e04,10de:10f7,10de:1ad6,10de:1ad7

# Replace the IDs with your actual GPU and associated device IDs
# Include ALL devices associated with your GPU (VGA, Audio, USB controllers, etc.)
# 
# After creating this file:
# 1. Reboot the system
# 2. Verify the GPU is bound to VFIO:
#    lspci -nnk -d [your_vendor:device]
#    Should show "Kernel driver in use: vfio-pci"
# 
# ⚠️  IMPORTANT: You MUST reboot after this step before proceeding to the next step!
# 
# If successful, proceed to the next step (Load VFIO modules early)
)";
    
    showTweakInstructions("GPU VFIO Binding Configuration", instructions);
}

void MainWindow::on_bindGpuConfigButton_clicked() {
    openConfigInNano("/etc/modprobe.d/vfio.conf");
}

void MainWindow::on_bindGpuBackupButton_clicked() {
    backupConfigFile("/etc/modprobe.d/vfio.conf", "VFIO Configuration");
}

void MainWindow::on_loadVfioToggle_clicked() {
    QString instructions = R"(
# Load VFIO Modules Early Instructions
# ===================================
# 
# This ensures VFIO modules are loaded early in the boot process.
# 
# Create /etc/modules-load.d/vfio.conf with this content:

vfio
vfio_pci
vfio_iommu_type1
vfio_virqfd

# After creating this file:
# 1. Regenerate initramfs:
#    sudo mkinitcpio -P
# 
# 2. Reboot the system
# 
# 3. Verify VFIO modules are loaded:
#    lsmod | grep vfio
# 
# Should show:
# vfio_pci
# vfio_iommu_type1
# vfio_virqfd
# vfio
# 
# ⚠️  IMPORTANT: You MUST reboot after this step before proceeding to the next step!
# 
# If successful, proceed to the next step (Configure Huge Pages)
)";
    
    showTweakInstructions("VFIO Modules Configuration", instructions);
}

void MainWindow::on_loadVfioConfigButton_clicked() {
    openConfigInNano("/etc/modules-load.d/vfio.conf");
}

void MainWindow::on_loadVfioBackupButton_clicked() {
    backupConfigFile("/etc/modules-load.d/vfio.conf", "VFIO Modules Configuration");
}

void MainWindow::on_hugePagesToggle_clicked() {
    QString instructions = R"(
# Huge Pages Configuration Instructions
# ===================================
# 
# Huge pages improve VM performance by using larger memory pages.
# 
# ⚠️  IMPORTANT: Huge pages allocation is TEMPORARY and resets on each reboot!
# You need to re-allocate huge pages every time you boot your system.
# 
# To configure huge pages:
# 
# 1. Calculate how many GB you want to allocate (e.g., 16GB)
# 2. Convert to pages: GB * 1024 / 2 = pages (16GB = 8192 pages)
# 3. Allocate huge pages:
#    echo 8192 | sudo tee /proc/sys/vm/nr_hugepages
# 
# 4. Mount hugetlbfs if not already mounted:
#    sudo mkdir -p /dev/hugepages
#    sudo mount -t hugetlbfs none /dev/hugepages
# 
# 5. Use these values in your KVM XML:
#    <memory unit="KiB">16777216</memory>
#    <currentMemory unit="KiB">16777216</currentMemory>
#    <memoryBacking>
#      <hugepages/>
#    </memoryBacking>
# 
# To make huge pages persistent across reboots:
# Add to /etc/sysctl.conf:
# vm.nr_hugepages = 8192
# 
# To verify huge pages are working:
# cat /proc/meminfo | grep Huge
# 
# 💡 TIP: Create a script to automate huge pages allocation:
# 
# Save this script as ~/hugepages.sh and make it executable:
# chmod +x ~/hugepages.sh
# 
#!/bin/bash
 
 read -p "How many GB of HugePages to allocate? " gb
 
 mb=$((gb * 1024))
 
 page_size_mb=2
 
 pages=$((mb / page_size_mb))
 
 echo "Allocating $pages HugePages..."
 
 echo $pages | sudo tee /proc/sys/vm/nr_hugepages
 
 if ! mount | grep -q '/dev/hugepages'; then
   echo "Mounting hugetlbfs..."
   sudo mkdir -p /dev/hugepages
   sudo mount -t hugetlbfs none /dev/hugepages
 fi
 
 kib=$((gb * 1024 * 1024))
 echo "Use the following values in your KVM XML:"
 echo "  <memory unit=\"KiB\">$kib</memory>"
 echo "  <currentMemory unit=\"KiB\">$kib</currentMemory>"
 
# 🚀 PRO TIP: Add it as an alias for easy access:
# Add this line to your shell config (~/.bashrc, ~/.zshrc, etc.):
# alias hugepages='~/hugepages.sh'
# 
# Then you can simply run: hugepages
# 
# You can also add it to your startup applications or systemd service.
)";
    
    showTweakInstructions("Huge Pages Configuration", instructions);
}

void MainWindow::on_hugePagesConfigButton_clicked() {
    QString command = 
        "echo \"Huge Pages Configuration Script\"\n"
        "echo \"=============================\"\n"
        "echo \"⚠️  NOTE: This allocation is TEMPORARY and will reset on reboot!\"\n"
        "echo \"\"\n"
        "read -p \"How many GB of HugePages to allocate? \" gb\n"
        "mb=$((gb * 1024))\n"
        "page_size_mb=2\n"
        "pages=$((mb / page_size_mb))\n"
        "echo \"Allocating $pages HugePages...\"\n"
        "echo $pages | sudo tee /proc/sys/vm/nr_hugepages\n"
        "if ! mount | grep -q '/dev/hugepages'; then\n"
        "  echo \"Mounting hugetlbfs...\"\n"
        "  sudo mkdir -p /dev/hugepages\n"
        "  sudo mount -t hugetlbfs none /dev/hugepages\n"
        "fi\n"
        "kib=$((gb * 1024 * 1024))\n"
        "echo \"\"\n"
        "echo \"✅ Huge pages allocated successfully!\"\n"
        "echo \"\"\n"
        "echo \"Use the following values in your KVM XML:\"\n"
        "echo \"  <memory unit=\\\"KiB\\\">$kib</memory>\"\n"
        "echo \"  <currentMemory unit=\\\"KiB\\\">$kib</currentMemory>\"\n"
        "echo \"  <memoryBacking>\"\n"
        "echo \"    <hugepages/>\"\n"
        "echo \"  </memoryBacking>\"\n"
        "echo \"\"\n"
        "echo \"💡 To make this persistent, create ~/hugepages.sh with this content:\"\n"
        "echo \"#!/bin/bash\"\n"
        "echo \"read -p \\\"How many GB of HugePages to allocate? \\\" gb\"\n"
        "echo \"mb=\\\$((gb * 1024))\"\n"
        "echo \"page_size_mb=2\"\n"
        "echo \"pages=\\\$((mb / page_size_mb))\"\n"
        "echo \"echo \\\$pages | sudo tee /proc/sys/vm/nr_hugepages\"\n"
        "echo \"if ! mount | grep -q '/dev/hugepages'; then\"\n"
        "echo \"  sudo mkdir -p /dev/hugepages\"\n"
        "echo \"  sudo mount -t hugetlbfs none /dev/hugepages\"\n"
        "echo \"fi\"\n"
        "echo \"kib=\\$((gb * 1024 * 1024))\"\n"
        "echo \"echo \\\"Huge pages allocated: \\$kib KiB\\\"\"";
    runSudoCommandInTerminal(command);
}

void MainWindow::on_hugePagesBackupButton_clicked() {
    QString command = "cat /proc/sys/vm/nr_hugepages > /tmp/hugepages_backup.txt && mount | grep hugepages >> /tmp/hugepages_backup.txt && echo 'Huge pages configuration saved to /tmp/hugepages_backup.txt'";
    runSudoCommandInTerminal(command);
}

void MainWindow::on_openKvmBackupFolderButton_clicked() {
    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/CachyOsTools/backups/kvm";
    QDir().mkpath(backupDir);
    QProcess::startDetached("xdg-open", QStringList() << backupDir);
}