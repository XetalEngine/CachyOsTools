
// Batch tab upgrades: Drives (health/burn/benchmark), Services (failed banner,
// journal preview, boot analysis), System Config (built-in sudo editor),
// Logs (follow mode, boot selector, severity colors), PKG Install (updates),
// PKG Uninstall (orphans, cache), Network (open ports).

#include <QPlainTextEdit>
#include <QListWidget>

// ============================================================
// Shared: authenticate sudo once for this app session via `sudo -v`
// (password goes to sudo's stdin only — never into a command line or file)
// ============================================================
bool MainWindow::authenticateSudo() {
    // Already authenticated (cached timestamp)?
    QProcess check;
    check.start("sudo", QStringList() << "-n" << "-v");
    check.waitForFinished(5000);
    if (check.exitCode() == 0) return true;

    bool ok;
    QString password = QInputDialog::getText(this, tr("Sudo Password"),
                                             tr("Enter your sudo password:"),
                                             QLineEdit::Password, "", &ok);
    if (!ok || password.isEmpty()) return false;

    QProcess auth;
    auth.start("sudo", QStringList() << "-S" << "-v");
    auth.write((password + "\n").toUtf8());
    auth.closeWriteChannel();
    auth.waitForFinished(10000);
    if (auth.exitCode() != 0) {
        QMessageBox::warning(this, tr("Authentication Failed"), tr("The sudo password was not accepted."));
        return false;
    }
    return true;
}

