
// Dashboard -> System Properties sub-tab: the classic sysdm.cpl panel.
// Identity + hardware summary, hostname change, machine-id regeneration.
// Also hosts the control-panel sub-tab refresh dispatcher.

void MainWindow::setupSystemPropsTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->systemPropsSubTab);

    spInfoLabel = new QLabel(ui->systemPropsSubTab);
    spInfoLabel->setTextFormat(Qt::RichText);
    spInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    spInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    spInfoLabel->setWordWrap(true);
    layout->addWidget(spInfoLabel, 1);

    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), ui->systemPropsSubTab);
    QPushButton *hostnameBtn = new QPushButton(tr("✏️ Change Hostname"), ui->systemPropsSubTab);
    QPushButton *machineIdBtn = new QPushButton(tr("🆔 Regenerate machine-id"), ui->systemPropsSubTab);
    QPushButton *copyBtn = new QPushButton(tr("📋 Copy Info"), ui->systemPropsSubTab);
    bar->addStretch();
    for (QPushButton *b : {refreshBtn, hostnameBtn, machineIdBtn, copyBtn}) bar->addWidget(b);
    layout->addLayout(bar);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshSystemProps);

    connect(hostnameBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, tr("Change Hostname"),
                                             tr("New hostname (letters, digits, hyphens):"),
                                             QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || name.isEmpty()) return;
        if (!QRegularExpression("^[a-zA-Z0-9][a-zA-Z0-9-]*$").match(name).hasMatch()) {
            QMessageBox::warning(this, tr("Invalid Hostname"), tr("Only letters, digits and hyphens are allowed."));
            return;
        }
        runSudoCommandInTerminal(QString(
            "sudo hostnamectl set-hostname '%1' && echo 'Hostname changed to %1. Also check /etc/hosts if you customized it.'; read -p 'Press Enter...'").arg(name));
    });

    connect(machineIdBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::warning(this, tr("Regenerate machine-id"),
                tr("The machine-id identifies this installation (journald, DHCP leases, some licensing).\n\n"
                   "Regenerating gives the system a new identity — mainly useful after cloning. Continue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal(
            "sudo rm -f /etc/machine-id && sudo systemd-machine-id-setup && "
            "echo 'New machine-id:' && cat /etc/machine-id; read -p 'Press Enter...'");
    });

    connect(copyBtn, &QPushButton::clicked, this, [this]() {
        QGuiApplication::clipboard()->setText(spInfoLabel->text()
            .remove(QRegularExpression("<[^>]*>")).replace("&nbsp;", " ").trimmed());
        QMessageBox::information(this, tr("Copied"), tr("System information copied to clipboard."));
    });
}

void MainWindow::refreshSystemProps() {
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                QHash<QString, QString> v;
                for (const QString &line : QString::fromUtf8(proc->readAllStandardOutput()).split('\n')) {
                    int colon = line.indexOf('=');
                    if (colon > 0) v.insert(line.left(colon), line.mid(colon + 1).trimmed());
                }
                auto row = [](const QString &k, const QString &val) {
                    return QString("<tr><td style='color:#888; padding:3px 18px 3px 0;'>%1</td><td style='padding:3px 0;'><b>%2</b></td></tr>")
                        .arg(k, val.toHtmlEscaped());
                };
                QString html = QString("<h2 style='color:#27ae60;'>🪪 %1</h2><table>").arg(tr("System Properties"))
                    + row(tr("Computer name"), v.value("HOSTNAME"))
                    + row(tr("Pretty name"), v.value("PRETTY").isEmpty() ? "—" : v.value("PRETTY"))
                    + row(tr("Operating system"), v.value("OS"))
                    + row(tr("Kernel"), v.value("KERNEL"))
                    + row(tr("Architecture"), v.value("ARCH"))
                    + "<tr><td colspan='2'><hr></td></tr>"
                    + row(tr("Processor"), v.value("CPU"))
                    + row(tr("Graphics"), v.value("GPU"))
                    + row(tr("Installed RAM"), v.value("RAM"))
                    + row(tr("Root filesystem"), v.value("FSTYPE"))
                    + "<tr><td colspan='2'><hr></td></tr>"
                    + row(tr("Virtualization"), v.value("VIRT") == "none" ? tr("none (bare metal)") : v.value("VIRT"))
                    + row(tr("machine-id"), v.value("MID"))
                    + (v.value("BORN").isEmpty() ? "" : row(tr("System installed"), v.value("BORN")))
                    + row(tr("Uptime"), v.value("UP"))
                    + "</table>";
                spInfoLabel->setText(html);
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c" <<
        "echo \"HOSTNAME=$(hostname)\"; "
        "echo \"PRETTY=$(hostnamectl --pretty 2>/dev/null)\"; "
        "echo \"OS=$(. /etc/os-release && echo $PRETTY_NAME)\"; "
        "echo \"KERNEL=$(uname -r)\"; "
        "echo \"ARCH=$(uname -m)\"; "
        "echo \"CPU=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')\"; "
        "echo \"GPU=$(lspci 2>/dev/null | grep -Ei 'vga|3d controller' | head -1 | sed 's/.*: //; s/ (rev.*//')\"; "
        "echo \"RAM=$(free -h | awk '/^Mem:/{print $2}')\"; "
        "echo \"FSTYPE=$(findmnt -no FSTYPE /)\"; "
        "echo \"VIRT=$(systemd-detect-virt 2>/dev/null || echo none)\"; "
        "echo \"MID=$(cat /etc/machine-id 2>/dev/null)\"; "
        "echo \"BORN=$(head -1 /var/log/pacman.log 2>/dev/null | grep -o '^\\[[^]]*\\]' | tr -d '[]')\"; "
        "echo \"UP=$(uptime -p | sed 's/^up //')\"");
}

// Lazy refresh: each control-panel sub-tab reloads when opened. The two that
// need sudo (Restore Points, Firewall) only load via their explicit buttons.
void MainWindow::setupControlPanelTabs() {
    setupDeviceManagerTab();
    setupUsersGroupsTab();
    setupRestorePointsTab();
    setupFirewallTab();
    setupKernelManagerTab();
    setupEnvVarsTab();
    setupSystemPropsTab();

    connect(ui->dashSubTabs, &QTabWidget::currentChanged, this, [this](int) {
        QWidget *page = ui->dashSubTabs->currentWidget();
        if (!page) return;
        const QString name = page->objectName();
        if (name == "deviceManagerSubTab") refreshDeviceManager();
        else if (name == "usersGroupsSubTab") refreshUsersGroups();
        else if (name == "kernelManagerSubTab") refreshKernelManager();
        else if (name == "envVarsSubTab") refreshEnvVars();
        else if (name == "systemPropsSubTab") refreshSystemProps();
        else if (name == "welcomeSubTab") refreshDashboard();
    });
}
