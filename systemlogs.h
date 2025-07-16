
void MainWindow::setupLogFiles() {
    logFileList.clear();
    
    // System logs
    logFileList.append({"/var/log/messages", "General system messages", "System Logs", false, 0});
    logFileList.append({"/var/log/syslog", "System log messages", "System Logs", false, 0});
    logFileList.append({"/var/log/kern.log", "Kernel log messages", "System Logs", false, 0});
    logFileList.append({"/var/log/auth.log", "Authentication and authorization logs", "System Logs", false, 0});
    logFileList.append({"/var/log/dmesg", "Kernel ring buffer messages", "System Logs", false, 0});
    logFileList.append({"/var/log/daemon.log", "System daemon logs", "System Logs", false, 0});
    logFileList.append({"/var/log/user.log", "User-level messages", "System Logs", false, 0});
    
    // Boot and startup logs
    logFileList.append({"/var/log/boot.log", "Boot process messages", "Boot Logs", false, 0});
    logFileList.append({"/var/log/wtmp", "User login/logout records", "Boot Logs", false, 0});
    logFileList.append({"/var/log/btmp", "Failed login attempts", "Boot Logs", false, 0});
    logFileList.append({"/var/log/lastlog", "Last login information", "Boot Logs", false, 0});
    
    // Package management logs
    logFileList.append({"/var/log/pacman.log", "Pacman package manager logs", "Package Logs", false, 0});
    logFileList.append({"/var/log/paru.log", "Paru AUR helper logs", "Package Logs", false, 0});
    
    // X11 and display logs
    logFileList.append({"/var/log/Xorg.0.log", "X11 server logs", "Display Logs", false, 0});
    logFileList.append({"/var/log/Xorg.0.log.old", "Previous X11 server logs", "Display Logs", false, 0});
    
    // Network logs
    logFileList.append({"/var/log/network.log", "Network service logs", "Network Logs", false, 0});
    logFileList.append({"/var/log/ufw.log", "Uncomplicated Firewall logs", "Network Logs", false, 0});
    
    // Application logs
    logFileList.append({"/var/log/cups/error_log", "CUPS printing system errors", "Application Logs", false, 0});
    logFileList.append({"/var/log/cups/access_log", "CUPS printing system access", "Application Logs", false, 0});
    logFileList.append({"/var/log/lightdm/lightdm.log", "LightDM display manager logs", "Application Logs", false, 0});
    logFileList.append({"/var/log/sddm.log", "SDDM display manager logs", "Application Logs", false, 0});
    
    // Systemd journal logs (special handling)
    logFileList.append({"/var/log/journal/", "Systemd journal logs", "Systemd Logs", false, 0});
    
    // System information commands
    logFileList.append({"inxi", "Complete system information (hardware, drivers, etc.)", "System Info", false, 0});
    logFileList.append({"inxi -G", "Graphics/GPU information", "System Info", false, 0});
    logFileList.append({"inxi -C", "CPU information", "System Info", false, 0});
    logFileList.append({"inxi -M", "Machine information", "System Info", false, 0});
    logFileList.append({"inxi -D", "Disk/Storage information", "System Info", false, 0});
    logFileList.append({"inxi -N", "Network information", "System Info", false, 0});
    logFileList.append({"inxi -A", "Audio information", "System Info", false, 0});
    
    // GPU and graphics logs
    logFileList.append({"/var/log/gpu-manager.log", "GPU manager logs", "GPU Logs", false, 0});
    logFileList.append({"/var/log/nvidia-installer.log", "NVIDIA driver installation logs", "GPU Logs", false, 0});
    logFileList.append({"/var/log/amd-installer.log", "AMD driver installation logs", "GPU Logs", false, 0});
    
    // Security logs
    logFileList.append({"/var/log/audit/audit.log", "Audit system logs", "Security Logs", false, 0});
    logFileList.append({"/var/log/secure", "Security-related messages", "Security Logs", false, 0});
    
    // Check which logs actually exist
    for (LogFileEntry &entry : logFileList) {
        if (entry.filePath.startsWith("inxi")) {
            // For inxi commands, check if inxi is available
            QProcess checkProcess;
            checkProcess.start("which", QStringList() << "inxi");
            checkProcess.waitForFinished();
            entry.exists = (checkProcess.exitCode() == 0);
            entry.size = 0; // Commands don't have a file size
        } else {
            QFileInfo fileInfo(entry.filePath);
            entry.exists = fileInfo.exists();
            if (entry.exists) {
                entry.size = fileInfo.size();
            }
        }
    }
}

