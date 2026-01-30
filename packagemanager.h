
// Package Manager tab functions

void MainWindow::checkAurHelpers() {
    // Check if Yay is installed
    QProcess yayCheck;
    yayCheck.start("which", QStringList() << "yay");
    yayCheck.waitForFinished();
    bool yayInstalled = (yayCheck.exitCode() == 0);
    
    // Check if Paru is installed
    QProcess paruCheck;
    paruCheck.start("which", QStringList() << "paru");
    paruCheck.waitForFinished();
    bool paruInstalled = (paruCheck.exitCode() == 0);
    
    // Update UI
    if (yayInstalled) {
        ui->yayStatusLabel->setText("Yay: ✅ Installed");
        ui->yayStatusLabel->setStyleSheet("color: green;");
        ui->yayInstallButton->setVisible(false);
        ui->yayReinstallButton->setVisible(true);
        ui->yayUninstallButton->setVisible(true);
    } else {
        ui->yayStatusLabel->setText("Yay: ❌ Not Installed");
        ui->yayStatusLabel->setStyleSheet("color: red;");
        ui->yayInstallButton->setVisible(true);
        ui->yayReinstallButton->setVisible(false);
        ui->yayUninstallButton->setVisible(false);
    }
    
    if (paruInstalled) {
        ui->paruStatusLabel->setText("Paru: ✅ Installed");
        ui->paruStatusLabel->setStyleSheet("color: green;");
        ui->paruInstallButton->setVisible(false);
        ui->paruReinstallButton->setVisible(true);
        ui->paruUninstallButton->setVisible(true);
    } else {
        ui->paruStatusLabel->setText("Paru: ❌ Not Installed");
        ui->paruStatusLabel->setStyleSheet("color: red;");
        ui->paruInstallButton->setVisible(true);
        ui->paruReinstallButton->setVisible(false);
        ui->paruUninstallButton->setVisible(false);
    }
}

void MainWindow::refreshPacmanInstalled() {
    ui->pacmanInstalledTable->setRowCount(0);
    ui->pacmanInstalledTable->setColumnCount(4);
    QStringList headers;
    headers << "Package Name" << "Version" << "Repository" << "Description";
    ui->pacmanInstalledTable->setHorizontalHeaderLabels(headers);
    ui->pacmanInstalledTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->pacmanInstalledTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->pacmanInstalledTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->pacmanInstalledTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    ui->packageOutputText->setPlainText("Loading installed packages...");
    QApplication::processEvents(); // Update UI
    
    // Get all installed packages - fast method using pacman -Q
    QProcess proc;
    proc.start("pacman", QStringList() << "-Q");
    proc.waitForFinished();
    
    if (proc.exitCode() != 0) {
        ui->packageOutputText->setPlainText("Error: Failed to get installed packages list");
        return;
    }
    
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Parse packages - show name and version immediately
    // Repository and description will be "Loading..." initially, can be loaded on demand if needed
    for (const QString &line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        
        QString name = parts[0];
        QString version = parts[1];
        
        int row = ui->pacmanInstalledTable->rowCount();
        ui->pacmanInstalledTable->insertRow(row);
        ui->pacmanInstalledTable->setItem(row, 0, new QTableWidgetItem(name));
        ui->pacmanInstalledTable->setItem(row, 1, new QTableWidgetItem(version));
        ui->pacmanInstalledTable->setItem(row, 2, new QTableWidgetItem("...")); // Will be loaded on demand
        ui->pacmanInstalledTable->setItem(row, 3, new QTableWidgetItem("...")); // Will be loaded on demand
    }
    
    ui->packageOutputText->setPlainText(QString("✅ Loaded %1 installed packages. Double-click a package to load details.").arg(lines.size()));
}