// ============================================================
// Drives
// ============================================================
void MainWindow::on_driveHealthButton_clicked() {
    QString drive = getSelectedDrive();
    if (drive.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Please select a disk to health-check."));
        return;
    }
    int row = ui->drivesTable->currentRow();
    if (ui->drivesTable->item(row, 3) && ui->drivesTable->item(row, 3)->text() != "disk") {
        QMessageBox::warning(this, tr("Invalid Selection"), tr("Health check works on disks, not partitions. Select the disk itself."));
        return;
    }
    if (!authenticateSudo()) return;

    QProcess proc;
    proc.start("sudo", QStringList() << "smartctl" << "-a" << drive);
    proc.waitForFinished(30000);
    QString out = QString::fromUtf8(proc.readAllStandardOutput());
    if (out.isEmpty()) {
        QMessageBox::warning(this, tr("Health Check Failed"), tr("Could not read SMART data (is smartmontools installed?)."));
        return;
    }

    // Extract key metrics (NVMe and SATA vocabularies)
    auto grab = [&out](const QString &key) -> QString {
        for (const QString &line : out.split('\n')) {
            if (line.startsWith(key)) return line.section(':', 1).trimmed();
        }
        return QString();
    };
    QString overall = grab("SMART overall-health");
    QString spare = grab("Available Spare:");
    QString used = grab("Percentage Used:");
    QString mediaErrors = grab("Media and Data Integrity Errors:");
    QString temp = grab("Temperature:");

    QStringList problems;
    if (overall.contains("FAILED", Qt::CaseInsensitive)) problems << "SMART overall test: FAILED";
    int spareVal = spare.left(spare.indexOf('%')).trimmed().toInt();
    if (!spare.isEmpty() && spareVal < 80) problems << QString("Available Spare low: %1 (healthy drives stay near 100%)").arg(spare);
    qlonglong mediaVal = mediaErrors.remove(',').toLongLong();
    if (mediaVal > 0) problems << QString("Media/Data Integrity Errors: %1").arg(mediaVal);
    // SATA attributes
    for (const QString &line : out.split('\n')) {
        if (line.contains("Reallocated_Sector_Ct") || line.contains("Current_Pending_Sector")) {
            qlonglong raw = line.section(' ', -1, -1, QString::SectionSkipEmpty).toLongLong();
            if (raw > 0) problems << line.simplified();
        }
    }

    QString verdict, style;
    if (overall.contains("FAILED", Qt::CaseInsensitive)) {
        verdict = "❌ FAILING — back up your data NOW";
        style = "background:#c0392b;color:white;font-weight:bold;padding:8px;border-radius:4px;";
    } else if (!problems.isEmpty()) {
        verdict = "⚠️ WORKING, BUT DEGRADED — keep backups current";
        style = "background:#e67e22;color:white;font-weight:bold;padding:8px;border-radius:4px;";
    } else {
        verdict = "✅ HEALTHY";
        style = "background:#27ae60;color:white;font-weight:bold;padding:8px;border-radius:4px;";
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("Health: %1").arg(drive));
    dialog->setMinimumSize(560, 420);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QLabel *verdictLabel = new QLabel(verdict, dialog);
    verdictLabel->setStyleSheet(style);
    verdictLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(verdictLabel);

    QString summary;
    if (!temp.isEmpty()) summary += "Temperature: " + temp + "\n";
    if (!spare.isEmpty()) summary += "Available Spare: " + spare + "\n";
    if (!used.isEmpty()) summary += "Wear (Percentage Used): " + used + "\n";
    if (!mediaErrors.isEmpty()) summary += "Media/Integrity Errors: " + mediaErrors + "\n";
    if (!problems.isEmpty()) summary += "\nIssues found:\n  • " + problems.join("\n  • ") + "\n";
    summary += "\n───── Full SMART output ─────\n" + out;

    QTextEdit *details = new QTextEdit(dialog);
    details->setReadOnly(true);
    details->setFont(QFont("Monospace", 9));
    details->setPlainText(summary);
    layout->addWidget(details);
    QPushButton *closeButton = new QPushButton(tr("Close"), dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::on_loadLabelsButton_clicked() {
    // Authenticate sudo (popup if needed); refreshDrives then automatically
    // uses root lsblk, which reads labels straight from the filesystems
    // instead of udev's cache.
    if (!authenticateSudo()) return;
    refreshDrives();
    logMessage("🏷️ Reloading drive list with filesystem-level label detection...");
}

void MainWindow::on_burnIsoButton_clicked() {
    QString drive = getSelectedDrive();
    if (drive.isEmpty()) {
        QMessageBox::warning(this, tr("No Drive Selected"), tr("Select the USB disk to burn to (the disk, not a partition)."));
        return;
    }
    int row = ui->drivesTable->currentRow();
    if (ui->drivesTable->item(row, 3) && ui->drivesTable->item(row, 3)->text() != "disk") {
        QMessageBox::warning(this, tr("Invalid Selection"), tr("Select the whole disk (type 'disk'), not a partition."));
        return;
    }

    // Never allow burning over the disk the running system lives on
    QProcess rootProc;
    rootProc.start("bash", QStringList() << "-c" << "lsblk -no PKNAME \"$(findmnt -no SOURCE /)\" | head -1");
    rootProc.waitForFinished();
    QString rootDisk = "/dev/" + QString::fromUtf8(rootProc.readAllStandardOutput()).trimmed();
    if (!rootDisk.endsWith("/") && drive == rootDisk) {
        QMessageBox::critical(this, tr("Refused"), tr("That is the disk your running system is installed on. Burning to it would destroy this system."));
        return;
    }

    QString isoDir = QDir::homePath() + "/iso/xiso/output";
    QString isoPath = QFileDialog::getOpenFileName(this, "Select ISO to Burn",
                                                   QDir(isoDir).exists() ? isoDir : QDir::homePath(),
                                                   "ISO Images (*.iso)");
    if (isoPath.isEmpty()) return;

    QString info;
    QProcess infoProc;
    infoProc.start("lsblk", QStringList() << "-dno" << "SIZE,MODEL" << drive);
    infoProc.waitForFinished();
    info = QString::fromUtf8(infoProc.readAllStandardOutput()).simplified();

    int ret = QMessageBox::warning(this, "Burn ISO to USB",
        QString("Write:\n  %1\n\nto:\n  %2  (%3)\n\n"
                "⚠️ EVERYTHING on that device will be PERMANENTLY ERASED.")
            .arg(isoPath, drive, info),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    bool ok;
    QString typed = QInputDialog::getText(this, "Final Confirmation",
        QString("Type the device path exactly to confirm:\n\n    %1").arg(drive),
        QLineEdit::Normal, "", &ok);
    if (!ok || typed.trimmed() != drive) {
        if (ok) QMessageBox::information(this, tr("Cancelled"), tr("Device path did not match — nothing was written."));
        return;
    }

    runScriptInTerminal(QString(
        "echo '=== Burning ISO to %2 ==='\n"
        "umount %2* 2>/dev/null || true\n"
        "dd if='%1' of='%2' bs=4M status=progress oflag=direct conv=fsync\n"
        "sync\n"
        "echo ''\n"
        "echo '✅ Burn complete. It is safe to unplug the USB drive after this window.'\n").arg(isoPath, drive), "burn_iso");
    logMessage(QString("Burning %1 to %2 in terminal...").arg(isoPath, drive));
}

// ============================================================
// Services
// ============================================================
void MainWindow::updateFailedServicesBanner() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                QStringList failed;
                for (const QString &line : lines) {
                    QString unit = line.trimmed().section(' ', 0, 0);
                    if (unit.endsWith(".service")) failed << unit;
                }
                if (failed.isEmpty()) {
                    ui->failedServicesLabel->setText(tr("✅ No failed services"));
                    ui->failedServicesLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
                } else {
                    QString names = failed.mid(0, 3).join(", ");
                    if (failed.size() > 3) names += QString(" +%1 more").arg(failed.size() - 3);
                    ui->failedServicesLabel->setText(QString("⚠️ %1 failed: %2").arg(failed.size()).arg(names));
                    ui->failedServicesLabel->setStyleSheet("color: #c0392b; font-weight: bold;");
                }
                proc->deleteLater();
            });
    QStringList args;
    if (showUserServices) args << "--user";
    args << "--failed" << "--no-legend" << "--plain";
    proc->start("systemctl", args);
}