void MainWindow::populateLogComboBox() {
    ui->logFileComboBox->clear();
    
    for (const LogFileEntry &entry : logFileList) {
        if (entry.exists) {
            QString displayText = QString("%1 (%2)").arg(entry.filePath, entry.description);
            ui->logFileComboBox->addItem(displayText, entry.filePath);
        }
    }
    
    if (ui->logFileComboBox->count() > 0) {
        ui->logFileComboBox->setCurrentIndex(0);
        loadLogContent();
    }
}

void MainWindow::loadLogContent() {
    QString selectedLog = ui->logFileComboBox->currentData().toString();
    if (selectedLog.isEmpty()) return;
    
    ui->logStatusLabel->setText("Loading...");
    
    // Kill any existing log process (defensive)
    if (logProcess) {
        if (logProcess->state() != QProcess::NotRunning) {
            logProcess->terminate();
            if (!logProcess->waitForFinished(2000)) {
                logProcess->kill();
                logProcess->waitForFinished(1000);
            }
        }
        delete logProcess;
        logProcess = nullptr;
    }
    
    logProcess = new QProcess(this);
    connect(logProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onLogProcessFinished);
    
    QStringList args;
    int lines = ui->logLinesSpinBox->value();
    QString filter = ui->logFilterEdit->text();
    
    if (selectedLog == "/var/log/journal/") {
        // Special handling for systemd journal
        args << "-n" << QString::number(lines);
        if (!filter.isEmpty()) {
            args << "-g" << filter;
        }
        logProcess->start("journalctl", args);
    } else if (selectedLog.startsWith("inxi")) {
        // Special handling for inxi commands
        QStringList inxiArgs = selectedLog.split(" ", Qt::SkipEmptyParts);
        if (inxiArgs.size() > 1) {
            // Remove "inxi" from the beginning and use the rest as arguments
            inxiArgs.removeFirst();
            logProcess->start("inxi", inxiArgs);
        } else {
            logProcess->start("inxi", QStringList());
        }
    } else {
        // Regular log files
        if (ui->tailLogCheckBox->isChecked()) {
            args << "-f" << "-n" << QString::number(lines);
        } else {
            args << "-n" << QString::number(lines);
        }
        
        if (!filter.isEmpty()) {
            args << "grep" << filter;
            logProcess->start("tail", args);
        } else {
            args << selectedLog;
            logProcess->start("tail", args);
        }
    }
}

void MainWindow::onLogProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QString output = QString::fromUtf8(logProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(logProcess->readAllStandardError());
    
    if (exitCode == 0 || !output.isEmpty()) {
        ui->logContentTextEdit->setPlainText(output);
        ui->logStatusLabel->setText(QString("Loaded %1 lines").arg(output.count('\n')));
    } else {
        ui->logContentTextEdit->setPlainText("Error loading log file or no content found.");
        ui->logStatusLabel->setText("Error");
    }
    
    delete logProcess;
    logProcess = nullptr;
}

void MainWindow::clearLogFile(const QString &filePath) {
    if (filePath.isEmpty()) return;
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Clear Log File",
        QString("Are you sure you want to clear '%1'?\n\nThis action cannot be undone.").arg(filePath),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        QProcess::startDetached("sudo", QStringList() << "sh" << "-c" << QString("echo '' > '%1'").arg(filePath));
        loadLogContent(); // Reload the content
    }
}

void MainWindow::searchLogFiles() {
    // This could be extended to search across multiple log files
    loadLogContent();
}

void MainWindow::applyLogFilters() {
    loadLogContent();
}

// Log management slots
void MainWindow::on_refreshLogsButton_clicked() {
    setupLogFiles();
    populateLogComboBox();
}

void MainWindow::on_clearLogButton_clicked() {
    QString selectedLog = ui->logFileComboBox->currentData().toString();
    clearLogFile(selectedLog);
}

void MainWindow::on_applyFilterButton_clicked() {
    applyLogFilters();
}

void MainWindow::on_copyLogButton_clicked() {
    QString content = ui->logContentTextEdit->toPlainText();
    if (!content.isEmpty()) {
        QApplication::clipboard()->setText(content);
        ui->logStatusLabel->setText("Copied to clipboard");
    }
}

void MainWindow::on_saveLogButton_clicked() {
    QString content = ui->logContentTextEdit->toPlainText();
    if (content.isEmpty()) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log Content", 
        QString("log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
        "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
            ui->logStatusLabel->setText("Saved to " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Could not save file.");
        }
    }
}

void MainWindow::on_logFileComboBox_currentTextChanged(const QString &text) {
    Q_UNUSED(text)
    loadLogContent();
}

// Helper to open a config file in nano in a terminal with sudo (shared by all tabs)
void MainWindow::openConfigInNano(const QString &filePath) {
    QString command = QString("sudo nano '%1'").arg(filePath);
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << command)) {
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
}