void MainWindow::searchPacmanAur(const QString &query, int searchMode, int searchType) {
    if (query.isEmpty()) {
        ui->pacmanAurTable->setRowCount(0);
        return;
    }
    
    ui->pacmanAurTable->setRowCount(0);
    ui->pacmanAurTable->setColumnCount(5);
    QStringList headers;
    headers << "Package Name" << "Version" << "Repository" << "Votes" << "Description";
    ui->pacmanAurTable->setHorizontalHeaderLabels(headers);
    ui->pacmanAurTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->pacmanAurTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->pacmanAurTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->pacmanAurTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->pacmanAurTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    
    // Use yay or paru to search AUR
    QString helper = "yay";
    QProcess checkProc;
    checkProc.start("which", QStringList() << "yay");
    checkProc.waitForFinished();
    if (checkProc.exitCode() != 0) {
        checkProc.start("which", QStringList() << "paru");
        checkProc.waitForFinished();
        if (checkProc.exitCode() == 0) {
            helper = "paru";
        } else {
            ui->packageOutputText->setPlainText("Error: Neither Yay nor Paru is installed. Please install one to search AUR.");
            return;
        }
    }
    
    ui->packageOutputText->setPlainText(QString("Searching AUR for '%1'...").arg(query));
    QApplication::processEvents();
    
    QProcess proc;
    QStringList args;
    args << "-Ss";
    bool useNameOnlyFlag = false;
    if (searchMode == 0) {
        // Try to use --nameonly flag for Package Name only
        useNameOnlyFlag = true;
        args << "--nameonly" << query;
    } else {
        args << query;
    }
    proc.start(helper, args);
    proc.waitForFinished();
    
    // If --nameonly failed, try without it and filter results
    if (proc.exitCode() != 0 && useNameOnlyFlag) {
        proc.start(helper, QStringList() << "-Ss" << query);
        proc.waitForFinished();
        useNameOnlyFlag = false; // Will filter in code instead
    }
    
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        ui->packageOutputText->setPlainText(QString("Error: Failed to search AUR\n%1").arg(error));
        return;
    }
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString currentPackage;
    QString currentVersion;
    QString currentDescription;
    QString currentVotes = "N/A";
    QString currentRepository = "";
    
    // Prepare query for comparison (always case-insensitive)
    QString queryLower = query.toLower();
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        // Check if this is a package line (starts with repo/)
        if (line.contains(QRegularExpression("^(aur|community|core|extra|multilib)/"))) {
            // Save previous package if exists
            if (!currentPackage.isEmpty()) {
                // Filter based on search mode and type
                bool shouldAdd = false;
                if (searchMode == 0) {
                    // Package Name only
                    QString packageLower = currentPackage.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower);
                    }
                } else if (searchMode == 1) {
                    // Description only
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = descLower.contains(queryLower);
                    }
                } else {
                    // Both - check if matches name or description
                    QString packageLower = currentPackage.toLower();
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
                    }
                }
                
                if (shouldAdd) {
                    int row = ui->pacmanAurTable->rowCount();
                    ui->pacmanAurTable->insertRow(row);
                    ui->pacmanAurTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
                    ui->pacmanAurTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
                    ui->pacmanAurTable->setItem(row, 2, new QTableWidgetItem(currentRepository));
                    ui->pacmanAurTable->setItem(row, 3, new QTableWidgetItem(currentVotes));
                    ui->pacmanAurTable->setItem(row, 4, new QTableWidgetItem(currentDescription));
                }
            }
            
            // Parse package line: repo/package-name version (votes) [installed]
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString repoPackage = parts[0];
                QStringList repoParts = repoPackage.split('/');
                if (repoParts.size() >= 2) {
                    currentRepository = repoParts[0]; // Extract repository (aur, community, core, etc.)
                    currentPackage = repoParts[1];
                } else {
                    currentPackage = repoPackage;
                    currentRepository = "unknown";
                }
                currentVersion = parts[1];
                currentVotes = "N/A";
                
                // Look for votes in parentheses
                for (int i = 2; i < parts.size(); i++) {
                    if (parts[i].startsWith('(') && parts[i].contains(')')) {
                        currentVotes = parts[i].remove('(').remove(')');
                        break;
                    }
                }
            }
            currentDescription = "";
        } else if (!trimmed.isEmpty() && (line.startsWith("    ") || line.startsWith("\t"))) {
            // Description line (indented)
            if (currentDescription.isEmpty()) {
                currentDescription = trimmed;
            } else {
                currentDescription += " " + trimmed;
            }
        }
    }
    
    // Add last package
    if (!currentPackage.isEmpty()) {
        // Filter based on search mode and type
        bool shouldAdd = false;
        if (searchMode == 0) {
            QString packageLower = currentPackage.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower);
            }
        } else if (searchMode == 1) {
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (descLower == queryLower);
            } else {
                shouldAdd = descLower.contains(queryLower);
            }
        } else {
            QString packageLower = currentPackage.toLower();
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
            }
        }
        
        if (shouldAdd) {
            int row = ui->pacmanAurTable->rowCount();
            ui->pacmanAurTable->insertRow(row);
            ui->pacmanAurTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
            ui->pacmanAurTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
            ui->pacmanAurTable->setItem(row, 2, new QTableWidgetItem(currentRepository));
            ui->pacmanAurTable->setItem(row, 3, new QTableWidgetItem(currentVotes));
            ui->pacmanAurTable->setItem(row, 4, new QTableWidgetItem(currentDescription));
        }
    }
    
    QString modeStr = (searchMode == 0) ? "name" : (searchMode == 1) ? "description" : "name or description";
    QString typeStr = (searchType == 0) ? "exact" : "contains";
    ui->packageOutputText->setPlainText(QString("✅ Found %1 packages matching '%2' (%3 match in %4)").arg(ui->pacmanAurTable->rowCount()).arg(query).arg(typeStr).arg(modeStr));
}

