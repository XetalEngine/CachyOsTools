
void MainWindow::refreshServices() {
    serviceList.clear();
    QStringList args;
    if (showUserServices) {
        args << "--user";
    }
    args << "list-units" << "--type=service" << "--all" << "--no-pager" << "--no-legend";
    QProcess proc;
    proc.start("systemctl", args);
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        // Format: UNIT LOAD ACTIVE SUB DESCRIPTION
        // Example: systemd-journald.service loaded active running Journal Service
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 5) continue;
        int offset = 0;
        // If the first part is not a service (doesn't end with .service), skip it
        if (!parts[0].endsWith(".service")) {
            offset = 1;
        }
        if (parts.size() < 5 + offset) continue;
        ServiceEntry entry;
        entry.name = parts[0 + offset];
        entry.loadState = parts[1 + offset];
        entry.activeState = parts[2 + offset];
        entry.subState = parts[3 + offset];
        entry.description = parts.mid(4 + offset).join(" ");
        // Get enabled/disabled
        QProcess enabledProc;
        QStringList enabledArgs;
        if (showUserServices) enabledArgs << "--user";
        enabledArgs << "is-enabled" << entry.name;
        enabledProc.start("systemctl", enabledArgs);
        enabledProc.waitForFinished();
        entry.enabled = QString(enabledProc.readAllStandardOutput()).trimmed();
        if (entry.enabled == "not-found") entry.enabled = "N/A";
        entry.status = entry.activeState;
        serviceList.append(entry);
    }
    populateServicesTable();
}

void MainWindow::populateServicesTable() {
    ui->servicesTable->setRowCount(0);
    ui->servicesTable->setColumnCount(5);
    QStringList headers;
    headers << "Service" << "Description" << "Status" << "Enabled" << "SubState";
    ui->servicesTable->setHorizontalHeaderLabels(headers);
    // Set column sizing behavior
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive); // Service
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); // Description (make this the biggest)
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive); // Status
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive); // Enabled
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive); // SubState
    for (const ServiceEntry &entry : serviceList) {
        int row = ui->servicesTable->rowCount();
        ui->servicesTable->insertRow(row);
        ui->servicesTable->setItem(row, 0, new QTableWidgetItem(entry.name));
        ui->servicesTable->setItem(row, 1, new QTableWidgetItem(entry.description));
        ui->servicesTable->setItem(row, 2, new QTableWidgetItem(entry.status));
        ui->servicesTable->setItem(row, 3, new QTableWidgetItem(entry.enabled));
        ui->servicesTable->setItem(row, 4, new QTableWidgetItem(entry.subState));
    }
    // Optionally, set default widths for the small columns
    ui->servicesTable->setColumnWidth(0, 120); // Service
    ui->servicesTable->setColumnWidth(2, 100); // Status
    ui->servicesTable->setColumnWidth(3, 80);  // Enabled
    ui->servicesTable->setColumnWidth(4, 100); // SubState
}

void MainWindow::filterServicesTable(const QString &filter) {
    for (int row = 0; row < ui->servicesTable->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < ui->servicesTable->columnCount(); ++col) {
            QTableWidgetItem *item = ui->servicesTable->item(row, col);
            if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        ui->servicesTable->setRowHidden(row, !match);
    }
}

void MainWindow::on_userServicesToggle_stateChanged(int state) {
    showUserServices = (state == Qt::Checked);
    refreshServices();
}

void MainWindow::on_serviceSearchBar_textChanged(const QString &text) {
    filterServicesTable(text);
}

void MainWindow::handleServiceAction(const QString &action) {
    int row = ui->servicesTable->currentRow();
    if (row < 0 || row >= serviceList.size()) return;
    QString service = serviceList[row].name;
    QStringList args;
    if (showUserServices) args << "--user";
    args << action << service;
    QProcess proc;
    proc.start("systemctl", args);
    proc.waitForFinished();
    refreshServices();
}

void MainWindow::on_startServiceButton_clicked() { handleServiceAction("start"); }
void MainWindow::on_stopServiceButton_clicked() { handleServiceAction("stop"); }
void MainWindow::on_restartServiceButton_clicked() { handleServiceAction("restart"); }
void MainWindow::on_enableServiceButton_clicked() { handleServiceAction("enable"); }
void MainWindow::on_disableServiceButton_clicked() { handleServiceAction("disable"); }

// void MainWindow::loadNumlockSetting() {
//     QSettings settings(QDir::homePath() + "/.config/kcminputrc", QSettings::IniFormat);
//     settings.beginGroup("Keyboard");
//     int numlock = settings.value("NumLock", 2).toInt();
//     settings.endGroup();
//     ui->numlockStartupCheckbox->blockSignals(true);
//     ui->numlockStartupCheckbox->setChecked(numlock == 1);
//     ui->numlockStartupCheckbox->blockSignals(false);
// }