
#include <QFormLayout>
#include <QComboBox>
// Dashboard -> Firewall sub-tab: rule management for ufw (full) and
// firewalld (basic). Pairs with Network -> Open Ports.

void MainWindow::setupFirewallTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->firewallSubTab);

    QHBoxLayout *bar = new QHBoxLayout();
    fwStatusLabel = new QLabel(tr("Click Load to detect your firewall and list its rules."), ui->firewallSubTab);
    fwStatusLabel->setStyleSheet("color:#888;");
    QPushButton *loadBtn = new QPushButton(tr("🔓 Load Rules (sudo)"), ui->firewallSubTab);
    QPushButton *enableBtn = new QPushButton(tr("🟢 Enable Firewall"), ui->firewallSubTab);
    QPushButton *disableBtn = new QPushButton(tr("🔴 Disable Firewall"), ui->firewallSubTab);
    QPushButton *addBtn = new QPushButton(tr("➕ Add Rule"), ui->firewallSubTab);
    QPushButton *delBtn = new QPushButton(tr("🗑️ Delete Rule"), ui->firewallSubTab);
    bar->addWidget(fwStatusLabel);
    bar->addStretch();
    for (QPushButton *b : {loadBtn, enableBtn, disableBtn, addBtn, delBtn}) bar->addWidget(b);
    layout->addLayout(bar);

    fwTable = new QTableWidget(ui->firewallSubTab);
    fwTable->setColumnCount(2);
    fwTable->setHorizontalHeaderLabels(QStringList() << tr("#") << tr("Rule"));
    fwTable->horizontalHeader()->setStretchLastSection(true);
    fwTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fwTable->setSelectionMode(QAbstractItemView::SingleSelection);
    fwTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fwTable->setAlternatingRowColors(true);
    fwTable->verticalHeader()->setVisible(false);
    fwTable->setFont(QFont("monospace"));
    layout->addWidget(fwTable);

    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::refreshFirewall);

    connect(enableBtn, &QPushButton::clicked, this, [this]() {
        if (fwBackend == "firewalld")
            runSudoCommandInTerminal("sudo systemctl enable --now firewalld && echo 'firewalld enabled.'; read -p 'Press Enter...'");
        else
            runSudoCommandInTerminal("sudo ufw --force enable && echo 'ufw enabled.'; read -p 'Press Enter...'");
    });
    connect(disableBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::warning(this, tr("Disable Firewall"),
                tr("Disable the firewall? All ports become reachable from the network."),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        if (fwBackend == "firewalld")
            runSudoCommandInTerminal("sudo systemctl disable --now firewalld && echo 'firewalld disabled.'; read -p 'Press Enter...'");
        else
            runSudoCommandInTerminal("sudo ufw disable && echo 'ufw disabled.'; read -p 'Press Enter...'");
    });

    connect(addBtn, &QPushButton::clicked, this, [this]() {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Add Firewall Rule"));
        QFormLayout *form = new QFormLayout(&dlg);
        QComboBox *action = new QComboBox(&dlg);
        action->addItems(QStringList() << "allow" << "deny");
        QLineEdit *port = new QLineEdit(&dlg);
        port->setPlaceholderText(tr("e.g. 22, 8080, 6000:6100"));
        QComboBox *proto = new QComboBox(&dlg);
        proto->addItems(QStringList() << "tcp" << "udp" << tr("both"));
        QLineEdit *from = new QLineEdit(&dlg);
        from->setPlaceholderText(tr("anywhere (empty) or IP/subnet, e.g. 192.168.1.0/24"));
        form->addRow(tr("Action:"), action);
        form->addRow(tr("Port:"), port);
        form->addRow(tr("Protocol:"), proto);
        form->addRow(tr("From:"), from);
        QHBoxLayout *btns = new QHBoxLayout();
        QPushButton *okBtn = new QPushButton(tr("Add"), &dlg);
        QPushButton *cancelBtn = new QPushButton(tr("Cancel"), &dlg);
        btns->addStretch(); btns->addWidget(okBtn); btns->addWidget(cancelBtn);
        form->addRow(btns);
        connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted) return;

        QString p = port->text().trimmed();
        if (p.isEmpty() || !QRegularExpression("^[0-9:]+$").match(p).hasMatch()) {
            QMessageBox::warning(this, tr("Invalid Port"), tr("Enter a port number or range like 6000:6100."));
            return;
        }
        QString src = from->text().trimmed();
        if (!src.isEmpty() && !QRegularExpression("^[0-9a-fA-F.:/]+$").match(src).hasMatch()) {
            QMessageBox::warning(this, tr("Invalid Source"), tr("Enter an IP or subnet like 192.168.1.0/24."));
            return;
        }

        if (fwBackend == "firewalld") {
            QString protoArg = (proto->currentIndex() == 1) ? "udp" : "tcp";  // firewalld: one proto per rule
            runSudoCommandInTerminal(QString(
                "sudo firewall-cmd --permanent --add-port=%1/%2 && sudo firewall-cmd --reload && echo 'Rule added.'; read -p 'Press Enter...'")
                .arg(p, protoArg));
        } else {
            QString cmd;
            QString protoPart = (proto->currentIndex() == 2) ? "" : "/" + proto->currentText();
            if (src.isEmpty())
                cmd = QString("sudo ufw %1 %2%3").arg(action->currentText(), p, protoPart);
            else
                cmd = QString("sudo ufw %1 from %2 to any port %3%4")
                          .arg(action->currentText(), src, p,
                               (proto->currentIndex() == 2) ? "" : " proto " + proto->currentText());
            runSudoCommandInTerminal(cmd + " && echo 'Rule added.'; read -p 'Press Enter...'");
        }
    });

    connect(delBtn, &QPushButton::clicked, this, [this]() {
        int row = fwTable->currentRow();
        if (row < 0 || !fwTable->item(row, 0)) {
            QMessageBox::information(this, tr("No Selection"), tr("Load and select a rule first."));
            return;
        }
        if (fwBackend == "firewalld") {
            QString rule = fwTable->item(row, 1)->text().trimmed();   // "8080/tcp"
            runSudoCommandInTerminal(QString(
                "sudo firewall-cmd --permanent --remove-port=%1 && sudo firewall-cmd --reload && echo 'Rule removed.'; read -p 'Press Enter...'").arg(rule));
        } else {
            QString num = fwTable->item(row, 0)->text();
            if (QMessageBox::question(this, tr("Delete Rule"),
                    tr("Delete rule #%1?\n\n%2").arg(num, fwTable->item(row, 1)->text())) != QMessageBox::Yes) return;
            runSudoCommandInTerminal(QString("sudo ufw --force delete %1 && echo 'Rule deleted.'; read -p 'Press Enter...'").arg(num));
        }
    });
}