void MainWindow::searchYay(const QString &query, int searchMode, int searchType) {
    if (query.isEmpty()) {
        ui->yayTable->setRowCount(0);
        return;
    }
    
    ui->yayTable->setRowCount(0);
    ui->yayTable->setColumnCount(4);
    QStringList headers;
    headers << "Package Name" << "Version" << "Votes" << "Description";
    ui->yayTable->setHorizontalHeaderLabels(headers);
    ui->yayTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->yayTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->yayTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->yayTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    ui->packageOutputText->setPlainText(QString("Searching with Yay for '%1'...").arg(query));
    QApplication::processEvents();
    
    QProcess proc;
    QStringList args;
    args << "-Ss";
    if (searchMode == 0) {
        // Package Name only
        args << "--nameonly" << query;
    } else {
        args << query;
    }
    proc.start("yay", args);
    proc.waitForFinished();
    
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        ui->packageOutputText->setPlainText(QString("Error: Failed to search with Yay\n%1").arg(error));
        return;
    }
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString currentPackage;
    QString currentVersion;
    QString currentDescription;
    QString currentVotes = "N/A";
    
    // Prepare query for comparison (always case-insensitive)
    QString queryLower = query.toLower();
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        // Check if this is a package line (starts with repo/)
        if (line.contains(QRegularExpression("^(aur|community|core|extra|multilib)/"))) {
            // Save previous package if exists
            if (!currentPackage.isEmpty()) {
                // Filter based on search mode and type
                bool shouldAdd = false;
                if (searchMode == 0) {
                    // Package Name only
                    QString packageLower = currentPackage.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower);
                    }
                } else if (searchMode == 1) {
                    // Description only
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = descLower.contains(queryLower);
                    }
                } else {
                    // Both - check if matches name or description
                    QString packageLower = currentPackage.toLower();
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
                    }
                }
                
                if (shouldAdd) {
                    int row = ui->yayTable->rowCount();
                    ui->yayTable->insertRow(row);
                    ui->yayTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
                    ui->yayTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
                    ui->yayTable->setItem(row, 2, new QTableWidgetItem(currentVotes));
                    ui->yayTable->setItem(row, 3, new QTableWidgetItem(currentDescription));
                }
            }
            
            // Parse package line: repo/package-name version (votes) [installed]
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString repoPackage = parts[0];
                currentPackage = repoPackage.split('/').last();
                currentVersion = parts[1];
                currentVotes = "N/A";
                
                // Look for votes in parentheses
                for (int i = 2; i < parts.size(); i++) {
                    if (parts[i].startsWith('(') && parts[i].contains(')')) {
                        currentVotes = parts[i].remove('(').remove(')');
                        break;
                    }
                }
            }
            currentDescription = "";
        } else if (!trimmed.isEmpty() && (line.startsWith("    ") || line.startsWith("\t"))) {
            // Description line (indented)
            if (currentDescription.isEmpty()) {
                currentDescription = trimmed;
            } else {
                currentDescription += " " + trimmed;
            }
        }
    }
    
    // Add last package
    if (!currentPackage.isEmpty()) {
        // Filter based on search mode and type
        bool shouldAdd = false;
        if (searchMode == 0) {
            QString packageLower = currentPackage.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower);
            }
        } else if (searchMode == 1) {
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (descLower == queryLower);
            } else {
                shouldAdd = descLower.contains(queryLower);
            }
        } else {
            QString packageLower = currentPackage.toLower();
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
            }
        }
        
        if (shouldAdd) {
            int row = ui->yayTable->rowCount();
            ui->yayTable->insertRow(row);
            ui->yayTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
            ui->yayTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
            ui->yayTable->setItem(row, 2, new QTableWidgetItem(currentVotes));
            ui->yayTable->setItem(row, 3, new QTableWidgetItem(currentDescription));
        }
    }
    
    QString modeStr = (searchMode == 0) ? "name" : (searchMode == 1) ? "description" : "name or description";
    QString typeStr = (searchType == 0) ? "exact" : "contains";
    ui->packageOutputText->setPlainText(QString("✅ Found %1 packages matching '%2' (%3 match in %4)").arg(ui->yayTable->rowCount()).arg(query).arg(typeStr).arg(modeStr));
}

