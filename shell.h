#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QIcon>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

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
    ui->aliasTable->setColumnCount(3);
    QStringList headers;
    headers << tr("Alias") << tr("Command") << tr("Shortcut");
    ui->aliasTable->setHorizontalHeaderLabels(headers);

    // Set column sizing behavior
    ui->aliasTable->horizontalHeader()->setStretchLastSection(false);
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Alias column auto-resizes
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Command column stretches
    ui->aliasTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Convert button fixed width
    ui->aliasTable->setColumnWidth(2, 170);

    for (const AliasEntry &entry : aliasList) {
        int row = ui->aliasTable->rowCount();
        ui->aliasTable->insertRow(row);
        ui->aliasTable->setItem(row, 0, new QTableWidgetItem(entry.name));
        ui->aliasTable->setItem(row, 1, new QTableWidgetItem(entry.command));

        QPushButton *convertButton = new QPushButton(tr("🚀 Convert to .desktop"));
        convertButton->setToolTip(tr("Create a launcher for this alias and pin it to the KDE start menu favorites"));
        connect(convertButton, &QPushButton::clicked, this, [this, entry]() {
            showAliasToDesktopDialog(entry);
        });
        ui->aliasTable->setCellWidget(row, 2, convertButton);
    }
}

void MainWindow::on_addAliasButton_clicked() {
    bool ok1, ok2;
    QString name = QInputDialog::getText(this, tr("Add Alias"), tr("Alias name:"), QLineEdit::Normal, "", &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, tr("Add Alias"), tr("Alias command:"), QLineEdit::Normal, "", &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate
    for (const AliasEntry &entry : aliasList) {
        if (entry.name == name) {
            QMessageBox::warning(this, tr("Duplicate Alias"), tr("An alias with this name already exists."));
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
    QString name = QInputDialog::getText(this, tr("Edit Alias"), tr("Alias name:"), QLineEdit::Normal, entry.name, &ok1);
    if (!ok1 || name.trimmed().isEmpty()) return;
    QString command = QInputDialog::getText(this, tr("Edit Alias"), tr("Alias command:"), QLineEdit::Normal, entry.command, &ok2);
    if (!ok2 || command.trimmed().isEmpty()) return;
    // Check for duplicate (except self)
    for (int i = 0; i < aliasList.size(); ++i) {
        if (i != row && aliasList[i].name == name) {
            QMessageBox::warning(this, tr("Duplicate Alias"), tr("An alias with this name already exists."));
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
    if (QMessageBox::question(this, tr("Remove Alias"), tr("Are you sure you want to remove this alias?")) != QMessageBox::Yes) return;
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

// ---------------------------------------------------------------------------
// Alias -> .desktop shortcut
//
// A shell alias only exists inside the shell, so the generated .desktop cannot
// call it directly. We write a tiny launcher script that sources the user's
// shell config and then runs the alias command, and point Exec= at that script.
// That also sidesteps the .desktop Exec quoting rules entirely.
// ---------------------------------------------------------------------------

// Turn a display name into a safe file-name component.
static QString aliasDesktopSlug(const QString &name) {
    QString slug;
    for (const QChar &c : name) {
        if (c.isLetterOrNumber()) slug += c.toLower();
        else if (c == '-' || c == '_') slug += c;
        else if (c.isSpace()) slug += '-';
    }
    while (slug.contains("--")) slug.replace("--", "-");
    slug = slug.mid(0, 60);
    if (slug.isEmpty()) slug = "alias";
    return slug;
}

// Body of the launcher script, in the syntax of the detected shell.
static QString aliasLauncherBody(const QString &shell, const QString &configFile,
                                 const QString &command, bool keepOpen) {
    QString s;
    if (shell == "fish") {
        s = "#!/usr/bin/env fish\n";
        s += "# Generated by CachyOsTools - shell alias launcher\n";
        if (!configFile.isEmpty()) s += QString("test -f \"%1\"; and source \"%1\"\n").arg(configFile);
        s += command + "\n";
        if (keepOpen) s += "echo \"\"; read -P \"Press Enter to close...\" _cot_wait\n";
    } else if (shell == "csh" || shell == "tcsh") {
        s = QString("#!/usr/bin/env %1\n").arg(shell);
        s += "# Generated by CachyOsTools - shell alias launcher\n";
        if (!configFile.isEmpty()) s += QString("if ( -f \"%1\" ) source \"%1\"\n").arg(configFile);
        s += command + "\n";
        if (keepOpen) s += "echo -n \"Press Enter to close...\"; set _cot_wait = $<\n";
    } else {
        s = QString("#!/usr/bin/env %1\n").arg(shell.isEmpty() ? QString("bash") : shell);
        s += "# Generated by CachyOsTools - shell alias launcher\n";
        // bash needs this to expand aliases in a non-interactive shell; harmless elsewhere.
        s += "shopt -s expand_aliases 2>/dev/null || true\n";
        if (!configFile.isEmpty()) s += QString("[ -f \"%1\" ] && . \"%1\"\n").arg(configFile);
        s += command + "\n";
        if (keepOpen) s += "printf '\\nPress Enter to close...'; read _cot_wait\n";
    }
    return s;
}

bool MainWindow::writeAliasDesktopEntry(const AliasEntry &entry, const QString &appName, const QString &command,
                                        const QString &icon, const QString &category, bool runInTerminal,
                                        bool keepTerminalOpen, QString &desktopFileOut, QString &errorOut)
{
    const QString slug = aliasDesktopSlug(appName);
    const QString dataDir = QDir::homePath() + "/.local/share";
    const QString appsDir = dataDir + "/applications";
    const QString scriptDir = dataDir + "/cachyostools/launchers";

    if (!QDir().mkpath(appsDir) || !QDir().mkpath(scriptDir)) {
        errorOut = tr("Could not create %1").arg(appsDir);
        return false;
    }

    // 1. The launcher script
    const QString scriptPath = scriptDir + "/" + slug + ".sh";
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        errorOut = tr("Could not write %1").arg(scriptPath);
        return false;
    }
    script.write(aliasLauncherBody(detectedShell, detectedConfigFile, command,
                                   runInTerminal && keepTerminalOpen).toUtf8());
    script.close();
    script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                          QFile::ReadGroup | QFile::ExeGroup |
                          QFile::ReadOther | QFile::ExeOther);

    // 2. The .desktop entry
    const QString desktopPath = appsDir + "/cachyostools-" + slug + ".desktop";
    QFile desktop(desktopPath);
    if (!desktop.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        errorOut = tr("Could not write %1").arg(desktopPath);
        return false;
    }
    // Values are single-line by construction; strip any newline just in case.
    auto oneLine = [](QString v) { return v.replace('\n', ' ').replace('\r', ' ').trimmed(); };
    QString content;
    content += "[Desktop Entry]\n";
    content += "Type=Application\n";
    content += "Version=1.0\n";
    content += "Name=" + oneLine(appName) + "\n";
    content += "GenericName=" + tr("Shell Alias") + "\n";
    content += "Comment=" + oneLine(tr("Runs the '%1' shell alias: %2").arg(entry.name, command)) + "\n";
    content += "Exec=" + scriptPath + "\n";
    content += "Icon=" + oneLine(icon) + "\n";
    content += "Terminal=" + QString(runInTerminal ? "true" : "false") + "\n";
    content += "Categories=" + category + ";\n";
    content += "StartupNotify=false\n";
    content += "X-CachyOsTools-Alias=" + oneLine(entry.name) + "\n";
    desktop.write(content.toUtf8());
    desktop.close();
    desktop.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                           QFile::ReadGroup | QFile::ReadOther);

    // 3. Refresh the desktop/KService caches so the menu picks it up right away.
    QProcess::execute("update-desktop-database", QStringList() << appsDir);
    for (const QString &tool : {QString("kbuildsycoca6"), QString("kbuildsycoca5")}) {
        QProcess sycoca;
        sycoca.start(tool, QStringList() << "--noincremental");
        if (sycoca.waitForStarted(1500)) {
            sycoca.waitForFinished(15000);
            break;
        }
    }

    desktopFileOut = desktopPath;
    return true;
}

bool MainWindow::addDesktopFileToKdeFavorites(const QString &desktopFilePath, QString &errorOut)
{
    // Plasma 6 keeps Kickoff favorites in the KActivities store (the applet config
    // carries favoritesPortedToKAstats=true), so pinning means linking the app
    // resource to the global activity through kactivitymanagerd.
    const QString desktopId = QFileInfo(desktopFilePath).fileName();

    QDBusInterface linking("org.kde.ActivityManager",
                           "/ActivityManager/Resources/Linking",
                           "org.kde.ActivityManager.ResourcesLinking",
                           QDBusConnection::sessionBus());
    if (!linking.isValid()) {
        errorOut = tr("Could not reach kactivitymanagerd on the session bus.");
        return false;
    }

    QDBusReply<void> reply = linking.call("LinkResourceToActivity",
                                          QString("org.kde.plasma.favorites.applications"),
                                          QString("applications:") + desktopId,
                                          QString(":global"));
    if (!reply.isValid()) {
        errorOut = reply.error().message();
        return false;
    }
    return true;
}

void MainWindow::showAliasToDesktopDialog(const AliasEntry &entry)
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Convert Alias to .desktop Shortcut"));
    dlg.setMinimumWidth(560);

    QVBoxLayout *root = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel(tr("Creates a desktop launcher for the alias <b>%1</b>.").arg(entry.name.toHtmlEscaped()), &dlg);
    hint->setWordWrap(true);
    root->addWidget(hint);

    QFormLayout *form = new QFormLayout();
    root->addLayout(form);

    // Shortcut name -> becomes the application name shown in the menu
    QLineEdit *nameEdit = new QLineEdit(entry.name, &dlg);
    nameEdit->setPlaceholderText(tr("Name shown in the application menu"));
    form->addRow(tr("Shortcut name:"), nameEdit);

    QLineEdit *commandEdit = new QLineEdit(entry.command, &dlg);
    form->addRow(tr("Command:"), commandEdit);

    // Icon: theme icon name or absolute file path, with a Browse button
    QLineEdit *iconEdit = new QLineEdit("utilities-terminal", &dlg);
    iconEdit->setPlaceholderText(tr("Icon name or image file"));
    QPushButton *browseButton = new QPushButton(tr("📁 Browse..."), &dlg);
    QLabel *iconPreview = new QLabel(&dlg);
    iconPreview->setFixedSize(48, 48);
    iconPreview->setAlignment(Qt::AlignCenter);
    iconPreview->setFrameShape(QFrame::StyledPanel);
    QHBoxLayout *iconRow = new QHBoxLayout();
    iconRow->addWidget(iconEdit, 1);
    iconRow->addWidget(browseButton);
    iconRow->addWidget(iconPreview);
    form->addRow(tr("Icon:"), iconRow);

    auto refreshPreview = [iconEdit, iconPreview]() {
        const QString value = iconEdit->text().trimmed();
        QIcon ico = QFileInfo(value).isFile() ? QIcon(value) : QIcon::fromTheme(value);
        iconPreview->setPixmap(ico.isNull() ? QPixmap() : ico.pixmap(40, 40));
        if (ico.isNull()) iconPreview->setText(tr("?"));
    };
    connect(iconEdit, &QLineEdit::textChanged, &dlg, refreshPreview);
    connect(browseButton, &QPushButton::clicked, &dlg, [this, &dlg, iconEdit]() {
        QString start = QFileInfo(iconEdit->text().trimmed()).isFile()
                            ? QFileInfo(iconEdit->text().trimmed()).absolutePath()
                            : QString("/usr/share/icons");
        QString picked = QFileDialog::getOpenFileName(&dlg, tr("Select Icon"), start,
                                                      tr("Images (*.png *.svg *.svgz *.xpm *.ico *.jpg *.jpeg);;All Files (*)"));
        if (!picked.isEmpty()) iconEdit->setText(picked);
    });
    refreshPreview();

    QComboBox *categoryBox = new QComboBox(&dlg);
    categoryBox->addItems(QStringList() << "Utility" << "System" << "Development"
                                        << "Network" << "Graphics" << "AudioVideo"
                                        << "Office" << "Game");
    form->addRow(tr("Category:"), categoryBox);

    QCheckBox *terminalCheck = new QCheckBox(tr("Run in a terminal window"), &dlg);
    terminalCheck->setChecked(true);
    QCheckBox *keepOpenCheck = new QCheckBox(tr("Keep the terminal open after the command finishes"), &dlg);
    keepOpenCheck->setChecked(true);
    QCheckBox *favoriteCheck = new QCheckBox(tr("Pin to the KDE start menu favorites"), &dlg);
    favoriteCheck->setChecked(true);
    connect(terminalCheck, &QCheckBox::toggled, keepOpenCheck, &QCheckBox::setEnabled);

    QGroupBox *options = new QGroupBox(tr("Options"), &dlg);
    QVBoxLayout *optLayout = new QVBoxLayout(options);
    optLayout->addWidget(terminalCheck);
    optLayout->addWidget(keepOpenCheck);
    optLayout->addWidget(favoriteCheck);
    root->addWidget(options);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Create Shortcut"));
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const QString appName = nameEdit->text().trimmed();
    const QString command = commandEdit->text().trimmed();
    if (appName.isEmpty() || command.isEmpty()) {
        QMessageBox::warning(this, tr("Missing Information"),
                             tr("The shortcut name and the command are both required."));
        return;
    }

    QString icon = iconEdit->text().trimmed();
    if (icon.isEmpty()) icon = "utilities-terminal";

    QString desktopFile, error;
    if (!writeAliasDesktopEntry(entry, appName, command, icon, categoryBox->currentText(),
                                terminalCheck->isChecked(), keepOpenCheck->isChecked(),
                                desktopFile, error)) {
        QMessageBox::critical(this, tr("Shortcut Failed"), error);
        return;
    }

    QString message = tr("Created %1").arg(desktopFile);
    if (favoriteCheck->isChecked()) {
        QString favError;
        if (addDesktopFileToKdeFavorites(desktopFile, favError))
            message += tr("\n\nPinned to the start menu favorites.");
        else
            message += tr("\n\nThe shortcut was created, but it could not be pinned automatically (%1).\n"
                          "You can right-click it in the application menu and choose "
                          "\"Add to Favorites\".").arg(favError);
    }
    QMessageBox::information(this, tr("Shortcut Created"), message);
}
