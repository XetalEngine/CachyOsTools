
// ISO Creator tab functions
void MainWindow::on_browseIsoOutputButton_clicked()
{
    // Default ISO output directory
    QString outputDir = QDir::homePath() + "/iso/xiso/output";

    // Create the directory if it doesn't exist
    if (!QDir().exists(outputDir)) {
        if (!QDir().mkpath(outputDir)) {
            QMessageBox::warning(this, tr("Directory Creation Failed"),
                                 tr("Failed to create output directory: ") + outputDir + "\n\n"
                                 "Please check permissions or try again.");
            return;
        }
    }

    // Use dolphin to browse to the ISO output folder
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [process](int exitCode, QProcess::ExitStatus exitStatus) {
                process->deleteLater();
            });

    process->start("dolphin", QStringList() << outputDir);
}

void MainWindow::on_createIsoButton_clicked()
{
    // Use default values
    QString isoName = "cachyos-system-clone";
    QString outputDir = QDir::homePath() + "/iso/xiso/output";

    // Create output directory if it doesn't exist
    if (!QDir().exists(outputDir)) {
        if (!QDir().mkpath(outputDir)) {
            QMessageBox::warning(this, tr("Directory Creation Failed"),
                                 tr("Failed to create output directory: ") + outputDir + "\n\n"
                                 "Please check permissions or try again.");
            return;
        }
    }

    // Gather exclusions from the two panels
    QStringList excludePaths = collectIsoExcludePaths();

    // Gather first-boot hardware adaptation options (all off = exact clone)
    IsoFirstBootOptions firstBoot;
    if (!ui->isoExactCloneCheck->isChecked()) {
        firstBoot.fixNetwork = ui->isoCompatNetworkCheck->isChecked();
        firstBoot.fixGpu = ui->isoCompatGpuCheck->isChecked();
        firstBoot.changeUser = ui->isoCompatUserCheck->isChecked();
        firstBoot.regenSsh = ui->isoCompatSshCheck->isChecked();
    }

    // Confirm with user
    QString excludeSummary = excludePaths.isEmpty()
        ? "• Nothing excluded (complete system)\n"
        : QString("• %1 folder(s)/file(s) EXCLUDED from the ISO\n").arg(excludePaths.size());
    if (firstBoot.any()) {
        QStringList opts;
        if (firstBoot.fixNetwork) opts << "network";
        if (firstBoot.fixGpu) opts << "GPU";
        if (firstBoot.changeUser) opts << "user/password";
        if (firstBoot.regenSsh) opts << "SSH keys";
        excludeSummary += QString("• First-boot hardware adaptation: %1\n").arg(opts.join(", "));
    } else {
        excludeSummary += "• Exact clone mode (no first-boot changes)\n";
    }
    QString message = QString("This will create a system clone ISO with the following settings:\n\n"
    "ISO Name: %1\n"
    "Output Directory: %2\n\n"
    "This process will:\n"
    "• Create a complete snapshot of your system\n"
    "%3"
    "• Build a bootable ISO with an auto-launching installer\n"
    "• Take several minutes to complete\n\n"
    "Do you want to continue?").arg(isoName, outputDir, excludeSummary);

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm ISO Creation",
                                                              message,
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    // Get sudo password once
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, tr("Sudo Password"),
                                                 tr("Enter your sudo password:"),
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        ui->createIsoButton->setEnabled(true);
        return;
    }

    // Disable button and show progress
    ui->createIsoButton->setEnabled(false);
    ui->isoProgressBar->setValue(0);
    ui->isoStatusLabel->setPlainText("Starting system clone ISO creation...\n");

    // Check if offline mode is selected and package is available
    bool offlineMode = ui->offlineModeRadio->isChecked();
    if (offlineMode) {
        QFileInfo fileInfo(offlinePackagePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            QMessageBox::critical(this, "Offline Package Not Found",
                                  QString("Offline mode is selected but the package file '%1' was not found.\n\n"
                                  "Please download the offline package first or switch to online mode.").arg(OFFLINE_PACKAGE_FILENAME));
            ui->createIsoButton->setEnabled(true);
            return;
        }
    }

    // Stage the XETAL ENGINE logo (embedded resource) so the build script can
    // put it inside the ISO for the installer splash screen
    QString logoStagePng = QDir::homePath() + "/iso/XetalEngine.png";
    QString logoStageBmp = QDir::homePath() + "/iso/XetalLogo.bmp";
    QDir().mkpath(QDir::homePath() + "/iso");
    QFile::remove(logoStagePng);
    QFile::remove(logoStageBmp);
    QFile::copy(":/images/XetalEngine.png", logoStagePng);
    QFile::copy(":/images/XetalLogo.bmp", logoStageBmp);

    // Create and run the ISO creation script
    QString scriptPath = createIsoScript(isoName, outputDir, sudoPassword, offlineMode, excludePaths, firstBoot);
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
                    QMessageBox::information(this, tr("Success"),
                                             tr("System clone ISO has been created successfully!\n\n"
                                             "The ISO contains an exact copy of your current system "
                                             "and can be used to install it on other machines."));
                } else {
                    ui->isoProgressBar->setValue(0);
                    ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n❌ ISO creation failed. Check the output above for details.");
                    QMessageBox::critical(this, tr("Error"),
                                          tr("Failed to create ISO. Check the output above for details.\n\n"
                                          "Exit code: ") + QString::number(exitCode));
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
        QMessageBox::critical(this, tr("Process Error"),
                              tr("Error running ISO creation script: ") + QString::number(error));
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
QString MainWindow::createIsoScript(const QString &isoName, const QString &outputDir, const QString &sudoPassword, bool offlineMode,
                                    const QStringList &excludePaths, const IsoFirstBootOptions &firstBoot)
{
    // Create temporary script file
    QString tempDir = QDir::tempPath();
    QString scriptPath = tempDir + "/create_iso_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".sh";

    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to create temporary script file"));
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

    // Add offline mode setup if enabled
    if (offlineMode) {
        out << "# === Offline Mode Setup ===\n";
        out << "OFFLINE_PACKAGE_NEW=\"" << QDir::currentPath() << "/" << OFFLINE_PACKAGE_FILENAME << "\"\n";
        out << "OFFLINE_PACKAGE_OLD=\"" << QDir::currentPath() << "/offline-iso-packages.tar.gz\"\n";
        out << "OFFLINE_CACHE_DIR=\"$BASE/xiso/offline-cache\"\n\n";
        out << "echo \"[*] Setting up offline mode...\"\n";
        out << "if [[ -f \"$OFFLINE_PACKAGE_NEW\" ]]; then\n";
        out << "    OFFLINE_PACKAGE=\"$OFFLINE_PACKAGE_NEW\"\n";
        out << "    echo \"[*] Using complete offline package: $OFFLINE_PACKAGE\"\n";
        out << "elif [[ -f \"$OFFLINE_PACKAGE_OLD\" ]]; then\n";
        out << "    OFFLINE_PACKAGE=\"$OFFLINE_PACKAGE_OLD\"\n";
        out << "    echo \"[*] Using legacy offline package: $OFFLINE_PACKAGE\"\n";
        out << "else\n";
        out << "    echo \"[ERROR] No offline package found. Expected one of:\"\n";
        out << "    echo \"[ERROR]   $OFFLINE_PACKAGE_NEW\"\n";
        out << "    echo \"[ERROR]   $OFFLINE_PACKAGE_OLD\"\n";
        out << "    exit 1\n";
        out << "fi\n\n";
        out << "echo \"[*] Extracting offline package cache...\"\n";
        out << "mkdir -p \"$OFFLINE_CACHE_DIR\"\n";
        out << "echo \"[*] Package file size: $(ls -lh \"$OFFLINE_PACKAGE\" | awk '{print $5}')\"\n";
        out << "tar -xzf \"$OFFLINE_PACKAGE\" -C \"$OFFLINE_CACHE_DIR\"\n";
        out << "if [[ $? -ne 0 ]]; then\n";
        out << "    echo \"[ERROR] Failed to extract offline package\"\n";
        out << "    echo \"[ERROR] Please check if the package file is corrupted\"\n";
        out << "    exit 1\n";
        out << "fi\n";
        out << "echo \"[*] Extracted contents:\"\n";
        out << "ls -la \"$OFFLINE_CACHE_DIR/offline-packages/\" 2>/dev/null || echo \"[WARNING] Expected directory structure not found\"\n\n";
        out << "echo \"[*] Setting up local package cache...\"\n";
        out << "mkdir -p \"/var/cache/pacman/pkg\"\n";
        out << "if [[ -d \"$OFFLINE_CACHE_DIR/offline-packages/pkg\" ]]; then\n";
        out << "    cp -r \"$OFFLINE_CACHE_DIR/offline-packages/pkg/\"* \"/var/cache/pacman/pkg/\" 2>/dev/null || true\n";
        out << "    echo \"[*] Packages copied successfully\"\n";
        out << "else\n";
        out << "    echo \"[WARNING] Package directory not found in offline package\"\n";
        out << "fi\n";
        out << "echo \"[*] Setting up local package databases...\"\n";
        out << "mkdir -p \"/var/lib/pacman/sync\"\n";
        out << "if [[ -d \"$OFFLINE_CACHE_DIR/offline-packages/sync\" ]]; then\n";
        out << "    cp -r \"$OFFLINE_CACHE_DIR/offline-packages/sync/\"* \"/var/lib/pacman/sync/\" 2>/dev/null || true\n";
        out << "    echo \"[*] Package databases copied successfully\"\n";
        out << "else\n";
        out << "    echo \"[WARNING] Package databases directory not found in offline package\"\n";
        out << "fi\n";
        out << "echo \"[*] Offline mode setup complete\"\n\n";
    }

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
    out << "# squashfs with zstd instead of releng's xz: 3-6x faster build (and faster\n";
    out << "# live boot). Matters hugely here because the airootfs contains the big\n";
    out << "# already-compressed snapshot tar, which xz would slowly re-compress for nothing.\n";
    out << "sed -i -E \"s/^airootfs_image_tool_options=.*/airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '15' '-b' '1M')/\" \"$PROFILE/profiledef.sh\"\n";
    out << "awk 'BEGIN{skip=0}\n";
    out << "     /^bootmodes=\\(/ {skip=1; next}\n";
    out << "     skip==1 { if ($0 ~ /\\)/) {skip=0; next} else next }\n";
    out << "     {print}' \"$PROFILE/profiledef.sh\" > /tmp/profiledef.$$ && mv /tmp/profiledef.$$ \"$PROFILE/profiledef.sh\"\n";
    out << "printf 'bootmodes=(bios.syslinux uefi.systemd-boot)\\n' >> \"$PROFILE/profiledef.sh\"\n\n";

    out << "# mkarchiso copies airootfs with --no-preserve=mode, so exec bits are LOST\n";
    out << "# unless declared in profiledef.sh's file_permissions array. Without this,\n";
    out << "# /xetal.sh boots as non-executable and the installer can't auto-start.\n";
    out << "sed -i 's|^file_permissions=(|file_permissions=(\\n  [\"/xetal.sh\"]=\"0:0:755\"\\n  [\"/usr/local/bin/installer.sh\"]=\"0:0:755\"|' \"$PROFILE/profiledef.sh\"\n";
    out << "grep -q '\"/xetal.sh\"' \"$PROFILE/profiledef.sh\" || printf 'file_permissions=(\\n  [\"/xetal.sh\"]=\"0:0:755\"\\n  [\"/usr/local/bin/installer.sh\"]=\"0:0:755\"\\n)\\n' >> \"$PROFILE/profiledef.sh\"\n\n";

    out << "# === Ensure live ISO has installer tools (dialog = TUI, chafa = logo rendering) ===\n";
    out << "for p in rsync tar zstd grub efibootmgr parted gptfdisk e2fsprogs dosfstools dialog chafa; do\n";
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
    if (!excludePaths.isEmpty()) {
        out << "  # User-selected exclusions from the ISO Creator tab.\n";
        out << "  # Folders use \"/*\" so the folder itself survives (empty) in the ISO —\n";
        out << "  # only its contents are dropped. Files are excluded outright.\n";
        for (const QString &path : excludePaths) {
            QString escaped = path;
            escaped.replace("\\", "\\\\").replace("\"", "\\\"").replace("$", "\\$");
            if (QFileInfo(path).isDir()) {
                out << "  \"--exclude=" << escaped << "/*\"\n";
            } else {
                out << "  \"--exclude=" << escaped << "\"\n";
            }
        }
    }
    out << ")\n\n";
    if (!excludePaths.isEmpty()) {
        out << "echo \"[*] Excluding " << excludePaths.size() << " user-selected path(s) from the snapshot\"\n\n";
    }

    out << "# rsync exit 23/24 = partial transfer (unreadable/vanished files) — normal when\n";
    out << "# snapshotting a live system, so warn and continue instead of aborting the build.\n";
    out << "set +e\n";
    out << "run_sudo rsync -aHAX --numeric-ids --delete \\\n";
    out << "  --info=progress2,stats2 \\\n";
    out << "  \"${RSYNC_EXCLUDES[@]}\" \\\n";
    out << "  / \"$SNAPDIR/\"\n";
    out << "RSYNC_RC=$?\n";
    out << "set -e\n";
    out << "if [[ $RSYNC_RC -ne 0 && $RSYNC_RC -ne 23 && $RSYNC_RC -ne 24 ]]; then\n";
    out << "    echo \"[ERROR] rsync failed with exit code $RSYNC_RC - aborting.\"\n";
    out << "    exit \"$RSYNC_RC\"\n";
    out << "fi\n";
    out << "if [[ $RSYNC_RC -ne 0 ]]; then\n";
    out << "    echo \"[WARNING] Some files could not be read and were SKIPPED (rsync code $RSYNC_RC).\"\n";
    out << "    echo \"[WARNING] See the errors above for which files. Continuing with the build...\"\n";
    out << "fi\n\n";

    out << "# regenerate on target\n";
    out << "run_sudo rm -f \"$SNAPDIR/etc/machine-id\" \"$SNAPDIR/etc/fstab\" \"$SNAPDIR/var/lib/systemd/random-seed\" || true\n\n";

    // zstd -10 instead of -19: several times faster on all cores for a ~5%
    // size penalty; the frame format (and the installer's decompress) is identical.
    out << "echo \"[*] Packing snapshot to $SNAP_TAR (zstd -10, all cores)…\"\n";
    out << "run_sudo tar --xattrs --acls --numeric-owner -C \"$SNAPDIR\" -I 'zstd -10 -T0' -cpf \"$SNAP_TAR\" .\n";
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
    out << "cp \"$SNAP_TAR\" \"$PROFILE/airootfs/opt/clone/rootfs-snapshot.tar.zst\"\n";
    out << "# XETAL ENGINE logo for the installer splash (staged by the app)\n";
    out << "[[ -f \"$BASE/XetalEngine.png\" ]] && cp \"$BASE/XetalEngine.png\" \"$PROFILE/airootfs/opt/clone/logo.png\"\n";
    out << "[[ -f \"$BASE/XetalLogo.bmp\" ]] && cp \"$BASE/XetalLogo.bmp\" \"$PROFILE/airootfs/opt/clone/logo.bmp\"\n\n";

    if (firstBoot.any()) {
        out << "# === First-boot hardware adaptation (selected in the app) ===\n";
        out << "cat > \"$PROFILE/airootfs/opt/clone/firstboot.conf\" <<'FBCONF'\n";
        out << "FIX_NETWORK=" << (firstBoot.fixNetwork ? "1" : "0") << "\n";
        out << "FIX_GPU=" << (firstBoot.fixGpu ? "1" : "0") << "\n";
        out << "CHANGE_USER=" << (firstBoot.changeUser ? "1" : "0") << "\n";
        out << "REGEN_SSH=" << (firstBoot.regenSsh ? "1" : "0") << "\n";
        out << "FBCONF\n\n";

        out << "cat > \"$PROFILE/airootfs/opt/clone/xetal-firstboot.service\" <<'FBUNIT'\n";
        out << "[Unit]\n";
        out << "Description=XETAL ENGINE first-boot hardware adaptation\n";
        out << "ConditionPathExists=/etc/xetal-firstboot.conf\n";
        out << "Wants=NetworkManager.service\n";
        out << "After=NetworkManager.service systemd-user-sessions.service\n";
        out << "Before=display-manager.service getty@tty1.service\n";
        out << "\n";
        out << "[Service]\n";
        out << "Type=oneshot\n";
        out << "StandardInput=tty\n";
        out << "StandardOutput=tty\n";
        out << "TTYPath=/dev/tty1\n";
        out << "TTYReset=yes\n";
        out << "TTYVHangup=yes\n";
        out << "ExecStart=/usr/local/bin/xetal-firstboot.sh\n";
        out << "TimeoutStartSec=0\n";
        out << "\n";
        out << "[Install]\n";
        out << "WantedBy=multi-user.target\n";
        out << "FBUNIT\n\n";

        out << "cat > \"$PROFILE/airootfs/opt/clone/xetal-firstboot.sh\" <<'FBSH'\n";
        out << "#!/usr/bin/env bash\n";
        out << "CONF=/etc/xetal-firstboot.conf\n";
        out << "LOG=/var/log/xetal-firstboot.log\n";
        out << "[ -f \"$CONF\" ] || exit 0\n";
        out << ". \"$CONF\"\n";
        out << "exec > >(tee -a \"$LOG\") 2>&1\n";
        out << "GRN=$'\\e[32m'; RED=$'\\e[31m'; YLW=$'\\e[33m'; CLR=$'\\e[0m'\n";
        out << "echo \"\"\n";
        out << "echo \"${GRN}=== XETAL ENGINE first-boot hardware adaptation ===${CLR}\"\n";
        out << "ok=1\n";
        out << "\n";
        out << "# --- d) SSH host keys ---\n";
        out << "if [ \"${REGEN_SSH:-0}\" = \"1\" ]; then\n";
        out << "  echo \"[ssh] Regenerating SSH host keys (new machine identity)...\"\n";
        out << "  rm -f /etc/ssh/ssh_host_*\n";
        out << "  if command -v ssh-keygen >/dev/null 2>&1; then ssh-keygen -A >/dev/null && echo \"[ssh] done\"; else echo \"[ssh] openssh not installed - skipped\"; fi\n";
        out << "fi\n";
        out << "\n";
        out << "# --- a) Network adaptation ---\n";
        out << "if [ \"${FIX_NETWORK:-0}\" = \"1\" ]; then\n";
        out << "  echo \"[net] Adapting network configuration to this hardware...\"\n";
        out << "  if command -v nmcli >/dev/null 2>&1; then\n";
        out << "    for uuid in $(nmcli -t -f UUID connection show 2>/dev/null); do\n";
        out << "      ifname=$(nmcli -g connection.interface-name connection show uuid \"$uuid\" 2>/dev/null)\n";
        out << "      [ -n \"$ifname\" ] || continue\n";
        out << "      ip link show \"$ifname\" >/dev/null 2>&1 && continue\n";
        out << "      master=$(nmcli -g connection.master connection show uuid \"$uuid\" 2>/dev/null)\n";
        out << "      ctype=$(nmcli -g connection.type connection show uuid \"$uuid\" 2>/dev/null)\n";
        out << "      if [ -n \"$master\" ]; then\n";
        out << "        echo \"[net] removing stale bridge-slave for missing '$ifname'\"\n";
        out << "        nmcli connection delete uuid \"$uuid\" >/dev/null 2>&1\n";
        out << "      elif [ \"$ctype\" = \"802-3-ethernet\" ]; then\n";
        out << "        echo \"[net] unbinding wired profile from missing '$ifname' (will match any NIC)\"\n";
        out << "        nmcli connection modify uuid \"$uuid\" connection.interface-name \"\" >/dev/null 2>&1\n";
        out << "      fi\n";
        out << "    done\n";
        out << "    eth=\"\"\n";
        out << "    for d in $(nmcli -t -f DEVICE,TYPE device 2>/dev/null | awk -F: '$2==\"ethernet\"{print $1}'); do\n";
        out << "      eth=\"$d\"\n";
        out << "      [ \"$(cat /sys/class/net/$d/carrier 2>/dev/null)\" = \"1\" ] && break\n";
        out << "    done\n";
        out << "    wifi=$(nmcli -t -f DEVICE,TYPE device 2>/dev/null | awk -F: '$2==\"wifi\"{print $1; exit}')\n";
        out << "    nmcli -t -f NAME,TYPE connection show 2>/dev/null | awk -F: '$2==\"bridge\"{print $1}' | while read -r br; do\n";
        out << "      bruuid=$(nmcli -t -g connection.uuid connection show \"$br\" 2>/dev/null | head -1)\n";
        out << "      have=\"\"\n";
        out << "      for uuid in $(nmcli -t -f UUID connection show); do\n";
        out << "        m=$(nmcli -g connection.master connection show uuid \"$uuid\" 2>/dev/null)\n";
        out << "        if [ \"$m\" = \"$br\" ]; then have=1; elif [ -n \"$bruuid\" ] && [ \"$m\" = \"$bruuid\" ]; then have=1; fi\n";
        out << "      done\n";
        out << "      if [ -z \"$have\" ] && [ -n \"$eth\" ]; then\n";
        out << "        echo \"[net] attaching bridge '$br' to $eth\"\n";
        out << "        nmcli connection add type bridge-slave ifname \"$eth\" master \"$br\" >/dev/null 2>&1\n";
        out << "      elif [ -z \"$have\" ] && [ -n \"$wifi\" ]; then\n";
        out << "        echo \"[net] no ethernet found - switching bridge '$br' to shared/NAT mode over Wi-Fi\"\n";
        out << "        nmcli connection modify \"$br\" ipv4.method shared ipv6.method ignore >/dev/null 2>&1\n";
        out << "      fi\n";
        out << "    done\n";
        out << "    echo \"[net] done\"\n";
        out << "  else\n";
        out << "    echo \"[net] NetworkManager not present - skipped\"\n";
        out << "  fi\n";
        out << "fi\n";
        out << "\n";
        out << "# --- b) GPU adaptation ---\n";
        out << "if [ \"${FIX_GPU:-0}\" = \"1\" ]; then\n";
        out << "  echo \"[gpu] Detecting graphics hardware...\"\n";
        out << "  gpuinfo=$(lspci -nn 2>/dev/null | grep -Ei 'vga|3d controller|display')\n";
        out << "  echo \"[gpu] $gpuinfo\"\n";
        out << "  if echo \"$gpuinfo\" | grep -qi nvidia; then\n";
        out << "    echo \"[gpu] NVIDIA detected - leaving the existing setup untouched (bulletproof).\"\n";
        out << "  elif echo \"$gpuinfo\" | grep -Eqi 'amd|ati|radeon'; then\n";
        out << "    echo \"[gpu] AMD detected - installing drivers and configuring X11 + Wayland...\"\n";
        out << "    command -v nm-online >/dev/null 2>&1 && nm-online -t 45 >/dev/null 2>&1\n";
        out << "    pkgs=\"mesa vulkan-radeon libva-mesa-driver xf86-video-amdgpu\"\n";
        out << "    grep -q \"^\\[multilib\\]\" /etc/pacman.conf 2>/dev/null && pkgs=\"$pkgs lib32-mesa lib32-vulkan-radeon\"\n";
        out << "    if pacman -Sy --noconfirm --needed $pkgs; then\n";
        out << "      echo \"[gpu] AMD driver packages installed.\"\n";
        out << "    else\n";
        out << "      echo \"${YLW}[gpu] Package install failed (no internet yet?) - will retry on next boot.${CLR}\"\n";
        out << "      ok=0\n";
        out << "    fi\n";
        out << "    mkdir -p /etc/X11/xetal-quarantine\n";
        out << "    for f in /etc/X11/xorg.conf /etc/X11/xorg.conf.d/*.conf; do\n";
        out << "      [ -f \"$f\" ] || continue\n";
        out << "      if grep -qi 'driver.*\"nvidia\"' \"$f\"; then\n";
        out << "        echo \"[gpu] quarantining X11 config that forces nvidia: $f\"\n";
        out << "        mv \"$f\" /etc/X11/xetal-quarantine/\n";
        out << "      fi\n";
        out << "    done\n";
        out << "    if [ -f /etc/environment ]; then\n";
        out << "      sed -i -E 's/^(GBM_BACKEND=nvidia)/#\\1/; s/^(__GLX_VENDOR_LIBRARY_NAME=nvidia)/#\\1/' /etc/environment\n";
        out << "    fi\n";
        out << "    [ -f /etc/gdm/custom.conf ] && sed -i 's/^WaylandEnable=false/#WaylandEnable=false/' /etc/gdm/custom.conf\n";
        out << "    for f in /etc/modprobe.d/*.conf; do\n";
        out << "      [ -f \"$f\" ] && sed -i 's/^blacklist amdgpu/#blacklist amdgpu/' \"$f\"\n";
        out << "    done\n";
        out << "    echo \"[gpu] X11 configs cleaned and Wayland re-enabled where applicable.\"\n";
        out << "  else\n";
        out << "    echo \"[gpu] Intel/other GPU detected - mesa already covers it, nothing to do.\"\n";
        out << "  fi\n";
        out << "fi\n";
        out << "\n";
        out << "# --- c) Username / password (interactive on the console) ---\n";
        out << "if [ \"${CHANGE_USER:-0}\" = \"1\" ]; then\n";
        out << "  olduser=$(awk -F: '$3>=1000 && $3<60000 && $1!=\"nobody\"{print $1; exit}' /etc/passwd)\n";
        out << "  if [ -n \"$olduser\" ]; then\n";
        out << "    echo \"\"\n";
        out << "    echo \"${GRN}--- User account setup ---${CLR}\"\n";
        out << "    echo \"Current main user: $olduser\"\n";
        out << "    printf \"New username (press Enter to keep '%s'): \" \"$olduser\"\n";
        out << "    read -r newuser || newuser=\"\"\n";
        out << "    if [ -n \"$newuser\" ] && [ \"$newuser\" != \"$olduser\" ]; then\n";
        out << "      if usermod -l \"$newuser\" \"$olduser\" 2>/dev/null; then\n";
        out << "        groupmod -n \"$newuser\" \"$olduser\" 2>/dev/null\n";
        out << "        usermod -d \"/home/$newuser\" -m \"$newuser\" 2>/dev/null\n";
        out << "        echo \"[user] renamed '$olduser' -> '$newuser'\"\n";
        out << "        olduser=\"$newuser\"\n";
        out << "      else\n";
        out << "        echo \"${RED}[user] rename failed - keeping '$olduser'${CLR}\"\n";
        out << "      fi\n";
        out << "    fi\n";
        out << "    echo \"Set a new password for '$olduser' (Ctrl+C to keep the current one):\"\n";
        out << "    passwd \"$olduser\" || echo \"[user] password unchanged\"\n";
        out << "  fi\n";
        out << "fi\n";
        out << "\n";
        out << "if [ \"$ok\" = \"1\" ]; then\n";
        out << "  rm -f \"$CONF\"\n";
        out << "  systemctl disable xetal-firstboot.service >/dev/null 2>&1\n";
        out << "  echo \"${GRN}=== Adaptation complete - continuing boot ===${CLR}\"\n";
        out << "else\n";
        out << "  n=$(cat /var/lib/xetal-firstboot-tries 2>/dev/null || echo 0)\n";
        out << "  n=$((n+1)); echo \"$n\" > /var/lib/xetal-firstboot-tries\n";
        out << "  if [ \"$n\" -ge 3 ]; then\n";
        out << "    rm -f \"$CONF\"; systemctl disable xetal-firstboot.service >/dev/null 2>&1\n";
        out << "    echo \"${YLW}Giving up after 3 attempts - see $LOG${CLR}\"\n";
        out << "  else\n";
        out << "    echo \"${YLW}Will retry the remaining steps on next boot (attempt $n/3).${CLR}\"\n";
        out << "  fi\n";
        out << "fi\n";
        out << "sleep 2\n";
        out << "exit 0\n";
        out << "FBSH\n\n";
    }

    out << "INSTALLER_REL=\"/usr/local/bin/installer.sh\"\n";
    out << "mkdir -p \"$PROFILE/airootfs$(dirname \"$INSTALLER_REL\")\"\n";
    out << "cat > \"$PROFILE/airootfs$INSTALLER_REL\" <<'EOF'\n";
    out << "#!/usr/bin/env bash\n";
    out << "set -euo pipefail\n";
    out << "RED=$'\\e[31m'; GRN=$'\\e[32m'; YLW=$'\\e[33m'; BLU=$'\\e[34m'; WHT=$'\\e[37m'; CLR=$'\\e[0m'\n";
    out << "SNAP=\"/opt/clone/rootfs-snapshot.tar.zst\"\n";
    out << "LOGO=\"/opt/clone/logo.png\"\n";
    out << "BACK=\"XETAL ENGINE - System Installer\"\n\n";
    out << "need(){ command -v \"$1\" >/dev/null 2>&1 || { echo \"${RED}Missing $1${CLR}\"; exit 1; }; }\n";
    out << "for b in lsblk parted mkfs.fat mkfs.ext4 mount umount genfstab arch-chroot grub-install grub-mkconfig efibootmgr rsync tar zstd; do need \"$b\"; done\n";
    out << "[[ -f \"$SNAP\" ]] || { echo \"${RED}Snapshot not found at $SNAP${CLR}\"; exit 1; }\n\n";

    out << "center(){ local w t p; w=$(tput cols 2>/dev/null || echo 80); t=\"$1\"; p=$(( (w - ${#t}) / 2 )); (( p < 0 )) && p=0; printf '%*s%s\\n' \"$p\" '' \"$t\"; }\n\n";

    out << "show_logo(){\n";
    out << "  clear\n";
    out << "  echo\n";
    out << "  if command -v chafa >/dev/null 2>&1 && [[ -f \"$LOGO\" ]]; then\n";
    out << "    local w; w=$(tput cols 2>/dev/null || echo 80); (( w > 110 )) && w=110\n";
    out << "    chafa --align center --size \"${w}x9\" \"$LOGO\" 2>/dev/null || true\n";
    out << "  else\n";
    out << "    echo \"${RED}  X   X EEEEE TTTTT  AAA  L      ${GRN}EEEEE N   N  GGGG  III N   N EEEEE${CLR}\"\n";
    out << "    echo \"${RED}   X X  E       T   A   A L      ${GRN}E     NN  N G       I  NN  N E    ${CLR}\"\n";
    out << "    echo \"${RED}    X   EEEE    T   AAAAA L      ${GRN}EEEE  N N N G  GG   I  N N N EEEE ${CLR}\"\n";
    out << "    echo \"${RED}   X X  E       T   A   A L      ${GRN}E     N  NN G   G   I  N  NN E    ${CLR}\"\n";
    out << "    echo \"${RED}  X   X EEEEE   T   A   A LLLLL  ${GRN}EEEEE N   N  GGGG  III N   N EEEEE${CLR}\"\n";
    out << "  fi\n";
    out << "  echo\n";
    out << "  center \"XETAL ENGINE - System Installer\"\n";
    out << "  echo\n";
    out << "}\n\n";

    out << "DISK=\"\"\n";
    out << "if command -v dialog >/dev/null 2>&1; then\n";
    out << "  show_logo\n";
    out << "  center \"Press ENTER to start\"\n";
    out << "  read -r || true\n\n";
    out << "  # Identify the live USB so the user doesn't nuke it by accident\n";
    out << "  live_disk=$(lsblk -no PKNAME \"$(findmnt -no SOURCE /run/archiso/bootmnt 2>/dev/null)\" 2>/dev/null | head -1 || true)\n";
    out << "  items=()\n";
    out << "  for d in $(lsblk -dpno NAME,TYPE | awk '$2==\"disk\"{print $1}'); do\n";
    out << "    size=$(lsblk -dno SIZE \"$d\" | tr -d ' ')\n";
    out << "    model=$(lsblk -dno MODEL \"$d\" | sed 's/  *$//')\n";
    out << "    tag=\"${size} ${model:-disk}\"\n";
    out << "    if [[ -n \"$live_disk\" && \"$d\" == \"/dev/$live_disk\" ]]; then tag=\"$tag  [LIVE USB - DO NOT USE]\"; fi\n";
    out << "    items+=(\"$d\" \"$tag\")\n";
    out << "  done\n";
    out << "  [[ ${#items[@]} -gt 0 ]] || { echo \"${RED}No disks found.${CLR}\"; exit 1; }\n\n";
    out << "  DISK=$(dialog --backtitle \"$BACK\" --colors --title \" Select Target Disk \" \\\n";
    out << "    --menu \"\\nUse the ARROW KEYS (or mouse) to pick the disk to install to, then press ENTER.\\n\\n\\Z1ALL DATA ON THE CHOSEN DISK WILL BE ERASED!\\Zn\" \\\n";
    out << "    20 74 8 \"${items[@]}\" 3>&1 1>&2 2>&3) || { clear; echo \"Installation cancelled.\"; exit 1; }\n\n";
    out << "  dinfo=$(lsblk -dno SIZE,MODEL \"$DISK\" | sed 's/  */ /g')\n";
    out << "  dialog --backtitle \"$BACK\" --colors --defaultno --title \" Confirm Target \" \\\n";
    out << "    --yesno \"\\nInstall the cloned system to:\\n\\n    $DISK  ($dinfo)\\n\\n\\Z1This PERMANENTLY ERASES everything on that disk.\\Zn\\n\\nContinue?\" 14 66 \\\n";
    out << "    || { clear; echo \"Installation cancelled.\"; exit 1; }\n";
    out << "  dialog --backtitle \"$BACK\" --colors --defaultno --title \" FINAL WARNING \" \\\n";
    out << "    --yesno \"\\n\\Z1LAST CHANCE:\\Zn wipe $DISK and install the cloned system?\" 9 56 \\\n";
    out << "    || { clear; echo \"Installation cancelled.\"; exit 1; }\n";
    out << "  clear; show_logo\n";
    out << "else\n";
    out << "  # Fallback: classic text prompts (no dialog available)\n";
    out << "  show_logo\n";
    out << "  echo \"${YLW}*** WARNING: This ERASES the selected disk (UEFI/GPT only). ***${CLR}\"\n";
    out << "  lsblk -dpno NAME,SIZE,MODEL | sed 's/^/  /'\n";
    out << "  read -rp \"Target DISK (e.g., /dev/nvme0n1 or /dev/sda): \" DISK\n";
    out << "  [[ -b \"$DISK\" ]] || { echo \"${RED}Invalid disk${CLR}\"; exit 1; }\n";
    out << "  echo \"${YLW}Type 'WIPE' to confirm: ${CLR}\"; read -r CONF; [[ \"$CONF\" == \"WIPE\" ]] || { echo \"${RED}Aborted.${CLR}\"; exit 1; }\n";
    out << "fi\n\n";
    out << "[[ -b \"$DISK\" ]] || { echo \"${RED}Invalid disk${CLR}\"; exit 1; }\n\n";

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

    out << "# First-boot hardware adaptation (only present if selected at build time)\n";
    out << "if [ -f /opt/clone/firstboot.conf ]; then\n";
    out << "  echo \"${BLU}[+] Installing first-boot hardware adaptation…${CLR}\"\n";
    out << "  install -Dm755 /opt/clone/xetal-firstboot.sh /mnt/usr/local/bin/xetal-firstboot.sh\n";
    out << "  install -Dm644 /opt/clone/firstboot.conf /mnt/etc/xetal-firstboot.conf\n";
    out << "  install -Dm644 /opt/clone/xetal-firstboot.service /mnt/etc/systemd/system/xetal-firstboot.service\n";
    out << "fi\n\n";

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
    out << "if [ -f /etc/xetal-firstboot.conf ]; then systemctl enable xetal-firstboot.service || true; fi\n";
    out << "CHROOT\n\n";

    out << "echo \"${BLU}[6/6] Done.${CLR}\"\n";
    out << "if command -v dialog >/dev/null 2>&1; then\n";
    out << "  if dialog --backtitle \"$BACK\" --title \" Success \" \\\n";
    out << "     --yesno \"\\nInstallation complete!\\n\\nRemove the USB drive once the machine restarts.\\n\\nReboot now?\" 12 58; then\n";
    out << "    clear; umount -R /mnt 2>/dev/null || true; reboot\n";
    out << "  fi\n";
    out << "  clear; show_logo\n";
    out << "fi\n";
    out << "echo \"${GRN}Installation complete.${CLR}\"\n";
    out << "echo \"Run:   umount -R /mnt && reboot\"\n";
    out << "EOF\n";
    out << "chmod +x \"$PROFILE/airootfs$INSTALLER_REL\"\n\n";

    out << "# === Create convenience wrapper script in root directory ===\n";
    out << "cat > \"$PROFILE/airootfs/xetal.sh\" <<'EOF'\n";
    out << "#!/usr/bin/env bash\n";
    out << "echo \"Setting up installer...\"\n";
    out << "chmod +x /usr/local/bin/installer.sh\n";
    out << "echo \"Running installer...\"\n";
    out << "exec /usr/local/bin/installer.sh \"$@\"\n";
    out << "EOF\n";
    out << "chmod +x \"$PROFILE/airootfs/xetal.sh\"\n\n";

    out << "# === Autologin & autorun installer on TTY1 ===\n";
    out << "mkdir -p \"$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d\" \"$PROFILE/airootfs/root\"\n";
    out << "cat > \"$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d/override.conf\" <<'EOF'\n";
    out << "[Service]\n";
    out << "ExecStart=\n";
    out << "ExecStart=-/sbin/agetty --autologin root --noclear %I $TERM\n";
    out << "Type=idle\n";
    out << "EOF\n";
    out << "# Auto-execute xetal.sh when root logs in on TTY1 (after Arch finishes booting).\n";
    out << "# The archiso releng root shell is ZSH, so .bash_profile alone never ran —\n";
    out << "# .zlogin is what actually fires. Both are written so either shell works.\n";
    out << "cat > \"$PROFILE/airootfs/root/.zlogin\" <<'EOF'\n";
    out << "if [[ -z \"$DISPLAY\" ]] && [[ $(tty) == /dev/tty1 ]]; then\n";
    out << "  bash /xetal.sh || exec zsh\n";
    out << "fi\n";
    out << "EOF\n";
    out << "cat > \"$PROFILE/airootfs/root/.bash_profile\" <<'EOF'\n";
    out << "if [[ -z \"$DISPLAY\" ]] && [[ $(tty) == /dev/tty1 ]]; then\n";
    out << "  bash /xetal.sh || bash\n";
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

    // Clean up offline cache if used
    if (offlineMode) {
        out << "echo \"[*] Cleaning up offline cache...\"\n";
        out << "rm -rf \"$OFFLINE_CACHE_DIR\"\n";
    }


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

// Offline mode functions
void MainWindow::on_onlineModeRadio_toggled(bool checked)
{
    if (checked) {
        ui->offlineStatusLabel->setText(tr("Online mode selected - packages will be downloaded during ISO creation"));
        ui->offlineStatusLabel->setStyleSheet("color: #666666;");
        ui->downloadOfflineButton->setVisible(false);
        ui->checkAvailabilityButton->setVisible(false);
    }
}

void MainWindow::on_offlineModeRadio_toggled(bool checked)
{
    if (checked) {
        ui->checkAvailabilityButton->setVisible(true);
        checkOfflinePackageAvailability();
    }
}

void MainWindow::checkOfflinePackageAvailability()
{
    // Check for both old and new filename
    QString oldFilename = "offline-iso-packages.tar.gz";
    QString newFilename = OFFLINE_PACKAGE_FILENAME;

    QFileInfo newFileInfo(offlinePackagePath);
    QFileInfo oldFileInfo(QDir::currentPath() + "/" + oldFilename);

    if (newFileInfo.exists() && newFileInfo.isFile()) {
        qint64 fileSize = newFileInfo.size();
        QString sizeStr = formatSize(fileSize);
        ui->offlineStatusLabel->setText(QString("✅ %1 Found and Ready (%2)").arg(newFilename, sizeStr));
        ui->offlineStatusLabel->setStyleSheet("color: #28a745; font-weight: bold;");
        ui->downloadOfflineButton->setVisible(false);
    } else if (oldFileInfo.exists() && oldFileInfo.isFile()) {
        qint64 fileSize = oldFileInfo.size();
        QString sizeStr = formatSize(fileSize);
        ui->offlineStatusLabel->setText(QString("✅ %1 Found and Ready (%2) - Consider upgrading to complete package").arg(oldFilename, sizeStr));
        ui->offlineStatusLabel->setStyleSheet("color: #28a745; font-weight: bold;");
        ui->downloadOfflineButton->setVisible(false);
        // Update the path to use the old file
        offlinePackagePath = oldFileInfo.absoluteFilePath();
    } else {
        ui->offlineStatusLabel->setText(tr("❌ Offline package not found. Click download button to get it."));
        ui->offlineStatusLabel->setStyleSheet("color: #dc3545; font-weight: bold;");
        ui->downloadOfflineButton->setVisible(true);
    }
}

void MainWindow::on_downloadOfflineButton_clicked()
{
    QString instructions = QString(
        "Manual Download Instructions:\n\n"
        "1. Your default browser will open to the Google Drive page\n"
        "2. Click the download button on the Google Drive page\n"
        "3. If you see a warning page, click 'Download anyway'\n"
        "4. Save the file as '%1' in this directory:\n"
        "   %2\n\n"
        "5. Once downloaded, return to this application and click 'Check Availability'\n\n"
        "Note: The file is approximately 3.8GB and may take some time to download."
    ).arg(OFFLINE_PACKAGE_FILENAME, QDir::currentPath());

    QMessageBox::information(this, "Download Instructions", instructions);

    // Open the Google Drive page in the default browser
    QDesktopServices::openUrl(QUrl("https://drive.google.com/file/d/1U8Z1MuOTHBJXDqLbgtKoUSKtUqZvBJ8L/view?usp=drive_link"));
}