void MainWindow::searchParu(const QString &query, int searchMode, int searchType) {
    if (query.isEmpty()) {
        ui->paruTable->setRowCount(0);
        return;
    }
    
    ui->paruTable->setRowCount(0);
    ui->paruTable->setColumnCount(4);
    QStringList headers;
    headers << "Package Name" << "Version" << "Votes" << "Description";
    ui->paruTable->setHorizontalHeaderLabels(headers);
    ui->paruTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->paruTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->paruTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->paruTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    
    ui->packageOutputText->setPlainText(QString("Searching with Paru for '%1'...").arg(query));
    QApplication::processEvents();
    
    QProcess proc;
    QStringList args;
    args << "-Ss";
    if (searchMode == 0) {
        // Package Name only
        args << "--nameonly" << query;
    } else {
        args << query;
    }
    proc.start("paru", args);
    proc.waitForFinished();
    
    if (proc.exitCode() != 0) {
        QString error = QString::fromUtf8(proc.readAllStandardError());
        ui->packageOutputText->setPlainText(QString("Error: Failed to search with Paru\n%1").arg(error));
        return;
    }
    
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString currentPackage;
    QString currentVersion;
    QString currentDescription;
    QString currentVotes = "N/A";
    
    // Prepare query for comparison (always case-insensitive)
    QString queryLower = query.toLower();
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        // Check if this is a package line (starts with repo/)
        if (line.contains(QRegularExpression("^(aur|community|core|extra|multilib)/"))) {
            // Save previous package if exists
            if (!currentPackage.isEmpty()) {
                // Filter based on search mode and type
                bool shouldAdd = false;
                if (searchMode == 0) {
                    // Package Name only
                    QString packageLower = currentPackage.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower);
                    }
                } else if (searchMode == 1) {
                    // Description only
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = descLower.contains(queryLower);
                    }
                } else {
                    // Both - check if matches name or description
                    QString packageLower = currentPackage.toLower();
                    QString descLower = currentDescription.toLower();
                    if (searchType == 0) {
                        // Exact match
                        shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
                    } else {
                        // Contains
                        shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
                    }
                }
                
                if (shouldAdd) {
                    int row = ui->paruTable->rowCount();
                    ui->paruTable->insertRow(row);
                    ui->paruTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
                    ui->paruTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
                    ui->paruTable->setItem(row, 2, new QTableWidgetItem(currentVotes));
                    ui->paruTable->setItem(row, 3, new QTableWidgetItem(currentDescription));
                }
            }
            
            // Parse package line: repo/package-name version (votes) [installed]
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString repoPackage = parts[0];
                currentPackage = repoPackage.split('/').last();
                currentVersion = parts[1];
                currentVotes = "N/A";
                
                // Look for votes in parentheses
                for (int i = 2; i < parts.size(); i++) {
                    if (parts[i].startsWith('(') && parts[i].contains(')')) {
                        currentVotes = parts[i].remove('(').remove(')');
                        break;
                    }
                }
            }
            currentDescription = "";
        } else if (!trimmed.isEmpty() && (line.startsWith("    ") || line.startsWith("\t"))) {
            // Description line (indented)
            if (currentDescription.isEmpty()) {
                currentDescription = trimmed;
            } else {
                currentDescription += " " + trimmed;
            }
        }
    }
    
    // Add last package
    if (!currentPackage.isEmpty()) {
        // Filter based on search mode and type
        bool shouldAdd = false;
        if (searchMode == 0) {
            QString packageLower = currentPackage.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower);
            }
        } else if (searchMode == 1) {
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (descLower == queryLower);
            } else {
                shouldAdd = descLower.contains(queryLower);
            }
        } else {
            QString packageLower = currentPackage.toLower();
            QString descLower = currentDescription.toLower();
            if (searchType == 0) {
                shouldAdd = (packageLower == queryLower) || (descLower == queryLower);
            } else {
                shouldAdd = packageLower.contains(queryLower) || descLower.contains(queryLower);
            }
        }
        
        if (shouldAdd) {
            int row = ui->paruTable->rowCount();
            ui->paruTable->insertRow(row);
            ui->paruTable->setItem(row, 0, new QTableWidgetItem(currentPackage));
            ui->paruTable->setItem(row, 1, new QTableWidgetItem(currentVersion));
            ui->paruTable->setItem(row, 2, new QTableWidgetItem(currentVotes));
            ui->paruTable->setItem(row, 3, new QTableWidgetItem(currentDescription));
        }
    }
    
    QString modeStr = (searchMode == 0) ? "name" : (searchMode == 1) ? "description" : "name or description";
    QString typeStr = (searchType == 0) ? "exact" : "contains";
    ui->packageOutputText->setPlainText(QString("✅ Found %1 packages matching '%2' (%3 match in %4)").arg(ui->paruTable->rowCount()).arg(query).arg(typeStr).arg(modeStr));
}

