

void MainWindow::setupConfigFiles() {
    configFileList.clear();
    
    // System Configuration
    configFileList.append({"/etc/pacman.conf", "Pacman settings and enabling parallel downloads or custom repos (like chaotic-aur)", "System Configuration"});
    configFileList.append({"/etc/mkinitcpio.conf", "Initramfs settings, especially after installing kernel modules or changing hooks", "System Configuration"});
    configFileList.append({"/etc/default/grub", "GRUB bootloader settings (e.g., timeout, kernel params). Requires grub-mkconfig after edits", "System Configuration"});
    configFileList.append({"/etc/locale.gen", "Uncomment your locale (e.g., en_US.UTF-8) in locale.gen, then run locale-gen", "System Configuration"});
    configFileList.append({"/etc/locale.conf", "System locale configuration", "System Configuration"});
    configFileList.append({"/etc/vconsole.conf", "Console font and keymap (TTY)", "System Configuration"});
    configFileList.append({"/etc/hostname", "Set your system's hostname", "System Configuration"});
    configFileList.append({"/etc/hosts", "Matching entries for localhost", "System Configuration"});
    
    // User Environment
    configFileList.append({QDir::homePath() + "/.bashrc", "Shell configuration, aliases, prompt, etc.", "User Environment"});
    configFileList.append({QDir::homePath() + "/.zshrc", "Shell configuration, aliases, prompt, etc. (zsh is default in CachyOS)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.profile", "Environment variables for X11 sessions", "User Environment"});
    configFileList.append({QDir::homePath() + "/.xprofile", "Environment variables for X11 sessions", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/fish/config.fish", "If using fish shell", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/gtk-3.0/settings.ini", "GTK theme tweaks for GTK-based apps", "User Environment"});
    configFileList.append({QDir::homePath() + "/.config/xdg-user-dirs.conf", "User directory mappings (e.g., Downloads → ~/dl)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.Xresources", "X11 terminal font/colors (e.g., for urxvt)", "User Environment"});
    configFileList.append({QDir::homePath() + "/.Xdefaults", "X11 terminal font/colors (e.g., for urxvt)", "User Environment"});
    
    // Desktop Environment / Window Manager
    configFileList.append({QDir::homePath() + "/.config/kwinrc", "KWin window manager settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/kscreenlockerrc", "KDE Plasma screen locker settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/plasmarc", "KDE Plasma settings", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/plasmashellrc", "KDE Plasma shell settings", "Desktop Environment"});
    configFileList.append({"/etc/sddm.conf", "SDDM display manager theme and behavior", "Desktop Environment"});
    configFileList.append({QDir::homePath() + "/.config/compiz/", "Compiz settings, if using", "Desktop Environment"});
    
    // Performance and Kernel
    configFileList.append({"/etc/sysctl.d/", "Kernel parameters (e.g., vm.swappiness)", "Performance and Kernel"});
    configFileList.append({"/etc/modprobe.d/", "Module options (e.g., disabling nouveau)", "Performance and Kernel"});
    configFileList.append({"/etc/X11/xorg.conf.d/", "Manual GPU/input driver configs", "Performance and Kernel"});
    configFileList.append({"/etc/udev/rules.d/", "Custom udev rules (for devices, permissions)", "Performance and Kernel"});
    
    // Systemd and Services
    configFileList.append({QDir::homePath() + "/.config/systemd/user/", "User-specific services (e.g., overlay auto-launch)", "Systemd and Services"});
    configFileList.append({"/etc/systemd/system/", "System-wide services", "Systemd and Services"});
    configFileList.append({QDir::homePath() + "/.config/autostart/", "Apps to auto-start in GUI", "Systemd and Services"});
    
    // KVM/QEMU/Libvirt & GPU Passthrough
    configFileList.append({"/etc/default/grub", "Kernel parameters for IOMMU, vfio, GPU isolation, etc. (intel_iommu=on, amd_iommu=on, vfio-pci.ids, etc.)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/mkinitcpio.conf", "Add vfio modules to initramfs for passthrough (MODULES=(... vfio vfio_iommu_type1 vfio_pci vfio_virqfd))", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modprobe.d/vfio.conf", "Bind devices to vfio-pci for passthrough (options vfio-pci ids=...)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modprobe.d/blacklist.conf", "Blacklist drivers (nouveau, radeon, etc.) for passthrough devices", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/modules-load.d/vfio.conf", "Ensure vfio modules load at boot", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/grub.d/40_custom", "Custom GRUB entries or hooks for GPU selection", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/qemu.conf", "Libvirt QEMU daemon config (user/group, security, cgroup)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/qemu/<vmname>.xml", "VM-specific XML config (edit with virsh edit <vmname>)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/hooks/qemu", "Custom scripts for VM start/stop (GPU/PCI rebinding, audio switching)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/libvirt/hooks/qemu.d/", "Hook scripts for advanced automation", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/usr/share/OVMF/OVMF_VARS.fd", "UEFI NVRAM for Windows VMs (copy and edit for each VM if needed)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/udev/rules.d/", "Custom udev rules for device permissions, binding, or isolation", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/X11/xorg.conf.d/", "Manual GPU assignment, disabling X on passthrough GPU", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/sysctl.d/99-kvm.conf", "Kernel tweaks for virtualization (e.g., kernel.nmi_watchdog=0)", "KVM/QEMU/GPU Passthrough"});
    configFileList.append({"/etc/security/limits.conf", "Resource limits for QEMU/libvirt users", "KVM/QEMU/GPU Passthrough"});
    
    // Optional but Common on CachyOS
    configFileList.append({"/etc/reflector.conf", "Controls mirror selection by reflector", "CachyOS Specific"});
    configFileList.append({"/etc/xdg/reflector/reflector.conf", "Alternate location depending on version", "CachyOS Specific"});
    configFileList.append({"/etc/paru.conf", "AUR helper configuration (if using paru)", "CachyOS Specific"});
    configFileList.append({QDir::homePath() + "/.config/paru/paru.conf", "AUR helper configuration (if using paru)", "CachyOS Specific"});
}

void MainWindow::populateConfigTable() {
    ui->configTable->setRowCount(0);
    ui->configTable->setColumnCount(3);
    QStringList headers;
    headers << "File Path" << "Description" << "Edit";
    ui->configTable->setHorizontalHeaderLabels(headers);
    
    // Set column sizing behavior
    ui->configTable->horizontalHeader()->setStretchLastSection(false);
    ui->configTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // File Path
    ui->configTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Description stretches
    ui->configTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Edit button fixed width
    
    ui->configTable->setColumnWidth(2, 80); // Edit button width
    
    for (const ConfigFileEntry &entry : configFileList) {
        int row = ui->configTable->rowCount();
        ui->configTable->insertRow(row);
        
        // File path
        ui->configTable->setItem(row, 0, new QTableWidgetItem(entry.filePath));
        
        // Description
        ui->configTable->setItem(row, 1, new QTableWidgetItem(entry.description));
        
        // Edit button
        QPushButton *editButton = new QPushButton("Edit");
        editButton->setProperty("filePath", entry.filePath);
        connect(editButton, &QPushButton::clicked, [this, entry]() {
            editConfigFile(entry.filePath);
        });
        ui->configTable->setCellWidget(row, 2, editButton);
    }
}

void MainWindow::editConfigFile(const QString &filePath) {
    QString editor = ui->editorComboBox->currentText();
    
    // Check if file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, "File Not Found", 
                           QString("The file '%1' does not exist.\n\nThis might be a directory or the file hasn't been created yet.").arg(filePath));
        return;
    }
    
    // Prepare the command
    QString command;
    if (editor == "kate") {
        command = QString("kate --new '%1'").arg(filePath);
    } else {
        command = QString("%1 '%2'").arg(editor, filePath);
    }
    
    // Check if file needs sudo (system files)
    bool needsSudo = filePath.startsWith("/etc/") || filePath.startsWith("/usr/") || filePath.startsWith("/var/");
    
    if (needsSudo) {
        command = QString("sudo %1").arg(command);
    }
    
    // Open a new terminal window with the command
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    bool launched = false;
    
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << command)) {
            launched = true;
            break;
        }
    }
    
    if (!launched) {
        QMessageBox::warning(this, "Terminal Not Found", 
                           QString("Could not find a suitable terminal emulator.\n\nPlease install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
    }
}

void MainWindow::on_configTable_cellDoubleClicked(int row, int column) {
    if (column == 0 || column == 1) { // File path or description columns
        if (row >= 0 && row < configFileList.size()) {
            editConfigFile(configFileList[row].filePath);
        }
    }
}
