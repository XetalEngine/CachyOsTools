
// ISO Creator tab functions
void MainWindow::on_createIsoButton_clicked()
{
    // Use default values
    QString isoName = "cachyos-system-clone";
    QString outputDir = QDir::homePath() + "/iso";
    
    // Create output directory if it doesn't exist
    if (!QDir().exists(outputDir)) {
        if (!QDir().mkpath(outputDir)) {
            QMessageBox::warning(this, "Directory Creation Failed", 
                               "Failed to create output directory: " + outputDir + "\n\n"
                               "Please check permissions or try again.");
            return;
        }
    }
    
    // Confirm with user
    QString message = QString("This will create a system clone ISO with the following settings:\n\n"
                            "ISO Name: %1\n"
                            "Output Directory: %2\n\n"
                            "This process will:\n"
                            "• Create a complete snapshot of your system\n"
                            "• Build a bootable ISO with installer\n"
                            "• Take several minutes to complete\n\n"
                            "Do you want to continue?").arg(isoName, outputDir);
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm ISO Creation", 
                                                             message, 
                                                             QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Get sudo password once
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password", 
                                                "Enter your sudo password:", 
                                                QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        ui->createIsoButton->setEnabled(true);
        return;
    }
    
    // Disable button and show progress
    ui->createIsoButton->setEnabled(false);
    ui->isoProgressBar->setValue(0);
    ui->isoStatusLabel->setPlainText("Starting system clone ISO creation...\n");
    
    // Create and run the ISO creation script
    QString scriptPath = createIsoScript(isoName, outputDir, sudoPassword);
    if (scriptPath.isEmpty()) {
        ui->createIsoButton->setEnabled(true);
        return;
    }
    
    // Run the script with step-by-step monitoring
    QProcess *process = new QProcess(this);
    
    // Set up environment for sudo
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SUDO_ASKPASS", "/bin/echo");
    
    connect(process, &QProcess::started, [this]() {
        ui->isoStatusLabel->setPlainText("🚀 Starting system clone ISO creation...\n");
    });
    
    connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
        QString output = QString::fromUtf8(process->readAllStandardOutput());
        
        // Print raw output to console
        printf("%s", output.toUtf8().constData());
        fflush(stdout);
        
        // Update GUI with step summaries only
        QString currentStatus = ui->isoStatusLabel->toPlainText();
        
        if (output.contains("Preparing HOME-only build tree")) {
            ui->isoProgressBar->setValue(5);
            ui->isoStatusLabel->setPlainText("📁 Preparing build environment...\n");
        } else if (output.contains("Creating full-system snapshot")) {
            ui->isoProgressBar->setValue(10);
            ui->isoStatusLabel->setPlainText("📸 Creating system snapshot...\n");
        } else if (output.contains("Packing snapshot")) {
            ui->isoProgressBar->setValue(30);
            ui->isoStatusLabel->setPlainText("🗜️ Compressing system snapshot...\n");
        } else if (output.contains("Cleaning up temporary snapshot directory")) {
            ui->isoProgressBar->setValue(35);
            ui->isoStatusLabel->setPlainText("🧹 Cleaning up snapshot directory...\n");
        } else if (output.contains("Embedding snapshot")) {
            ui->isoProgressBar->setValue(50);
            ui->isoStatusLabel->setPlainText("🔧 Preparing ISO structure...\n");
        } else if (output.contains("Building ISO")) {
            ui->isoProgressBar->setValue(70);
            ui->isoStatusLabel->setPlainText("🏗️ Building bootable ISO...\n");
        } else if (output.contains("Final cleanup")) {
            ui->isoProgressBar->setValue(95);
            ui->isoStatusLabel->setPlainText("🧹 Final cleanup...\n");
        } else if (output.contains("ISO ready")) {
            ui->isoProgressBar->setValue(100);
            ui->isoStatusLabel->setPlainText("✅ ISO creation completed successfully!\n");
        }
        
        // Auto-scroll to bottom
        QTextCursor cursor = ui->isoStatusLabel->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->isoStatusLabel->setTextCursor(cursor);
        ui->isoStatusLabel->ensureCursorVisible();
    });
    
    // Also capture stderr for error messages
    connect(process, &QProcess::readyReadStandardError, [this, process]() {
        QString error = QString::fromUtf8(process->readAllStandardError());
        if (!error.trimmed().isEmpty()) {
            // Print error to console
            fprintf(stderr, "%s", error.toUtf8().constData());
            fflush(stderr);
            
            // Show error in GUI
            ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n❌ Error: " + error.trimmed() + "\n");
            
            // Auto-scroll to bottom
            QTextCursor cursor = ui->isoStatusLabel->textCursor();
            cursor.movePosition(QTextCursor::End);
            ui->isoStatusLabel->setTextCursor(cursor);
            ui->isoStatusLabel->ensureCursorVisible();
        }
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, scriptPath](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        ui->createIsoButton->setEnabled(true);
        //ui->estimateSizeButton->setEnabled(true);
        
        if (exitCode == 0) {
            ui->isoProgressBar->setValue(100);
            ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n✅ System clone ISO created successfully!");
            QMessageBox::information(this, "Success", 
                                   "System clone ISO has been created successfully!\n\n"
                                   "The ISO contains an exact copy of your current system "
                                   "and can be used to install it on other machines.");
        } else {
            ui->isoProgressBar->setValue(0);
            ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n❌ ISO creation failed. Check the output above for details.");
            QMessageBox::critical(this, "Error", 
                                "Failed to create ISO. Check the output above for details.\n\n"
                                "Exit code: " + QString::number(exitCode));
        }
        
        // Clean up script file
        QFile::remove(scriptPath);
    });
    
    connect(process, &QProcess::errorOccurred, [this, process, scriptPath](QProcess::ProcessError error) {
        process->deleteLater();
        ui->createIsoButton->setEnabled(true);
        //ui->estimateSizeButton->setEnabled(true);
        ui->isoProgressBar->setValue(0);
        ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n❌ Process error occurred");
        QMessageBox::critical(this, "Process Error", 
                            "Error running ISO creation script: " + QString::number(error));
        QFile::remove(scriptPath);
    });
    
    // Start the process with unbuffered output
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setProcessEnvironment(env);
    
    // Debug: Show script path
    ui->isoStatusLabel->setPlainText("Starting system clone ISO creation...\nScript path: " + scriptPath + "\nWaiting for output...\n");
    
    process->start("bash", QStringList() << scriptPath);
}





    // Helper function to create the ISO creation script
    QString MainWindow::createIsoScript(const QString &isoName, const QString &outputDir, const QString &sudoPassword)
{
    // Create temporary script file
    QString tempDir = QDir::tempPath();
    QString scriptPath = tempDir + "/create_iso_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".sh";
    
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Failed to create temporary script file");
        return QString();
    }
    
    QTextStream out(&scriptFile);
    
    // Write the bash script content
    out << "#!/usr/bin/env bash\n";
    out << "set -euo pipefail\n";
    out << "export PYTHONUNBUFFERED=1\n";
    out << "export FORCE_COLOR=1\n";
    out << "exec 1>&1\n";
    out << "exec 2>&1\n\n";
    
    out << "# Sudo password for the session\n";
    out << "SUDO_PASS=\"" << sudoPassword << "\"\n\n";
    
    out << "# Function to run sudo with password\n";
    out << "run_sudo() {\n";
    out << "    echo \"$SUDO_PASS\" | sudo -S \"$@\"\n";
    out << "}\n\n";
    
    out << "# === Paths ===\n";
    out << "BASE=\"$HOME/iso\"\n";
    out << "PROFILE=\"$BASE/xiso\"              # releng profile copy\n";
    out << "WORK=\"$BASE/xiso/work\"            # mkarchiso work dir\n";
    out << "OUT=\"$BASE/xiso/output\"           # ISO output dir\n";
    out << "RELENG=\"/usr/share/archiso/configs/releng\"\n\n";
    
    out << "SNAPDIR=\"$BASE/xiso/snapshot\"     # rsync snapshot (temp)\n";
    out << "SNAP_TAR=\"$BASE/xiso/rootfs-snapshot.tar.zst\"  # compressed snapshot\n\n";
    
    out << "# User output directory\n";
    out << "OUTPUT_DIR=\"" << outputDir << "\"\n\n";
    
    out << "# Everything must be under HOME\n";
    out << "case \"$BASE\" in \"$HOME\"/*) : ;; *) echo \"BASE must be under \\$HOME\"; exit 1 ;; esac\n\n";
    
    out << "echo \"[*] Preparing HOME-only build tree under $BASE\"\n";
    out << "mkdir -p \"$BASE\"\n";
    out << "cd \"$BASE\"\n";
    out << "rm -rf \"$PROFILE\"\n";
    out << "cp -r \"$RELENG\" \"$PROFILE\"\n\n";
    
    out << "# === Keep stock releng boot entries; just fix bootmodes to a single line ===\n";
    out << "sed -i 's/\\r$//' \"$PROFILE/profiledef.sh\"\n";
    out << "sed -i -E 's/^iso_name=.*/iso_name=\"archlinux\"/; s/^iso_label=.*/iso_label=\"ARCH_CLONE\"/' \"$PROFILE/profiledef.sh\"\n";
    out << "awk 'BEGIN{skip=0}\n";
    out << "     /^bootmodes=\\(/ {skip=1; next}\n";
    out << "     skip==1 { if ($0 ~ /\\)/) {skip=0; next} else next }\n";
    out << "     {print}' \"$PROFILE/profiledef.sh\" > /tmp/profiledef.$$ && mv /tmp/profiledef.$$ \"$PROFILE/profiledef.sh\"\n";
    out << "printf 'bootmodes=(bios.syslinux.mbr bios.syslinux.eltorito uefi-x64.systemd-boot.esp uefi-x64.systemd-boot.eltorito)\\n' >> \"$PROFILE/profiledef.sh\"\n\n";
    
    out << "# === Ensure live ISO has installer tools ===\n";
    out << "for p in rsync tar zstd grub efibootmgr parted gptfdisk e2fsprogs dosfstools; do\n";
    out << "  grep -qxF \"$p\" \"$PROFILE/packages.x86_64\" || echo \"$p\" >> \"$PROFILE/packages.x86_64\"\n";
    out << "done\n\n";
    
    out << "# === FULL SNAPSHOT of current system (EXACT STATE) ===\n";
    out << "echo \"[*] Creating full-system snapshot at $SNAPDIR …\"\n";
    out << "echo \"[*] Available space before snapshot:\"\n";
    out << "df -h . | tail -1\n";
    out << "run_sudo rm -rf \"$SNAPDIR\" \"$SNAP_TAR\"\n";
    out << "mkdir -p \"$SNAPDIR\"\n\n";
    
    out << "RSYNC_EXCLUDES=(\n";
    out << "  \"--exclude=/proc/*\" \"--exclude=/sys/*\" \"--exclude=/dev/*\"\n";
    out << "  \"--exclude=/run/*\"  \"--exclude=/tmp/*\" \"--exclude=/mnt/*\"\n";
    out << "  \"--exclude=/media/*\" \"--exclude=/lost+found\"\n";
    out << "  \"--exclude=$BASE/*\" \"--exclude=$SNAP_TAR\"\n";
    out << ")\n\n";
    
    out << "run_sudo rsync -aHAX --numeric-ids --delete \\\n";
    out << "  --info=progress2,stats2 \\\n";
    out << "  \"${RSYNC_EXCLUDES[@]}\" \\\n";
    out << "  / \"$SNAPDIR/\"\n\n";
    
    out << "# regenerate on target\n";
    out << "run_sudo rm -f \"$SNAPDIR/etc/machine-id\" \"$SNAPDIR/etc/fstab\" \"$SNAPDIR/var/lib/systemd/random-seed\" || true\n\n";
    
    out << "echo \"[*] Packing snapshot to $SNAP_TAR (zstd -19)…\"\n";
    out << "run_sudo tar --xattrs --acls --numeric-owner -C \"$SNAPDIR\" -I 'zstd -19 -T0' -cpf \"$SNAP_TAR\" .\n";
    out << "run_sudo chown \"$USER:$USER\" \"$SNAP_TAR\"\n";
    out << "echo \"[*] Snapshot compressed. Available space:\"\n";
    out << "df -h . | tail -1\n\n";
    
    out << "# free space - CRITICAL for small drives\n";
    out << "echo \"[*] Cleaning up temporary snapshot directory to free space...\"\n";
    out << "run_sudo rm -rf \"$SNAPDIR\"\n";
    out << "echo \"[*] Snapshot directory cleaned up. Available space:\"\n";
    out << "df -h . | tail -1\n\n";
    
    out << "# === Embed snapshot + installer into live ISO ===\n";
    out << "echo \"[*] Embedding snapshot and installer into profile airootfs …\"\n";
    out << "mkdir -p \"$PROFILE/airootfs/opt/clone\"\n";
    out << "cp \"$SNAP_TAR\" \"$PROFILE/airootfs/opt/clone/rootfs-snapshot.tar.zst\"\n\n";
    
    out << "INSTALLER_REL=\"/usr/local/bin/installer.sh\"\n";
    out << "mkdir -p \"$PROFILE/airootfs$(dirname \"$INSTALLER_REL\")\"\n";
    out << "cat > \"$PROFILE/airootfs$INSTALLER_REL\" <<'EOF'\n";
    out << "#!/usr/bin/env bash\n";
    out << "set -euo pipefail\n";
    out << "RED=$'\\e[31m'; GRN=$'\\e[32m'; YLW=$'\\e[33m'; BLU=$'\\e[34m'; CLR=$'\\e[0m'\n";
    out << "need(){ command -v \"$1\" >/dev/null 2>&1 || { echo \"${RED}Missing $1${CLR}\"; exit 1; }; }\n";
    out << "for b in lsblk parted mkfs.fat mkfs.ext4 mount umount genfstab arch-chroot grub-install grub-mkconfig efibootmgr rsync tar zstd; do need \"$b\"; done\n\n";
    
    out << "SNAP=\"/opt/clone/rootfs-snapshot.tar.zst\"\n";
    out << "[[ -f \"$SNAP\" ]] || { echo \"${RED}Snapshot not found at $SNAP${CLR}\"; exit 1; }\n\n";
    
    out << "echo \"${YLW}*** WARNING: This ERASES the selected disk (UEFI/GPT only). ***${CLR}\"\n";
    out << "lsblk -dpno NAME,SIZE,MODEL | sed 's/^/  /'\n";
    out << "read -rp \"Target DISK (e.g., /dev/nvme0n1 or /dev/sda): \" DISK\n";
    out << "[[ -b \"$DISK\" ]] || { echo \"${RED}Invalid disk${CLR}\"; exit 1; }\n";
    out << "echo \"${YLW}Type 'WIPE' to confirm: ${CLR}\"; read -r CONF; [[ \"$CONF\" == \"WIPE\" ]] || { echo \"${RED}Aborted.${CLR}\"; exit 1; }\n\n";
    
    out << "echo \"${BLU}[1/6] Partitioning GPT (ESP + ROOT)…${CLR}\"\n";
    out << "swapoff -a || true\n";
    out << "umount -R /mnt 2>/dev/null || true\n";
    out << "parted -s \"$DISK\" mklabel gpt\n";
    out << "parted -s \"$DISK\" mkpart ESP fat32 1MiB 513MiB\n";
    out << "parted -s \"$DISK\" set 1 esp on\n";
    out << "parted -s \"$DISK\" mkpart ROOT ext4 513MiB 100%\n\n";
    
    out << "# correct partition naming\n";
    out << "if [[ \"$DISK\" =~ (nvme|mmcblk) ]]; then ESP=\"${DISK}p1\"; ROOT=\"${DISK}p2\"; else ESP=\"${DISK}1\"; ROOT=\"${DISK}2\"; fi\n";
    out << "sleep 1\n\n";
    
    out << "echo \"${BLU}[2/6] Formatting…${CLR}\"\n";
    out << "mkfs.fat -F32 \"$ESP\"\n";
    out << "mkfs.ext4 -F \"$ROOT\"\n\n";
    
    out << "echo \"${BLU}[3/6] Mounting…${CLR}\"\n";
    out << "mount \"$ROOT\" /mnt\n";
    out << "mkdir -p /mnt/boot/efi\n";
    out << "mount \"$ESP\" /mnt/boot/efi\n\n";
    
    out << "echo \"${BLU}[4/6] Restoring snapshot to target…${CLR}\"\n";
    out << "tar --xattrs --acls --numeric-owner -I zstd -xpf \"$SNAP\" -C /mnt\n";
    out << "genfstab -U /mnt > /mnt/etc/fstab\n";
    out << ": > /mnt/etc/machine-id\n\n";
    
    out << "echo \"${BLU}[5/6] Finalize inside chroot…${CLR}\"\n";
    out << "arch-chroot /mnt /bin/bash -e <<CHROOT\n";
    out << "set -euo pipefail\n";
    out << "rm -f /etc/machine-id\n";
    out << "systemd-machine-id-setup\n";
    out << "rm -f /var/lib/systemd/random-seed || true\n\n";
    
    out << "mkdir -p /boot/efi\n";
    out << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ArchLinux\n\n";
    
    out << "# Remove stale root=/resume= from snapshot if present (new UUIDs)\n";
    out << "sed -i -E 's/(GRUB_CMDLINE_LINUX(_DEFAULT)?=\"[^\"]*)\\s*root=[^\" ]+/\\1/g; s/(GRUB_CMDLINE_LINUX(_DEFAULT)?=\"[^\"]*)\\s*resume=[^\" ]+/\\1/g; s/  */ /g' /etc/default/grub\n";
    out << "grub-mkconfig -o /boot/grub/grub.cfg\n\n";
    
    out << "if command -v mkinitcpio >/dev/null 2>&1; then mkinitcpio -P || true; fi\n";
    out << "if command -v systemctl   >/dev/null 2>&1; then systemctl enable NetworkManager || true; fi\n";
    out << "CHROOT\n\n";
    
    out << "echo \"${BLU}[6/6] Done. You can reboot into your cloned system.${CLR}\"\n";
    out << "echo \"${GRN}Installation complete.${CLR}\"\n";
    out << "echo \"Run:   umount -R /mnt && reboot\"\n";
    out << "EOF\n";
    out << "chmod +x \"$PROFILE/airootfs$INSTALLER_REL\"\n\n";
    
    out << "# === Autologin & autorun installer on TTY1 ===\n";
    out << "mkdir -p \"$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d\" \"$PROFILE/airootfs/root\"\n";
    out << "cat > \"$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d/override.conf\" <<'EOF'\n";
    out << "[Service]\n";
    out << "ExecStart=\n";
    out << "ExecStart=-/sbin/agetty --autologin root --noclear %I $TERM\n";
    out << "Type=idle\n";
    out << "EOF\n";
    out << "cat > \"$PROFILE/airootfs/root/.bash_profile\" <<'EOF'\n";
    out << "if [[ -z \"$DISPLAY\" ]] && [[ $(tty) == /dev/tty1 ]]; then\n";
    out << "  clear\n";
    out << "  echo \"Launching EXACT clone installer...\"\n";
    out << "  /usr/local/bin/installer.sh || bash\n";
    out << "fi\n";
    out << "EOF\n\n";
    
    out << "# === Build ISO ===\n";
    out << "echo \"[*] Building ISO with mkarchiso (home-only dirs)…\"\n";
    out << "echo \"[*] Available space before ISO build:\"\n";
    out << "df -h . | tail -1\n";
    out << "run_sudo rm -rf \"$WORK\" \"$OUT\"\n";
    out << "mkdir -p \"$OUT\"\n";
    out << "run_sudo mkarchiso -v -w \"$WORK\" -o \"$OUT\" \"$PROFILE\"\n\n";
    
    out << "echo\n";
    out << "echo \"[✓] ISO ready in: $OUT\"\n";
    out << "ls -lh \"$OUT\"/*.iso || true\n";
    out << "echo \"[*] Final cleanup - removing work directory:\"\n";
    out << "run_sudo rm -rf \"$WORK\"\n";
  
    
    scriptFile.close();
    
    // Make script executable
    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFile::ExeOwner);
    
    return scriptPath;
}

// Helper function to format size in human readable format
QString MainWindow::formatSize(qint64 bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unitIndex]);
}