void MainWindow::on_clearCacheButton_clicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Clear Cache",
                                                              "This will clear the pacman package cache.\n\n"
                                                              "This action requires sudo privileges.\n\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText("Clearing pacman cache...\n");
    
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput());
                QString error = QString::fromUtf8(process->readAllStandardError());
                ui->packageOutputText->setPlainText(output + "\n" + error);
                if (exitCode == 0) {
                    ui->packageOutputText->append("\n✅ Cache cleared successfully!");
                } else {
                    ui->packageOutputText->append("\n❌ Failed to clear cache.");
                }
                process->deleteLater();
            });
    
    QString script = QString("echo '%1' | sudo -S pacman -Sc --noconfirm").arg(sudoPassword);
    process->start("bash", QStringList() << "-c" << script);
}

void MainWindow::on_refreshPackagesButton_clicked() {
    checkAurHelpers();
    refreshPacmanInstalled();
}

void MainWindow::on_pacmanInstalledSearch_textChanged(const QString &text) {
    for (int row = 0; row < ui->pacmanInstalledTable->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < ui->pacmanInstalledTable->columnCount(); ++col) {
            QTableWidgetItem *item = ui->pacmanInstalledTable->item(row, col);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        ui->pacmanInstalledTable->setRowHidden(row, !match);
    }
}

void MainWindow::on_pacmanRefreshInstalledButton_clicked() {
    refreshPacmanInstalled();
}

void MainWindow::on_pacmanAurSearchButton_clicked() {
    QString query = ui->pacmanAurSearch->text();
    int searchMode = ui->pacmanAurSearchMode->currentIndex(); // 0=Name, 1=Desc, 2=Both
    int searchType = ui->pacmanAurSearchType->currentIndex(); // 0=Exact, 1=Contains
    searchPacmanAur(query, searchMode, searchType);
}

void MainWindow::on_pacmanAurSearch_returnPressed() {
    QString query = ui->pacmanAurSearch->text();
    int searchMode = ui->pacmanAurSearchMode->currentIndex();
    int searchType = ui->pacmanAurSearchType->currentIndex();
    searchPacmanAur(query, searchMode, searchType);
}