void MainWindow::showServiceJournal() {
    int row = ui->servicesTable->currentRow();
    if (row < 0 || row >= serviceList.size()) return;
    QString service = serviceList[row].name;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, service](int, QProcess::ExitStatus) {
                QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
                if (out.isEmpty()) out = "(no journal entries for " + service + ")";
                ui->serviceJournalView->setPlainText(out);
                proc->deleteLater();
            });
    QStringList args;
    if (showUserServices) args << "--user";
    args << "-u" << service << "-n" << "20" << "--no-pager" << "-o" << "short";
    proc->start("journalctl", args);
}

void MainWindow::on_bootAnalysisButton_clicked() {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Boot Time Analysis"));
    dialog->setMinimumSize(620, 480);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTextEdit *view = new QTextEdit(dialog);
    view->setReadOnly(true);
    view->setFont(QFont("Monospace", 9));
    view->setPlainText("Analyzing boot time...");
    layout->addWidget(view);
    QPushButton *closeButton = new QPushButton(tr("Close"), dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    QProcess *proc = new QProcess(dialog);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [view, proc](int, QProcess::ExitStatus) {
                view->setPlainText(QString::fromUtf8(proc->readAllStandardOutput()));
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c"
                << "echo '=== Total boot time ==='; systemd-analyze; echo; "
                   "echo '=== Slowest 15 units ==='; systemd-analyze blame --no-pager | head -15; echo; "
                   "echo '=== Critical chain ==='; systemd-analyze critical-chain --no-pager");
}

// ============================================================
// System Config: built-in editor with sudo read/write + auto-backup
// ============================================================
void MainWindow::on_builtinEditButton_clicked() {
    int row = ui->configTable->currentRow();
    if (row < 0 || row >= configFileList.size()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Select a config file in the table first."));
        return;
    }
    openBuiltinEditor(configFileList[row].filePath);
}

void MainWindow::openBuiltinEditor(const QString &filePath) {
    QFileInfo info(filePath);
    if (info.isDir()) {
        QMessageBox::information(this, tr("Directory"), tr("That entry is a directory — pick a specific file inside it (use the terminal editor for browsing)."));
        return;
    }

    // Read the file (directly if permitted, via sudo for root-owned files)
    QString content;
    bool needsSudo = !(info.exists() && info.isReadable() && info.isWritable());
    if (info.exists() && info.isReadable()) {
        QFile f(filePath);
        f.open(QIODevice::ReadOnly | QIODevice::Text);
        content = QString::fromUtf8(f.readAll());
        needsSudo = !info.isWritable();
    } else {
        if (!authenticateSudo()) return;
        QProcess proc;
        proc.start("sudo", QStringList() << "cat" << filePath);
        proc.waitForFinished(10000);
        if (proc.exitCode() != 0 && !info.exists()) {
            content = ""; // new file
        } else if (proc.exitCode() != 0) {
            QMessageBox::warning(this, tr("Read Failed"), tr("Could not read the file even with sudo."));
            return;
        } else {
            content = QString::fromUtf8(proc.readAllStandardOutput());
        }
        needsSudo = true;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(QString("%1%2").arg(filePath, needsSudo ? "  [root]" : ""));
    dialog->setMinimumSize(760, 560);
    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QPlainTextEdit *editor = new QPlainTextEdit(dialog);
    editor->setFont(QFont("Monospace", 10));
    editor->setPlainText(content);
    layout->addWidget(editor);

    QLabel *statusLabel = new QLabel(tr("A timestamped backup is saved automatically before every save."), dialog);
    layout->addWidget(statusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton(tr("💾 Save"), dialog);
    QPushButton *closeButton = new QPushButton(tr("Close"), dialog);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(saveButton, &QPushButton::clicked, [this, dialog, editor, statusLabel, filePath, needsSudo]() {
        // Backup first
        QString backupDir = QDir::homePath() + "/.config/cachyostools/config-backups";
        QDir().mkpath(backupDir);
        QString backupPath = backupDir + "/" + QFileInfo(filePath).fileName() + "." +
                             QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

        if (needsSudo) {
            if (!authenticateSudo()) return;
            QProcess backupProc;
            backupProc.start("sudo", QStringList() << "cp" << "-a" << filePath << backupPath);
            backupProc.waitForFinished(10000); // may fail for new files — fine

            // Write via sudo tee; content goes through stdin, nothing hits a shell
            QProcess writeProc;
            writeProc.start("sudo", QStringList() << "tee" << filePath);
            writeProc.write(editor->toPlainText().toUtf8());
            writeProc.closeWriteChannel();
            writeProc.waitForFinished(15000);
            if (writeProc.exitCode() == 0) {
                statusLabel->setText("✅ Saved (as root). Backup: " + backupPath);
                statusLabel->setStyleSheet("color: #27ae60;");
            } else {
                statusLabel->setText("❌ Save failed: " + QString::fromUtf8(writeProc.readAllStandardError()).trimmed());
                statusLabel->setStyleSheet("color: #c0392b;");
            }
        } else {
            QFile::copy(filePath, backupPath);
            QFile f(filePath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                f.write(editor->toPlainText().toUtf8());
                f.close();
                statusLabel->setText("✅ Saved. Backup: " + backupPath);
                statusLabel->setStyleSheet("color: #27ae60;");
            } else {
                statusLabel->setText(tr("❌ Could not write file."));
                statusLabel->setStyleSheet("color: #c0392b;");
            }
        }
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

// ============================================================
// Logs: follow mode with severity colors + boot selector
// ============================================================
QString MainWindow::colorizeLogLine(const QString &line) {
    QString escaped = line.toHtmlEscaped();
    QString lower = line.toLower();
    if (lower.contains("error") || lower.contains("failed") || lower.contains("fatal") || lower.contains("critical")) {
        return "<span style=\"color:#e74c3c;\">" + escaped + "</span>";
    }
    if (lower.contains("warn")) {
        return "<span style=\"color:#f39c12;\">" + escaped + "</span>";
    }
    return escaped;
}

void MainWindow::on_followLogButton_clicked() {
    if (ui->followLogButton->isChecked()) {
        ui->logContentTextEdit->clear();
        ui->followLogButton->setText(tr("⏸ Stop Following"));
        ui->logStatusLabel->setText(tr("Following journal (live)..."));

        journalFollowProcess = new QProcess(this);
        connect(journalFollowProcess, &QProcess::readyReadStandardOutput, this, [this]() {
            if (!journalFollowProcess) return;
            const QStringList lines = QString::fromUtf8(journalFollowProcess->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                ui->logContentTextEdit->append(colorizeLogLine(line));
            }
        });
        journalFollowProcess->start("journalctl", QStringList() << "-f" << "-n" << "50" << "--no-pager");
    } else {
        ui->followLogButton->setText(tr("▶ Follow Journal"));
        ui->logStatusLabel->setText(tr("Stopped following."));
        if (journalFollowProcess) {
            journalFollowProcess->kill();
            journalFollowProcess->deleteLater();
            journalFollowProcess = nullptr;
        }
    }
}

void MainWindow::on_bootSelectCombo_activated(int index) {
    // Stop follow mode if running (it would interleave)
    if (ui->followLogButton->isChecked()) {
        ui->followLogButton->setChecked(false);
        on_followLogButton_clicked();
    }
    ui->logStatusLabel->setText(tr("Loading journal..."));

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc, index](int, QProcess::ExitStatus) {
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n');
                ui->logContentTextEdit->clear();
                QStringList html;
                for (const QString &line : lines) {
                    if (!line.isEmpty()) html << colorizeLogLine(line);
                }
                ui->logContentTextEdit->setHtml("<pre style=\"font-family:monospace;\">" + html.join("\n") + "</pre>");
                ui->logStatusLabel->setText(QString("Journal for boot -%1 (%2 lines, errors red / warnings orange)").arg(index).arg(html.size()));
                proc->deleteLater();
            });
    int lines = ui->logLinesSpinBox->value();
    proc->start("journalctl", QStringList() << "-b" << QString::number(-index) << "-n" << QString::number(lines) << "--no-pager");
}

// ============================================================
// PKG Install: Updates sub-tab
// ============================================================
void MainWindow::on_checkUpdatesButton_clicked() {
    ui->updatesTable->setSortingEnabled(false);
    ui->updatesTable->setRowCount(0);
    ui->updatesTable->setColumnCount(4);
    QStringList headers;
    headers << tr("Package") << tr("Current Version") << tr("New Version") << tr("Source");
    ui->updatesTable->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < 3; i++) ui->updatesTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    ui->updatesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->updatesCountLabel->setText(tr("Checking official repos..."));
    fetchArchNews();

    // checkupdates comes from pacman-contrib and is not guaranteed installed;
    // a missing binary would never emit finished() and leave the UI stuck
    QProcess whichCheck;
    whichCheck.start("which", QStringList() << "checkupdates");
    whichCheck.waitForFinished();
    if (whichCheck.exitCode() != 0) {
        ui->updatesTable->setSortingEnabled(true);
        ui->updatesCountLabel->setText(tr("⚠️ 'checkupdates' not found — install it with: sudo pacman -S pacman-contrib"));
        return;
    }

    auto addRows = [this](const QString &output, const QString &source) -> int {
        int added = 0;
        const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            // Format: "name current -> new"
            QStringList parts = line.simplified().split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 4 || parts[2] != "->") continue;
            int row = ui->updatesTable->rowCount();
            ui->updatesTable->insertRow(row);
            ui->updatesTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
            ui->updatesTable->setItem(row, 1, new QTableWidgetItem(parts[1]));
            ui->updatesTable->setItem(row, 2, new QTableWidgetItem(parts[3]));
            ui->updatesTable->setItem(row, 3, new QTableWidgetItem(source));
            added++;
        }
        return added;
    };

    QProcess *officialProc = new QProcess(this);
    connect(officialProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, officialProc, addRows](int exitCode, QProcess::ExitStatus) {
                int official = 0;
                if (exitCode == 0) {
                    official = addRows(QString::fromUtf8(officialProc->readAllStandardOutput()), "Official Repo");
                } else if (exitCode == 1) {
                    ui->updatesCountLabel->setText(tr("⚠️ checkupdates failed (install 'pacman-contrib')."));
                }
                officialProc->deleteLater();

                // Then AUR (if a helper exists)
                QString helper;
                QProcess which;
                which.start("which", QStringList() << "yay");
                which.waitForFinished();
                if (which.exitCode() == 0) helper = "yay";
                else {
                    which.start("which", QStringList() << "paru");
                    which.waitForFinished();
                    if (which.exitCode() == 0) helper = "paru";
                }
                if (helper.isEmpty()) {
                    ui->updatesTable->setSortingEnabled(true);
                    ui->updatesCountLabel->setText(QString("✅ %1 official update(s). (No AUR helper for AUR checks.)").arg(official));
                    return;
                }
                QProcess *aurProc = new QProcess(this);
                connect(aurProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        [this, aurProc, addRows, official](int, QProcess::ExitStatus) {
                            int aur = addRows(QString::fromUtf8(aurProc->readAllStandardOutput()), "AUR");
                            ui->updatesTable->setSortingEnabled(true);
                            if (official + aur == 0) {
                                ui->updatesCountLabel->setText(tr("✅ System is up to date!"));
                            } else {
                                ui->updatesCountLabel->setText(QString("⬆️ %1 update(s): %2 official, %3 AUR").arg(official + aur).arg(official).arg(aur));
                            }
                            aurProc->deleteLater();
                        });
                aurProc->start(helper, QStringList() << "-Qua");
            });
    officialProc->start("checkupdates", QStringList());
}