void MainWindow::refreshFirewall() {
    // Backend detection is a cheap local check
    QProcess which;
    which.start("bash", QStringList() << "-c" << "command -v ufw >/dev/null && echo ufw || (command -v firewall-cmd >/dev/null && echo firewalld || echo none)");
    which.waitForFinished(2000);
    fwBackend = QString::fromUtf8(which.readAllStandardOutput()).trimmed();

    if (fwBackend == "none") {
        fwStatusLabel->setText(tr("No firewall installed."));
        fwStatusLabel->setStyleSheet("color:#e67e22; font-weight:bold;");
        QMessageBox::information(this, tr("No Firewall"),
            tr("Neither ufw nor firewalld is installed.\n\nRecommended for desktops:\n  sudo pacman -S ufw\n  sudo ufw enable\n\n"
               "Then come back here to manage rules."));
        return;
    }

    if (!authenticateSudo()) return;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                fwTable->setRowCount(0);
                const QString out = QString::fromUtf8(proc->readAllStandardOutput());
                if (fwBackend == "ufw") {
                    // "Status: active" then "[ 1] 22/tcp  ALLOW IN  Anywhere"
                    bool active = out.contains("Status: active");
                    fwStatusLabel->setText(tr("ufw — %1").arg(active ? tr("🟢 active") : tr("🔴 inactive")));
                    fwStatusLabel->setStyleSheet(active ? "color:#27ae60; font-weight:bold;" : "color:#c0392b; font-weight:bold;");
                    for (const QString &line : out.split('\n')) {
                        QRegularExpressionMatch m = QRegularExpression("^\\[\\s*(\\d+)\\]\\s+(.*)$").match(line.trimmed());
                        if (!m.hasMatch()) continue;
                        int row = fwTable->rowCount();
                        fwTable->insertRow(row);
                        fwTable->setItem(row, 0, new QTableWidgetItem(m.captured(1)));
                        fwTable->setItem(row, 1, new QTableWidgetItem(m.captured(2).simplified()));
                    }
                } else {
                    // firewalld: ports + services of the default zone, one per row
                    fwStatusLabel->setText(tr("firewalld — default zone rules (ports shown are deletable here)"));
                    fwStatusLabel->setStyleSheet("color:#27ae60; font-weight:bold;");
                    int n = 1;
                    for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
                        int row = fwTable->rowCount();
                        fwTable->insertRow(row);
                        fwTable->setItem(row, 0, new QTableWidgetItem(QString::number(n++)));
                        fwTable->setItem(row, 1, new QTableWidgetItem(line.trimmed()));
                    }
                }
                fwTable->resizeColumnToContents(0);
                proc->deleteLater();
            });
    if (fwBackend == "ufw")
        proc->start("bash", QStringList() << "-c" << "sudo -n ufw status numbered");
    else
        proc->start("bash", QStringList() << "-c" << "sudo -n firewall-cmd --list-ports | tr ' ' '\\n'; sudo -n firewall-cmd --list-services | tr ' ' '\\n' | sed 's/^/service: /'");
}