void MainWindow::on_pacmanUninstallButton_clicked() {
    int row = ui->pacmanInstalledTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to uninstall.");
        return;
    }
    
    QString packageName = ui->pacmanInstalledTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Package",
                                                              QString("Are you sure you want to uninstall '%1'?\n\n"
                                                              "This action requires sudo privileges.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Uninstalling %1...\n").arg(packageName));
    
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, packageName](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput());
                QString error = QString::fromUtf8(process->readAllStandardError());
                ui->packageOutputText->setPlainText(output + "\n" + error);
                if (exitCode == 0) {
                    ui->packageOutputText->append(QString("\n✅ %1 uninstalled successfully!").arg(packageName));
                    refreshPacmanInstalled();
                } else {
                    ui->packageOutputText->append(QString("\n❌ Failed to uninstall %1.").arg(packageName));
                }
                process->deleteLater();
            });
    
    QString script = QString("echo '%1' | sudo -S pacman -R --noconfirm %2").arg(sudoPassword, packageName);
    process->start("bash", QStringList() << "-c" << script);
}

void MainWindow::on_pacmanReinstallButton_clicked() {
    int row = ui->pacmanInstalledTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to reinstall.");
        return;
    }
    
    QString packageName = ui->pacmanInstalledTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Reinstall Package",
                                                              QString("Are you sure you want to reinstall '%1'?\n\n"
                                                              "This action requires sudo privileges.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Refreshing package database...\n"));
    
    // First refresh package database
    QProcess *refreshProcess = new QProcess(this);
    connect(refreshProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, refreshProcess, packageName, sudoPassword](int exitCode, QProcess::ExitStatus exitStatus) {
                refreshProcess->deleteLater();
                
                if (exitCode != 0) {
                    QString error = QString::fromUtf8(refreshProcess->readAllStandardError());
                    ui->packageOutputText->append(QString("\n❌ Failed to refresh package database: %1").arg(error));
                    return;
                }
                
                ui->packageOutputText->append("✅ Package database refreshed.\n");
                ui->packageOutputText->append(QString("Reinstalling %1...\n").arg(packageName));
                
                // Now reinstall the package
                QProcess *process = new QProcess(this);
                connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        [this, process, packageName](int exitCode, QProcess::ExitStatus exitStatus) {
                            QString output = QString::fromUtf8(process->readAllStandardOutput());
                            QString error = QString::fromUtf8(process->readAllStandardError());
                            ui->packageOutputText->append(output + "\n" + error);
                            if (exitCode == 0) {
                                ui->packageOutputText->append(QString("\n✅ %1 reinstalled successfully!").arg(packageName));
                                refreshPacmanInstalled();
                            } else {
                                ui->packageOutputText->append(QString("\n❌ Failed to reinstall %1.").arg(packageName));
                            }
                            process->deleteLater();
                        });
                
                QString script = QString("echo '%1' | sudo -S pacman -S --noconfirm --overwrite='*' %2").arg(sudoPassword, packageName);
                process->start("bash", QStringList() << "-c" << script);
            });
    
    QString refreshScript = QString("echo '%1' | sudo -S pacman -Sy").arg(sudoPassword);
    refreshProcess->start("bash", QStringList() << "-c" << refreshScript);
}

