#include <QTemporaryFile>
#include "script_helpers.h"

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

    // Use the desktop's default file manager.
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputDir));
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

    // Gather optional first-boot changes (all off preserves users and identity).
    IsoFirstBootOptions firstBoot;
    if (!ui->isoExactCloneCheck->isChecked()) {
        firstBoot.fixNetwork = ui->isoCompatNetworkCheck->isChecked();
        firstBoot.fixGpu = ui->isoCompatGpuCheck->isChecked();
        firstBoot.changeUser = ui->isoCompatUserCheck->isChecked();
        firstBoot.regenSsh = ui->isoCompatSshCheck->isChecked();
        firstBoot.regenMachineId = ui->isoCompatMachineIdCheck->isChecked();
    }

    // Confirm with user
    QString excludeSummary = excludePaths.isEmpty()
        ? "• Standard runtime/build exclusions only\n"
        : QString("• %1 folder(s)/file(s) EXCLUDED from the ISO\n").arg(excludePaths.size());
    if (firstBoot.any()) {
        QStringList opts;
        if (firstBoot.fixNetwork) opts << "network";
        if (firstBoot.fixGpu) opts << "GPU";
        if (firstBoot.changeUser) opts << "user/password";
        if (firstBoot.regenSsh) opts << "SSH keys";
        if (firstBoot.regenMachineId) opts << "NEW machine-id";
        excludeSummary += QString("• First-boot hardware adaptation: %1\n").arg(opts.join(", "));
    } else {
        excludeSummary += "• Preserve users and identity (no first-boot changes)\n";
    }
    QString message = QString("This will create a system clone ISO with the following settings:\n\n"
    "ISO Name: %1\n"
    "Output Directory: %2\n\n"
    "This process will:\n"
    "• Create a file-based system snapshot\n"
    "%3"
    "• Build a bootable ISO with an auto-launching installer\n"
    "• Restore to a new unencrypted ext4 filesystem; partition layout and boot configuration change\n"
    "• Require compatible x86_64 hardware and Secure Boot disabled\n"
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
    ui->isoStatusLabel->document()->setMaximumBlockCount(5000);
    ui->isoStatusLabel->setPlainText("Starting system clone ISO creation...\n");

    // Check if offline mode is selected and package is available
    bool offlineMode = ui->offlineModeRadio->isChecked();
    if (offlineMode) {
        QFileInfo fileInfo(offlinePackagePath);
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            QMessageBox::critical(this, "Offline Package Not Found",
                                  QString("Offline mode is selected but the package file '%1' was not found.\n\n"
                                  "Choose an offline archive first or switch to online mode.").arg(OFFLINE_PACKAGE_FILENAME));
            ui->createIsoButton->setEnabled(true);
            return;
        }
    }

    // Create and run the ISO creation script
    QString scriptPath = createIsoScript(isoName, outputDir, offlineMode, excludePaths, firstBoot);
    if (scriptPath.isEmpty()) {
        ui->createIsoButton->setEnabled(true);
        return;
    }

    // Run the script with step-by-step monitoring
    QProcess *process = new QProcess(this);

    // Set up environment for sudo
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    connect(process, &QProcess::started, [this, process, sudoPassword]() {
        process->write(sudoPassword.toUtf8() + "\n");
        process->closeWriteChannel();
        ui->isoStatusLabel->setPlainText("🚀 Starting system clone ISO creation...\n");
    });

    connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
        QString output = QString::fromUtf8(process->readAllStandardOutput());

        // Print raw output to console
        printf("%s", output.toUtf8().constData());
        fflush(stdout);

        // Keep merged output so package errors remain visible in the tab.
        if (output.contains("Preparing HOME-only build tree")) {
            ui->isoProgressBar->setValue(5);
        } else if (output.contains("Checking live ISO packages")) {
            ui->isoProgressBar->setValue(8);
        } else if (output.contains("Creating full-system snapshot")) {
            ui->isoProgressBar->setValue(10);
        } else if (output.contains("Packing snapshot")) {
            ui->isoProgressBar->setValue(30);
        } else if (output.contains("Cleaning up temporary snapshot directory")) {
            ui->isoProgressBar->setValue(35);
        } else if (output.contains("Embedding snapshot")) {
            ui->isoProgressBar->setValue(50);
        } else if (output.contains("Building ISO")) {
            ui->isoProgressBar->setValue(70);
        } else if (output.contains("Final cleanup")) {
            ui->isoProgressBar->setValue(95);
        } else if (output.contains("ISO ready")) {
            ui->isoProgressBar->setValue(100);
        }

        QTextCursor cursor = ui->isoStatusLabel->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(output);
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

                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    ui->isoProgressBar->setValue(100);
                    ui->isoStatusLabel->setPlainText(ui->isoStatusLabel->toPlainText() + "\n✅ System clone ISO created successfully!");
                    QMessageBox::information(this, tr("Success"),
                                             tr("System clone ISO has been created successfully!\n\n"
                                             "The ISO contains a file-based system snapshot.\n"
                                             "Test it in a VM before relying on it for recovery."));
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





