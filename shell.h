void MainWindow::detectShellAndConfig()
{
    detectedShell.clear();
    detectedConfigFile.clear();
    // Detect shell from $SHELL
    QString shellPath = QProcessEnvironment::systemEnvironment().value("SHELL");
    if (!shellPath.isEmpty()) {
        detectedShell = QFileInfo(shellPath).fileName();
    }
    // Fallback: check /etc/passwd
    if (detectedShell.isEmpty()) {
        detectedShell = "bash";
    }
    // Find config file
    for (const QString &file : shellConfigFiles.value(detectedShell)) {
        if (QFile::exists(file)) {
            detectedConfigFile = file;
            break;
        }
    }
    // Fallback: let user pick
    if (detectedConfigFile.isEmpty()) {
        detectedConfigFile = QFileDialog::getOpenFileName(this, "Select Shell Config File", QDir::homePath());
    }
    ui->shellLabel->setText("Detected Shell: " + detectedShell + "\nConfig: " + detectedConfigFile);
}

void MainWindow::loadAliases()
{
    aliasList.clear();
    if (detectedConfigFile.isEmpty()) return;
    QFile file(detectedConfigFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("alias ") || (detectedShell == "fish" && (line.startsWith("abbr ") || line.startsWith("function ")))) {
            AliasEntry entry = parseAliasLine(line, detectedShell);
            if (!entry.name.isEmpty()) aliasList.append(entry);
        }
    }
    file.close();
}

AliasEntry MainWindow::parseAliasLine(const QString &line, const QString &shell)
{
    AliasEntry entry;
    if (shell == "fish") {
        // fish: abbr NAME "COMMAND" or function NAME
        if (line.startsWith("abbr ")) {
            QRegularExpression rx("abbr \\s+(\\S+) \"(.+)\"");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                entry.name = match.captured(1);
                entry.command = match.captured(2);
            }
        } else if (line.startsWith("function ")) {
            QRegularExpression rx("function \\s+(\\S+)");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                entry.name = match.captured(1);
                entry.command = "(function)";
            }
        }
    } else if (shell == "csh" || shell == "tcsh") {
        // csh/tcsh: alias NAME 'COMMAND'
        QRegularExpression rx("alias \\s+(\\S+) \\s+'(.+)'$");
        QRegularExpressionMatch match = rx.match(line);
        if (match.hasMatch()) {
            entry.name = match.captured(1);
            entry.command = match.captured(2);
        }
    } else {
        // bash/zsh/ksh: alias NAME='COMMAND'
        QRegularExpression rx("alias \\s*(\\S+)='(.+)'$");
        QRegularExpressionMatch match = rx.match(line);
        if (match.hasMatch()) {
            entry.name = match.captured(1);
            entry.command = match.captured(2);
        }
    }
    return entry;
}

void MainWindow::populateAliasTable()
{
    ui->aliasTable->setRowCount(0);
    ui->aliasTable->setColumnCount(2);
    QStringList headers;
    headers << "Alias" << "Command";
    ui->aliasTable->setHorizontalHeaderLabels(headers);
    
    // Set column sizing behavior
    ui->aliasTable->horizontalHeader()->setStretchLastSection(true);  // Command column stretches
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Alias column auto-resizes
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Command column stretches
    
    for (const AliasEntry &entry : aliasList) {
        int row = ui->aliasTable->rowCount();
        ui->aliasTable->insertRow(row);
        ui->aliasTable->setItem(row, 0, new QTableWidgetItem(entry.name));
        ui->aliasTable->setItem(row, 1, new QTableWidgetItem(entry.command));
    }
}

void MainWindow::on_addAliasButton_clicked() {
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, "Add Alias", "Alias name:", QLineEdit::Normal, "", &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, "Add Alias", "Alias command:", QLineEdit::Normal, "", &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate
    for (const AliasEntry &entry : aliasList) {
        if (entry.name == name) {
            QMessageBox::warning(this, "Duplicate Alias", "An alias with this name already exists.");
            return;
        }
    }
    AliasEntry newEntry{name, command};
    aliasList.append(newEntry);
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_editAliasButton_clicked() {
    int row = ui->aliasTable->currentRow();
    if (row < 0 || row >= aliasList.size()) return;
    AliasEntry &entry = aliasList[row];
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, "Edit Alias", "Alias name:", QLineEdit::Normal, entry.name, &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, "Edit Alias", "Alias command:", QLineEdit::Normal, entry.command, &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate (except self)
    for (int i = 0; i < aliasList.size(); ++i) {
        if (i != row && aliasList[i].name == name) {
            QMessageBox::warning(this, "Duplicate Alias", "An alias with this name already exists.");
            return;
        }
    }
    entry.name = name;
    entry.command = command;
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_removeAliasButton_clicked() {
    int row = ui->aliasTable->currentRow();
    if (row < 0 || row >= aliasList.size()) return;
    if (QMessageBox::question(this, "Remove Alias", "Are you sure you want to remove this alias?") != QMessageBox::Yes) return;
    aliasList.removeAt(row);
    saveAliases();
    populateAliasTable();
}

void MainWindow::on_reloadAliasButton_clicked() {
    loadAliases();
    populateAliasTable();
    if (!detectedConfigFile.isEmpty()) {
        // Open a new Konsole window, source the config, start a new shell, and close when the shell exits
        QString command = QString("%1 -c 'source %2; %1; exit'").arg(detectedShell, detectedConfigFile);
        QProcess::startDetached("konsole", QStringList() << "-e" << command);
    }
}

QString MainWindow::aliasToLine(const AliasEntry &alias, const QString &shell) {
    if (shell == "fish") {
        return QString("abbr %1 \"%2\"").arg(alias.name, alias.command);
    } else if (shell == "csh" || shell == "tcsh") {
        return QString("alias %1 '%2'").arg(alias.name, alias.command);
    } else {
        return QString("alias %1='%2'").arg(alias.name, alias.command);
    }
}

void MainWindow::saveAliases() {
    if (detectedConfigFile.isEmpty()) return;
    QFile file(detectedConfigFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Only keep lines that are not aliases
        if (!(line.trimmed().startsWith("alias ") || (detectedShell == "fish" && (line.trimmed().startsWith("abbr ") || line.trimmed().startsWith("function "))))) {
            lines << line;
        }
    }
    file.close();
    // Add all current aliases
    for (const AliasEntry &entry : aliasList) {
        lines << aliasToLine(entry, detectedShell);
    }
    // Write back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return;
    QTextStream out(&file);
    for (const QString &line : lines) {
        out << line << "\n";
    }
    file.close();
}
