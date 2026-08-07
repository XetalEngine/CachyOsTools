
#include <QFormLayout>
#include <QTableWidget>
#include <functional>
// Dashboard -> Environment sub-tab: the Windows "Environment Variables"
// dialog. System pane edits /etc/environment (sudo), user pane edits
// ~/.config/environment.d/99-cachyostools.conf (systemd user sessions,
// read by graphical logins). Changes apply at next login.

static void envFillTable(QTableWidget *table, const QString &content) {
    table->setRowCount(0);
    for (const QString &line : content.split('\n')) {
        QString t = line.trimmed();
        if (t.isEmpty() || t.startsWith('#')) continue;
        int eq = t.indexOf('=');
        if (eq <= 0) continue;
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(t.left(eq).trimmed()));
        table->setItem(row, 1, new QTableWidgetItem(t.mid(eq + 1).trimmed()));
    }
}

static QString envTableToContent(QTableWidget *table, const QString &header) {
    QString out = header;
    for (int row = 0; row < table->rowCount(); ++row) {
        QString key = table->item(row, 0) ? table->item(row, 0)->text().trimmed() : "";
        QString val = table->item(row, 1) ? table->item(row, 1)->text().trimmed() : "";
        if (!key.isEmpty()) out += key + "=" + val + "\n";
    }
    return out;
}

static bool envEditRow(QWidget *parent, QString &key, QString &value, const QString &title) {
    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setMinimumWidth(420);
    QFormLayout *form = new QFormLayout(&dlg);
    QLineEdit *keyEdit = new QLineEdit(key, &dlg);
    QLineEdit *valEdit = new QLineEdit(value, &dlg);
    form->addRow(MainWindow::tr("Variable:"), keyEdit);
    form->addRow(MainWindow::tr("Value:"), valEdit);
    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *ok = new QPushButton(MainWindow::tr("OK"), &dlg);
    QPushButton *cancel = new QPushButton(MainWindow::tr("Cancel"), &dlg);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(cancel);
    form->addRow(btns);
    QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return false;
    key = keyEdit->text().trimmed();
    value = valEdit->text().trimmed();
    return QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$").match(key).hasMatch();
}

void MainWindow::setupEnvVarsTab() {
    QVBoxLayout *layout = new QVBoxLayout(ui->envVarsSubTab);

    QLabel *hint = new QLabel(tr("Like the Windows Environment Variables dialog. Changes apply at the NEXT login."), ui->envVarsSubTab);
    hint->setStyleSheet("color:#888;");
    layout->addWidget(hint);

    auto buildPane = [this](const QString &title, QTableWidget *&table, QWidget *parent) {
        QVBoxLayout *pane = new QVBoxLayout();
        QLabel *label = new QLabel(title, parent);
        QFont f = label->font(); f.setBold(true); label->setFont(f);
        pane->addWidget(label);
        table = new QTableWidget(parent);
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels(QStringList() << tr("Variable") << tr("Value"));
        table->horizontalHeader()->setStretchLastSection(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        pane->addWidget(table);
        return pane;
    };

    auto addPaneButtons = [this](QVBoxLayout *pane, QTableWidget *table, std::function<void()> saveFn) {
        QHBoxLayout *btns = new QHBoxLayout();
        QPushButton *add = new QPushButton(tr("➕ Add"), ui->envVarsSubTab);
        QPushButton *edit = new QPushButton(tr("✏️ Edit"), ui->envVarsSubTab);
        QPushButton *remove = new QPushButton(tr("🗑️ Remove"), ui->envVarsSubTab);
        QPushButton *save = new QPushButton(tr("💾 Save"), ui->envVarsSubTab);
        btns->addStretch();
        for (QPushButton *b : {add, edit, remove, save}) btns->addWidget(b);
        pane->addLayout(btns);

        connect(add, &QPushButton::clicked, this, [this, table]() {
            QString k, v;
            if (!envEditRow(this, k, v, tr("Add Variable"))) return;
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(k));
            table->setItem(row, 1, new QTableWidgetItem(v));
        });
        connect(edit, &QPushButton::clicked, this, [this, table]() {
            int row = table->currentRow();
            if (row < 0) { QMessageBox::information(this, tr("No Selection"), tr("Select a variable first.")); return; }
            QString k = table->item(row, 0)->text(), v = table->item(row, 1)->text();
            if (!envEditRow(this, k, v, tr("Edit Variable"))) return;
            table->item(row, 0)->setText(k);
            table->item(row, 1)->setText(v);
        });
        connect(remove, &QPushButton::clicked, this, [table]() {
            if (table->currentRow() >= 0) table->removeRow(table->currentRow());
        });
        connect(save, &QPushButton::clicked, this, saveFn);
    };

    QHBoxLayout *panes = new QHBoxLayout();

    QVBoxLayout *sysPane = buildPane(tr("🖥️ System variables — /etc/environment (all users)"), envSysTable, ui->envVarsSubTab);
    addPaneButtons(sysPane, envSysTable, [this]() {
        QString content = envTableToContent(envSysTable,
            "# /etc/environment — managed partially by CachyOsTools (Environment tab)\n");
        QString b64 = QString::fromUtf8(content.toUtf8().toBase64());
        runSudoCommandInTerminal(QString(
            "sudo cp /etc/environment /etc/environment.bak.$(date +%s) 2>/dev/null; "
            "echo '%1' | base64 -d | sudo tee /etc/environment >/dev/null && "
            "echo 'Saved /etc/environment (backup kept). Takes effect at next login.'; read -p 'Press Enter...'").arg(b64));
    });
    panes->addLayout(sysPane);

    QVBoxLayout *userPane = buildPane(tr("👤 User variables — ~/.config/environment.d (this user)"), envUserTable, ui->envVarsSubTab);
    addPaneButtons(userPane, envUserTable, [this]() {
        QString dir = QDir::homePath() + "/.config/environment.d";
        QDir().mkpath(dir);
        QFile f(dir + "/99-cachyostools.conf");
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Save Failed"), tr("Could not write %1").arg(f.fileName()));
            return;
        }
        f.write(envTableToContent(envUserTable, "# Managed by CachyOsTools (Environment tab)\n").toUtf8());
        f.close();
        QMessageBox::information(this, tr("Saved"), tr("User variables saved. They apply at your next login."));
    });
    panes->addLayout(userPane);

    layout->addLayout(panes);
}

void MainWindow::refreshEnvVars() {
    QFile sys("/etc/environment");
    if (sys.open(QIODevice::ReadOnly | QIODevice::Text))
        envFillTable(envSysTable, QString::fromUtf8(sys.readAll()));
    else
        envSysTable->setRowCount(0);

    QFile user(QDir::homePath() + "/.config/environment.d/99-cachyostools.conf");
    if (user.open(QIODevice::ReadOnly | QIODevice::Text))
        envFillTable(envUserTable, QString::fromUtf8(user.readAll()));
    else
        envUserTable->setRowCount(0);
}