void MainWindow::on_pacmanAurInstallButton_clicked() {
    int row = ui->pacmanAurTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to install.");
        return;
    }
    
    QString packageName = ui->pacmanAurTable->item(row, 0)->text();
    QString repository = "";
    // Get repository from column 2 (Repository column)
    if (ui->pacmanAurTable->item(row, 2)) {
        repository = ui->pacmanAurTable->item(row, 2)->text();
    }
    
    QString installer;
    QString installerName;
    
    // Check if package is from AUR or official repos
    if (repository.toLower().contains("aur")) {
        // AUR package - need yay or paru
        QProcess checkProc;
        checkProc.start("which", QStringList() << "yay");
        checkProc.waitForFinished();
        if (checkProc.exitCode() == 0) {
            installer = "yay";
            installerName = "Yay";
        } else {
            checkProc.start("which", QStringList() << "paru");
            checkProc.waitForFinished();
            if (checkProc.exitCode() == 0) {
                installer = "paru";
                installerName = "Paru";
            } else {
                QMessageBox::critical(this, "Error", "AUR package requires Yay or Paru to be installed. Please install one to install AUR packages.");
                return;
            }
        }
    } else {
        // Official repository package - use pacman
        installer = "pacman";
        installerName = "Pacman";
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Package",
                                                              QString("Install '%1' using %2?\n\n"
                                                              "This will run in a terminal for interactive prompts.").arg(packageName, installerName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Installing %1 using %2...\n").arg(packageName, installerName));
    
    // Run in terminal for interactive prompts
    QString command;
    if (installer == "pacman") {
        // Refresh package database first, then install
        command = QString("sudo pacman -Sy && sudo pacman -S %1").arg(packageName);
    } else {
        command = QString("%1 -S %2").arg(installer, packageName);
    }
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append(QString("\n✅ Installation started in terminal using %1. Please check the terminal window.").arg(installerName));
}

void MainWindow::on_yaySearchButton_clicked() {
    QString query = ui->yaySearch->text();
    int searchMode = ui->yaySearchMode->currentIndex(); // 0=Name, 1=Desc, 2=Both
    int searchType = ui->yaySearchType->currentIndex(); // 0=Exact, 1=Contains
    searchYay(query, searchMode, searchType);
}

void MainWindow::on_yaySearch_returnPressed() {
    QString query = ui->yaySearch->text();
    int searchMode = ui->yaySearchMode->currentIndex();
    int searchType = ui->yaySearchType->currentIndex();
    searchYay(query, searchMode, searchType);
}

void MainWindow::on_yayInstallPackageButton_clicked() {
    int row = ui->yayTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to install.");
        return;
    }
    
    QString packageName = ui->yayTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Package",
                                                              QString("Install '%1' using Yay?\n\n"
                                                              "This will run in a terminal for interactive prompts.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Installing %1 using Yay...\n").arg(packageName));
    
    QString command = QString("yay -S %1").arg(packageName);
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Installation started in terminal. Please check the terminal window.");
}

void MainWindow::on_yayUninstallPackageButton_clicked() {
    int row = ui->yayTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to uninstall.");
        return;
    }
    
    QString packageName = ui->yayTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Package",
                                                              QString("Uninstall '%1' using Yay?\n\n"
                                                              "This will run in a terminal for interactive prompts.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Uninstalling %1 using Yay...\n").arg(packageName));
    
    QString command = QString("yay -Rns %1").arg(packageName);
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Uninstallation started in terminal. Please check the terminal window.");
}

void MainWindow::on_paruSearchButton_clicked() {
    QString query = ui->paruSearch->text();
    int searchMode = ui->paruSearchMode->currentIndex(); // 0=Name, 1=Desc, 2=Both
    int searchType = ui->paruSearchType->currentIndex(); // 0=Exact, 1=Contains
    searchParu(query, searchMode, searchType);
}

void MainWindow::on_paruSearch_returnPressed() {
    QString query = ui->paruSearch->text();
    int searchMode = ui->paruSearchMode->currentIndex();
    int searchType = ui->paruSearchType->currentIndex();
    searchParu(query, searchMode, searchType);
}

void MainWindow::on_paruInstallPackageButton_clicked() {
    int row = ui->paruTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to install.");
        return;
    }
    
    QString packageName = ui->paruTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Package",
                                                              QString("Install '%1' using Paru?\n\n"
                                                              "This will run in a terminal for interactive prompts.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Installing %1 using Paru...\n").arg(packageName));
    
    QString command = QString("paru -S %1").arg(packageName);
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Installation started in terminal. Please check the terminal window.");
}

void MainWindow::on_paruUninstallPackageButton_clicked() {
    int row = ui->paruTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a package to uninstall.");
        return;
    }
    
    QString packageName = ui->paruTable->item(row, 0)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Package",
                                                              QString("Uninstall '%1' using Paru?\n\n"
                                                              "This will run in a terminal for interactive prompts.").arg(packageName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    ui->packageOutputText->setPlainText(QString("Uninstalling %1 using Paru...\n").arg(packageName));
    
    QString command = QString("paru -Rns %1").arg(packageName);
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << command + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Uninstallation started in terminal. Please check the terminal window.");
}