void MainWindow::fetchArchNews() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://archlinux.org/feeds/news/"));
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString rss = QString::fromUtf8(reply->readAll());
            ui->archNewsList->clear();
            QRegularExpression itemRe("<item>.*?<title>(.*?)</title>.*?<pubDate>(.*?)</pubDate>", QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatchIterator it = itemRe.globalMatch(rss);
            int count = 0;
            while (it.hasNext() && count < 8) {
                QRegularExpressionMatch m = it.next();
                QString title = m.captured(1);
                title.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">").replace("&quot;", "\"").replace("&#39;", "'");
                QString date = m.captured(2).section(' ', 1, 3); // "06 Aug 2026"
                ui->archNewsList->addItem(QString("%1 — %2").arg(date, title));
                count++;
            }
        } else {
            ui->archNewsList->clear();
            ui->archNewsList->addItem("Could not fetch Arch news (offline?).");
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

void MainWindow::on_upgradeSystemButton_clicked() {
    int ret = QMessageBox::question(this, tr("Upgrade System"),
        tr("Run a full system upgrade (sudo pacman -Syu) in a terminal?\n\n"
        "Tip: check the Arch news list first — occasionally upgrades need manual steps."),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    runScriptInTerminal("pacman -Syu\n", "system_upgrade");
}

void MainWindow::on_aurUpgradeButton_clicked() {
    QString helper;
    QProcess which;
    which.start("which", QStringList() << "yay");
    which.waitForFinished();
    if (which.exitCode() == 0) helper = "yay";
    else {
        which.start("which", QStringList() << "paru");
        which.waitForFinished();
        if (which.exitCode() == 0) helper = "paru";
    }
    if (helper.isEmpty()) {
        QMessageBox::warning(this, tr("No AUR Helper"), tr("Install Yay or Paru first (PKG Install tab)."));
        return;
    }
    int ret = QMessageBox::question(this, "Upgrade incl. AUR",
        QString("Run a full upgrade including AUR packages (%1 -Syu) in a terminal?").arg(helper),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    // AUR helpers must NOT run as root — plain terminal, they call sudo themselves
    runSudoCommandInTerminal(helper + " -Syu; echo ''; read -p 'Press Enter to close...'");
}

// ============================================================
// PKG Uninstall: orphans + cache
// ============================================================
void MainWindow::on_orphanCleanButton_clicked() {
    QProcess proc;
    proc.start("pacman", QStringList() << "-Qtdq");
    proc.waitForFinished(15000);
    const QStringList orphans = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    if (orphans.isEmpty()) {
        QMessageBox::information(this, tr("No Orphans"), tr("🎉 No orphaned packages found — nothing to clean."));
        return;
    }
    QString list = orphans.mid(0, 20).join("\n  ");
    if (orphans.size() > 20) list += QString("\n  ... and %1 more").arg(orphans.size() - 20);
    int ret = QMessageBox::question(this, "Clean Orphans",
        QString("%1 orphaned package(s) — installed as dependencies but no longer needed by anything:\n\n  %2\n\n"
                "Remove them? (You can review the list again in the terminal before confirming.)")
            .arg(orphans.size()).arg(list),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    runScriptInTerminal("pacman -Rns $(pacman -Qtdq)\n", "clean_orphans");
    ui->uninstallCountLabel->setText(tr("Orphan cleanup running in terminal — click Refresh when done."));
}

void MainWindow::on_cacheCleanButton_clicked() {
    QProcess proc;
    proc.start("bash", QStringList() << "-c" << "du -sh /var/cache/pacman/pkg 2>/dev/null | cut -f1");
    proc.waitForFinished(30000);
    QString cacheSize = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    int ret = QMessageBox::question(this, "Clean Pacman Cache",
        QString("The pacman package cache is using %1.\n\n"
                "Clean it, keeping the 2 most recent versions of each package\n"
                "(so you can still downgrade if an update breaks something)?").arg(cacheSize.isEmpty() ? "?" : cacheSize),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    runScriptInTerminal(
        "echo \"Cache before: $(du -sh /var/cache/pacman/pkg | cut -f1)\"\n"
        "if command -v paccache >/dev/null 2>&1; then\n"
        "  paccache -rk2\n"
        "  paccache -ruk0\n"
        "else\n"
        "  echo 'paccache not found (pacman-contrib) - using pacman -Sc'\n"
        "  pacman -Sc --noconfirm\n"
        "fi\n"
        "echo \"Cache after: $(du -sh /var/cache/pacman/pkg | cut -f1)\"\n", "clean_cache");
}

// ============================================================
// Network: Open Ports
// ============================================================
void MainWindow::refreshOpenPorts() {
    ui->portsTable->setSortingEnabled(false);
    ui->portsTable->setRowCount(0);
    ui->portsTable->setColumnCount(6);
    QStringList headers;
    headers << tr("Protocol") << tr("State") << tr("Address") << tr("Port") << tr("PID") << tr("Process");
    ui->portsTable->setHorizontalHeaderLabels(headers);
    for (int i = 0; i < 5; i++) ui->portsTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    ui->portsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                QRegularExpression procRe("users:\\(\\(\"([^\"]+)\",pid=(\\d+)");
                bool anyProcess = false;
                for (const QString &line : lines) {
                    QStringList parts = line.simplified().split(' ', Qt::SkipEmptyParts);
                    if (parts.size() < 5) continue;
                    QString local = parts[4];
                    int colon = local.lastIndexOf(':');
                    QString addr = (colon > 0) ? local.left(colon) : local;
                    QString port = (colon > 0) ? local.mid(colon + 1) : "";
                    QString procName = "-", pid = "-";
                    QRegularExpressionMatch m = procRe.match(line);
                    if (m.hasMatch()) { procName = m.captured(1); pid = m.captured(2); anyProcess = true; }

                    int row = ui->portsTable->rowCount();
                    ui->portsTable->insertRow(row);
                    ui->portsTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
                    ui->portsTable->setItem(row, 1, new QTableWidgetItem(parts[1]));
                    ui->portsTable->setItem(row, 2, new QTableWidgetItem(addr));
                    NumericTableWidgetItem *portItem = new NumericTableWidgetItem(port);
                    portItem->setData(Qt::UserRole, port.toLongLong());
                    ui->portsTable->setItem(row, 3, portItem);
                    ui->portsTable->setItem(row, 4, new QTableWidgetItem(pid));
                    ui->portsTable->setItem(row, 5, new QTableWidgetItem(procName));
                }
                ui->portsTable->setSortingEnabled(true);
                ui->portsTable->sortByColumn(3, Qt::AscendingOrder);
                QString note = anyProcess ? "" : " (run app with sudo rights cached to see all process names)";
                ui->portsInfoLabel->setText(QString("%1 listening sockets%2").arg(ui->portsTable->rowCount()).arg(note));
                proc->deleteLater();
            });
    // sudo -n shows every process name if a sudo timestamp is cached; falls back to plain ss
    proc->start("bash", QStringList() << "-c" << "sudo -n ss -tulpnH 2>/dev/null || ss -tulpnH");
}

void MainWindow::on_portsRefreshButton_clicked() {
    refreshOpenPorts();
}