// Generate a private script from the embedded, independently testable shell files.
QString MainWindow::createIsoScript(const QString &isoName, const QString &outputDir, bool offlineMode,
                                    const QStringList &excludePaths, const IsoFirstBootOptions &firstBoot)
{
    QMap<QString, QString> scripts;
    const QStringList names = {"common.sh", "build.sh", "installer.sh", "restore-boot.sh",
                               "firstboot.sh", "firstboot.service"};
    for (const QString &name : names) {
        QFile resource(":/iso/" + name);
        if (!resource.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, tr("Error"), tr("Missing embedded ISO resource: ") + name);
            return {};
        }
        scripts.insert(name, QString::fromUtf8(resource.readAll()));
    }
    QTemporaryFile scriptFile(QDir::tempPath() + "/cachyostools-iso-XXXXXX.sh");
    if (!scriptFile.open()) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot create a private ISO build script."));
        return {};
    }
    QTextStream out(&scriptFile);
    out << "#!/usr/bin/env bash\nset -Eeuo pipefail\n";
    out << "BASE=" << shQuote(QDir::homePath() + "/iso") << "\n";
    out << "OUTPUT_DIR=" << shQuote(outputDir) << "\n";
    out << "ISO_NAME=" << shQuote(isoName) << "\n";
    out << "RELENG=/usr/share/archiso/configs/releng\n";
    out << "ISO_OFFLINE=" << (offlineMode ? "1" : "0") << "\n";
    out << "OFFLINE_PACKAGE=" << shQuote(QFileInfo(offlinePackagePath).absoluteFilePath()) << "\n";
    out << "USER_EXCLUDES=(\n";
    for (const QString &path : excludePaths) {
        // Quote shell metacharacters and escape rsync pattern syntax in literal paths.
        QString pattern = path;
        pattern.replace("\\", "\\\\").replace("*", "\\*").replace("?", "\\?").replace("[", "\\[");
        if (QFileInfo(path).isDir()) pattern += "/*";
        out << "  " << shQuote(pattern) << "\n";
    }
    out << ")\n" << scripts.value("common.sh") << "\n";
    out << "stage_iso_payload() {\n";
    out << "mkdir -p \"$PROFILE/airootfs/usr/local/bin\" \"$PROFILE/airootfs/opt/clone\"\n";
    const QStringList payloads = {"common.sh", "installer.sh", "restore-boot.sh", "firstboot.sh", "firstboot.service"};
    for (const QString &name : payloads) {
        const QString target = name == "installer.sh" ? "/usr/local/bin/installer.sh" : "/opt/clone/" + name;
        out << "cat > \"$PROFILE/airootfs" << target << "\" <<'XETAL_PAYLOAD'\n";
        out << scripts.value(name) << "\nXETAL_PAYLOAD\n";
    }
    if (firstBoot.any()) {
        out << "cat > \"$PROFILE/airootfs/opt/clone/firstboot.conf\" <<'XETAL_OPTIONS'\n";
        out << "FIX_NETWORK=" << int(firstBoot.fixNetwork) << "\nFIX_GPU=" << int(firstBoot.fixGpu)
            << "\nCHANGE_USER=" << int(firstBoot.changeUser) << "\nREGEN_SSH=" << int(firstBoot.regenSsh)
            << "\nREGEN_MACHINE_ID=" << int(firstBoot.regenMachineId) << "\nXETAL_OPTIONS\n";
    }
    out << R"ISO_STAGE(
cat > "$PROFILE/airootfs/xetal.sh" <<'XETAL_WRAPPER'
#!/usr/bin/env bash
exec bash /usr/local/bin/installer.sh "$@"
XETAL_WRAPPER
mkdir -p "$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d" "$PROFILE/airootfs/root"
cat > "$PROFILE/airootfs/etc/systemd/system/getty@tty1.service.d/override.conf" <<'XETAL_GETTY'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear %I $TERM
Type=idle
XETAL_GETTY
for login in .zlogin .bash_profile; do
    cat > "$PROFILE/airootfs/root/$login" <<'XETAL_LOGIN'
if [[ -z "${DISPLAY:-}" && $(tty) == /dev/tty1 ]]; then
    bash /xetal.sh
fi
XETAL_LOGIN
done
}
)ISO_STAGE";
    out << scripts.value("build.sh");
    out.flush();
    if (out.status() != QTextStream::Ok || !scriptFile.flush()) return {};
    const QString scriptPath = scriptFile.fileName();
    scriptFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    scriptFile.setAutoRemove(false);
    scriptFile.close();
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
        ui->downloadOfflineButton->setText(tr("Choose Offline Archive"));
        checkOfflinePackageAvailability();
    }
}