void MainWindow::on_yayInstallButton_clicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Yay",
                                                              "This will install Yay AUR helper.\n\n"
                                                              "This requires sudo privileges and will clone from GitHub.\n\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText("Installing Yay...\n");
    
    QString script = QString(
        "cd /tmp && "
        "git clone https://aur.archlinux.org/yay.git && "
        "cd yay && "
        "echo '%1' | sudo -S makepkg -si --noconfirm"
    ).arg(sudoPassword);
    
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << script + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Yay installation started in terminal. Please check the terminal window.");
    ui->packageOutputText->append("After installation completes, click 'Refresh All' to update the status.");
}

void MainWindow::on_yayReinstallButton_clicked() {
    on_yayUninstallButton_clicked();
    QTimer::singleShot(2000, this, [this]() {
        on_yayInstallButton_clicked();
    });
}

void MainWindow::on_yayUninstallButton_clicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Yay",
                                                              "This will uninstall Yay AUR helper.\n\n"
                                                              "This requires sudo privileges.\n\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText("Uninstalling Yay...\n");
    
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput());
                QString error = QString::fromUtf8(process->readAllStandardError());
                ui->packageOutputText->setPlainText(output + "\n" + error);
                if (exitCode == 0) {
                    ui->packageOutputText->append("\n✅ Yay uninstalled successfully!");
                    checkAurHelpers();
                } else {
                    ui->packageOutputText->append("\n❌ Failed to uninstall Yay.");
                }
                process->deleteLater();
            });
    
    QString script = QString("echo '%1' | sudo -S pacman -Rns --noconfirm yay").arg(sudoPassword);
    process->start("bash", QStringList() << "-c" << script);
}

void MainWindow::on_paruInstallButton_clicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Install Paru",
                                                              "This will install Paru AUR helper.\n\n"
                                                              "This requires sudo privileges and will clone from GitHub.\n\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText("Installing Paru...\n");
    
    QString script = QString(
        "cd /tmp && "
        "git clone https://aur.archlinux.org/paru.git && "
        "cd paru && "
        "echo '%1' | sudo -S makepkg -si --noconfirm"
    ).arg(sudoPassword);
    
    QProcess::startDetached("konsole", QStringList() << "-e" << "bash" << "-c" << script + "; read -p 'Press Enter to close...'");
    
    ui->packageOutputText->append("\n✅ Paru installation started in terminal. Please check the terminal window.");
    ui->packageOutputText->append("After installation completes, click 'Refresh All' to update the status.");
}

void MainWindow::on_paruReinstallButton_clicked() {
    on_paruUninstallButton_clicked();
    QTimer::singleShot(2000, this, [this]() {
        on_paruInstallButton_clicked();
    });
}

void MainWindow::on_paruUninstallButton_clicked() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Uninstall Paru",
                                                              "This will uninstall Paru AUR helper.\n\n"
                                                              "This requires sudo privileges.\n\n"
                                                              "Do you want to continue?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    bool ok;
    QString sudoPassword = QInputDialog::getText(this, "Sudo Password",
                                                 "Enter your sudo password:",
                                                 QLineEdit::Password, "", &ok);
    if (!ok || sudoPassword.isEmpty()) {
        return;
    }
    
    ui->packageOutputText->setPlainText("Uninstalling Paru...\n");
    
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput());
                QString error = QString::fromUtf8(process->readAllStandardError());
                ui->packageOutputText->setPlainText(output + "\n" + error);
                if (exitCode == 0) {
                    ui->packageOutputText->append("\n✅ Paru uninstalled successfully!");
                    checkAurHelpers();
                } else {
                    ui->packageOutputText->append("\n❌ Failed to uninstall Paru.");
                }
                process->deleteLater();
            });
    
    QString script = QString("echo '%1' | sudo -S pacman -Rns --noconfirm paru").arg(sudoPassword);
    process->start("bash", QStringList() << "-c" << script);
}
