void MainWindow::executeCommand(const QString &command, const QStringList &arguments)
{
    if (currentProcess) {
        currentProcess->kill();
        delete currentProcess;
    }
    
    currentProcess = new QProcess(this);
    
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred,
            this, &MainWindow::onProcessError);
    
    currentProcess->start(command, arguments);
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString output = QString::fromUtf8(currentProcess->readAllStandardOutput());
    QString error = QString::fromUtf8(currentProcess->readAllStandardError());
    
    if (exitCode == 0) {
        if (currentProcess->program() == "lsblk") {
            logMessage("lsblk output:\n" + output); // Log raw output for debugging
            drivesList = parseDriveList(output);
            populateDriveTable();
            logMessage("✅ Drive list refreshed successfully");
        } else {
            logMessage("✅ Command completed successfully");
            // Refresh drive list after mount/unmount operations
            QTimer::singleShot(1000, this, &MainWindow::refreshDrives);
        }
    } else {
        logMessage("❌ Command failed with exit code: " + QString::number(exitCode));
        if (!error.isEmpty()) {
            logMessage("Error: " + error);
        }
    }
    
    delete currentProcess;
    currentProcess = nullptr;
}

void MainWindow::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start command";
            break;
        case QProcess::Crashed:
            errorMsg = "Command crashed";
            break;
        case QProcess::Timedout:
            errorMsg = "Command timed out";
            break;
        case QProcess::WriteError:
            errorMsg = "Write error";
            break;
        case QProcess::ReadError:
            errorMsg = "Read error";
            break;
        case QProcess::UnknownError:
            errorMsg = "Unknown error";
            break;
    }
    
    logMessage("❌ Process error: " + errorMsg);
    delete currentProcess;
    currentProcess = nullptr;
}