void MainWindow::checkOfflinePackageAvailability()
{
    QStringList candidates;
    if (!offlinePackagePath.isEmpty()) candidates << offlinePackagePath;
    for (const QString &base : {QCoreApplication::applicationDirPath(), QDir::currentPath()}) {
        candidates << base + "/" + OFFLINE_PACKAGE_FILENAME << base + "/offline-iso-packages.tar.gz";
    }
    for (const QString &candidate : candidates) {
        QFileInfo file(candidate);
        if (!file.isFile() || !file.isReadable()) continue;
        offlinePackagePath = file.absoluteFilePath();
        ui->offlineStatusLabel->setText(tr("Archive found: %1 (%2). Contents will be verified before snapshotting.")
                                           .arg(file.fileName(), formatSize(file.size())));
        ui->offlineStatusLabel->setStyleSheet("color: #28a745;");
        ui->downloadOfflineButton->setVisible(true);
        return;
    }
    ui->offlineStatusLabel->setText(tr("Choose an archive made with create_offline_package.sh after an online build."));
    ui->offlineStatusLabel->setStyleSheet("color: #dc3545;");
    ui->downloadOfflineButton->setVisible(true);
}

void MainWindow::on_downloadOfflineButton_clicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choose offline ISO package archive"),
        QDir::homePath(), tr("Package archives (*.tar.gz *.tar.zst *.tar);;All files (*)"));
    if (path.isEmpty()) return;
    offlinePackagePath = path;
    checkOfflinePackageAvailability();
}


// Missing-dependency bar: if the build script's offer to install the ISO
// deps (archiso, rsync, tar, zstd) was missed, the tab itself offers one
// install button per missing tool. Hidden entirely when everything is there.
void MainWindow::setupIsoDepsCheck() {
    isoDepsBar = new QWidget(ui->isoCreatorTab);
    isoDepsBar->setObjectName("isoDepsBar");
    isoDepsBar->setStyleSheet("QWidget#isoDepsBar { background: rgba(230,126,34,0.12);"
                              " border: 1px solid rgba(230,126,34,0.45); border-radius: 8px; }");
    isoDepsBarLayout = new QHBoxLayout(isoDepsBar);
    isoDepsBarLayout->setContentsMargins(10, 6, 10, 6);
    ui->verticalLayout_iso->insertWidget(0, isoDepsBar);
    isoDepsBar->hide();

    // Re-check whenever the user opens this tab (e.g. right after installing)
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int) {
        if (ui->tabWidget->currentWidget() == ui->isoCreatorTab) refreshIsoDeps();
    });
    refreshIsoDeps();
}

void MainWindow::refreshIsoDeps() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                QStringList missing;
                for (const QString &line : QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts))
                    if (line.startsWith("MISSING:")) missing << line.section(':', 1).trimmed();
                missing.removeDuplicates();

                // rebuild the bar's contents
                while (QLayoutItem *item = isoDepsBarLayout->takeAt(0)) {
                    delete item->widget();
                    delete item;
                }
                if (missing.isEmpty()) {
                    isoDepsBar->hide();
                    proc->deleteLater();
                    return;
                }
                QLabel *warn = new QLabel(tr("⚠️ ISO creation needs these missing tools:"), isoDepsBar);
                warn->setStyleSheet("color:#e67e22; font-weight:bold; background:transparent; border:none;");
                isoDepsBarLayout->addWidget(warn);
                for (const QString &pkg : missing) {
                    QPushButton *btn = new QPushButton(tr("⬇️ Install %1").arg(pkg), isoDepsBar);
                    connect(btn, &QPushButton::clicked, this, [this, pkg]() {
                        runSudoCommandInTerminal(QString(
                            "sudo pacman -S --needed %1 && echo '%1 installed — re-open the System ISO tab to refresh this bar.'; read -p 'Press Enter...'").arg(pkg));
                    });
                    isoDepsBarLayout->addWidget(btn);
                }
                QPushButton *allBtn = new QPushButton(tr("⬇️ Install All"), isoDepsBar);
                connect(allBtn, &QPushButton::clicked, this, [this, missing]() {
                    runSudoCommandInTerminal(QString(
                        "sudo pacman -S --needed %1 && echo 'All ISO dependencies installed.'; read -p 'Press Enter...'").arg(missing.join(' ')));
                });
                isoDepsBarLayout->addWidget(allBtn);
                isoDepsBarLayout->addStretch();
                isoDepsBar->show();
                proc->deleteLater();
            });
    // binary -> providing package (what pacman actually installs)
    proc->start("bash", QStringList() << "-c" <<
        "for p in mkarchiso:archiso rsync:rsync tar:tar zstd:zstd bsdtar:libarchive "
        "pacman-key:pacman file:file xorriso:libisoburn mksquashfs:squashfs-tools mkfs.fat:dosfstools mcopy:mtools; do "
        "b=${p%%:*}; k=${p##*:}; command -v \"$b\" >/dev/null 2>&1 || echo \"MISSING:$k\"; done; "
        "test -s /usr/share/pacman/keyrings/archlinux.gpg || echo MISSING:archlinux-keyring; "
        "test -f /usr/share/archiso/configs/releng/profiledef.sh || echo MISSING:archiso");
}
