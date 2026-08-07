
// Dashboard tab: styled system-at-a-glance cards. Every card loads
// asynchronously so opening the tab never blocks the UI.

#include <QGroupBox>

// Application-wide XETAL theme: gives every tab the Dashboard's card look.
// Palette-agnostic (translucent grays) so it works on light and dark themes.
// Widget-level setStyleSheet calls (status pills etc.) still override this.
void MainWindow::applyAppTheme() {
    setStyleSheet(
        // --- Cards: every group box gets the green accent stripe ---
        "QGroupBox {"
        "  background: rgba(127,127,127,0.07);"
        "  border: 1px solid rgba(127,127,127,0.25);"
        "  border-top: 3px solid #27ae60;"
        "  border-radius: 10px;"
        "  margin-top: 14px;"
        "  padding: 10px 8px 6px 8px;"
        "  font-weight: bold;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
        "QGroupBox QLabel, QGroupBox QPushButton, QGroupBox QCheckBox, QGroupBox QRadioButton,"
        "QGroupBox QComboBox, QGroupBox QSpinBox, QGroupBox QLineEdit { font-weight: normal; }"

        // --- Buttons ---
        "QPushButton {"
        "  border: 1px solid rgba(127,127,127,0.3);"
        "  border-radius: 6px;"
        "  padding: 5px 10px;"
        "  background: rgba(127,127,127,0.12);"
        "}"
        "QPushButton:hover { background: rgba(39,174,96,0.25); border-color: #27ae60; }"
        "QPushButton:pressed { background: rgba(39,174,96,0.40); }"
        "QPushButton:disabled { color: rgba(127,127,127,0.6); background: rgba(127,127,127,0.05); }"

        // --- Main tab bar ---
        "QTabWidget::pane { border: 1px solid rgba(127,127,127,0.25); border-radius: 6px; top: -1px; }"
        "QTabBar::tab {"
        "  background: rgba(127,127,127,0.08);"
        "  border: 1px solid rgba(127,127,127,0.2);"
        "  padding: 6px 14px;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: rgba(39,174,96,0.18);"
        "  border-top: 2px solid #27ae60;"
        "  font-weight: bold;"
        "}"
        "QTabBar::tab:hover:!selected { background: rgba(127,127,127,0.18); }"

        // --- Tables, trees, lists ---
        "QTableWidget, QTreeWidget, QListWidget {"
        "  border: 1px solid rgba(127,127,127,0.25);"
        "  border-radius: 8px;"
        "  gridline-color: rgba(127,127,127,0.12);"
        "  alternate-background-color: rgba(127,127,127,0.06);"
        "  selection-background-color: rgba(39,174,96,0.35);"
        "}"
        "QHeaderView::section {"
        "  background: rgba(127,127,127,0.12);"
        "  border: none;"
        "  border-bottom: 2px solid #27ae60;"
        "  padding: 5px;"
        "  font-weight: bold;"
        "}"

        // --- Inputs ---
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QPlainTextEdit {"
        "  border: 1px solid rgba(127,127,127,0.3);"
        "  border-radius: 6px;"
        "  padding: 3px 6px;"
        "  background: rgba(127,127,127,0.06);"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QTextEdit:focus, QPlainTextEdit:focus {"
        "  border: 1px solid #27ae60;"
        "}"

        // --- Progress bars (ISO build, backup...) ---
        "QProgressBar {"
        "  border: 1px solid rgba(127,127,127,0.3);"
        "  border-radius: 6px;"
        "  text-align: center;"
        "  background: rgba(127,127,127,0.08);"
        "}"
        "QProgressBar::chunk { background: #27ae60; border-radius: 5px; }"

        // --- Splitters (ISO exclusion panels) ---
        "QSplitter::handle { background: rgba(127,127,127,0.2); }"
        "QSplitter::handle:hover { background: #27ae60; }");
}

// Unicode usage bar: colored blocks, green -> orange -> red by fill level
static QString dashUsageBar(int percent, int width = 14) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    int filled = percent * width / 100;
    QString color = (percent >= 90) ? "#c0392b" : (percent >= 70) ? "#e67e22" : "#27ae60";
    return QString("<span style='font-family:monospace; color:%1;'>%2</span>"
                   "<span style='font-family:monospace; color:#555;'>%3</span>"
                   " <b>%4%</b>")
        .arg(color, QString("█").repeated(filled), QString("░").repeated(width - filled))
        .arg(percent);
}

void MainWindow::refreshDashboard() {
    // One-time visual setup: card styling, logo header, expanding grid
    static bool dashStyled = false;
    if (!dashStyled) {
        dashStyled = true;
        ui->dashboardTab->setStyleSheet(
            "QGroupBox {"
            "  background: rgba(127,127,127,0.07);"
            "  border: 1px solid rgba(127,127,127,0.25);"
            "  border-top: 3px solid #27ae60;"
            "  border-radius: 10px;"
            "  margin-top: 14px;"
            "  padding: 16px 12px 8px 12px;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "}"
            "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }"
            "QGroupBox QLabel { font-weight: normal; font-size: 14px; }"
            "QGroupBox QPushButton {"
            "  border: 1px solid rgba(127,127,127,0.3);"
            "  border-radius: 6px;"
            "  padding: 6px;"
            "  background: rgba(127,127,127,0.12);"
            "}"
            "QGroupBox QPushButton:hover { background: rgba(39,174,96,0.25); }");

        ui->dashTitleLabel->setTextFormat(Qt::RichText);
        ui->dashTitleLabel->setText(
            "<img src=':/images/XetalEngine.png' height='44' style='vertical-align:middle;'>"
            "&nbsp;&nbsp;<span style='font-size:15px; color:#888;'>System at a glance</span>");

        const QList<QGroupBox*> cards = {ui->dashDrivesGroup, ui->dashServicesGroup, ui->dashUpdatesGroup,
                                         ui->dashDiskGroup, ui->dashIsoGroup, ui->dashSystemGroup};
        for (QGroupBox *card : cards) {
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            card->setMinimumHeight(210);
        }
        ui->dashGrid->setRowStretch(0, 1);
        ui->dashGrid->setRowStretch(1, 1);
        for (int c = 0; c < 3; c++) ui->dashGrid->setColumnStretch(c, 1);
        ui->dashGrid->setHorizontalSpacing(14);
        ui->dashGrid->setVerticalSpacing(14);

        // Promo banner for Linux Studio 2026 (our IDE)
        ui->dashPromoLabel->setTextFormat(Qt::RichText);
        ui->dashPromoLabel->setStyleSheet(
            "QLabel {"
            "  background: rgba(192,57,43,0.10);"
            "  border: 1px solid rgba(192,57,43,0.35);"
            "  border-left: 3px solid #c0392b;"
            "  border-right: 3px solid #27ae60;"
            "  border-radius: 10px;"
            "  padding: 10px;"
            "  font-size: 14px;"
            "}");
        ui->dashPromoLabel->setText(
            "🚀 <b>Love C++?</b> Meet <span style='color:#c0392b; font-weight:bold;'>LINUX</span> "
            "<span style='color:#27ae60; font-weight:bold;'>STUDIO 2026</span> — "
            "the brand-new IDE built <i>for Linux, on Linux</i>, from the same team that made this app. "
            "&nbsp;<a href='https://www.xetal.net' style='color:#3498db; font-weight:bold;'>Grab it at xetal.net →</a>");
    }

    // ---- 🧠 System card ----
    {
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int, QProcess::ExitStatus) {
                    QString cpu, gpu, kernel, uptime, ramText;
                    int ramPct = -1;
                    const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n');
                    for (const QString &line : lines) {
                        if (line.startsWith("CPU:")) cpu = line.mid(4).trimmed();
                        else if (line.startsWith("GPU:")) gpu = line.mid(4).trimmed();
                        else if (line.startsWith("KERNEL:")) kernel = line.mid(7).trimmed();
                        else if (line.startsWith("UPTIME:")) uptime = line.mid(7).trimmed();
                        else if (line.startsWith("RAMPCT:")) ramPct = line.mid(7).trimmed().toInt();
                        else if (line.startsWith("RAM:")) ramText = line.mid(4).trimmed();
                    }
                    QString html = QString(
                        "<p style='margin:0;'><b>%1</b><br>"
                        "<span style='color:#888;'>%2</span></p>"
                        "<p style='margin:6px 0 0 0;'>Kernel <b>%3</b> &nbsp;·&nbsp; up %4</p>"
                        "<p style='margin:8px 0 0 0;'>RAM &nbsp;%5<br>"
                        "<span style='color:#888;'>%6</span></p>")
                        .arg(cpu.toHtmlEscaped(), gpu.toHtmlEscaped(), kernel.toHtmlEscaped(),
                             uptime.toHtmlEscaped(), dashUsageBar(ramPct), ramText.toHtmlEscaped());
                    ui->dashSystemLabel->setTextFormat(Qt::RichText);
                    ui->dashSystemLabel->setText(html);
                    proc->deleteLater();
                });
        proc->start("bash", QStringList() << "-c" <<
            "echo \"CPU: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | sed 's/^ //')\"; "
            "echo \"GPU: $(lspci 2>/dev/null | grep -Ei 'vga|3d controller' | head -1 | sed 's/.*: //; s/ (rev.*//')\"; "
            "echo \"KERNEL: $(uname -r)\"; "
            "echo \"UPTIME: $(uptime -p | sed 's/^up //')\"; "
            "free -b | awk '/^Mem:/{printf \"RAMPCT: %d\\n\", $3*100/$2}'; "
            "free -h | awk '/^Mem:/{printf \"RAM: %s used of %s\\n\", $3, $2}'");
    }

    // ---- 📊 Disk space card (usage bars for / and /home) ----
    {
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int, QProcess::ExitStatus) {
                    QString html;
                    const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                    for (const QString &line : lines) {
                        QStringList parts = line.simplified().split(' ');
                        if (parts.size() < 4) continue;
                        int pct = parts[3].replace("%", "").toInt();
                        html += QString("<p style='margin:0 0 10px 0;'><b>%1</b><br>%2<br>"
                                        "<span style='color:#888;'>%3 used of %4</span></p>")
                                .arg(parts[0].toHtmlEscaped(), dashUsageBar(pct), parts[1], parts[2]);
                    }
                    ui->dashDiskLabel->setTextFormat(Qt::RichText);
                    ui->dashDiskLabel->setText(html.isEmpty() ? "No data" : html);
                    proc->deleteLater();
                });
        proc->start("bash", QStringList() << "-c" <<
            "df -h --output=target,used,size,pcent / /home 2>/dev/null | tail -n +2 | sort -u");
    }

    // ---- 🧩 Services card ----
    {
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int, QProcess::ExitStatus) {
                    const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                    int running = 0;
                    QStringList failed;
                    for (const QString &line : lines) {
                        if (line.startsWith("RUNNING:")) running = line.mid(8).trimmed().toInt();
                        else if (line.startsWith("FAILED:")) failed << line.mid(7).trimmed();
                    }
                    QString html;
                    if (failed.isEmpty()) {
                        html = QString("<p style='margin:0; font-size:34px; color:#27ae60;'><b>✔</b></p>"
                                       "<p style='margin:4px 0 0 0; color:#27ae60;'><b>All services healthy</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%1 services running</p>")
                               .arg(running);
                    } else {
                        html = QString("<p style='margin:0; font-size:34px; color:#c0392b;'><b>%1</b></p>"
                                       "<p style='margin:4px 0 0 0; color:#c0392b;'><b>failed service(s)</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%2</p>")
                               .arg(failed.size())
                               .arg(failed.mid(0, 4).join("<br>").toHtmlEscaped());
                    }
                    ui->dashServicesLabel->setTextFormat(Qt::RichText);
                    ui->dashServicesLabel->setText(html);
                    proc->deleteLater();
                });
        proc->start("bash", QStringList() << "-c" <<
            "echo \"RUNNING: $(systemctl list-units --type=service --state=running --no-legend --plain | wc -l)\"; "
            "systemctl --failed --no-legend --plain | awk '{print \"FAILED: \" $1}'");
    }

    // ---- ⬆️ Updates card ----
    {
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int exitCode, QProcess::ExitStatus) {
                    const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                    QString html;
                    if (exitCode == 2 || (exitCode == 0 && lines.isEmpty())) {
                        html = "<p style='margin:0; font-size:34px; color:#27ae60;'><b>0</b></p>"
                               "<p style='margin:4px 0 0 0; color:#27ae60;'><b>System is up to date</b></p>";
                    } else if (exitCode == 0) {
                        QStringList names;
                        for (const QString &line : lines) names << line.section(' ', 0, 0);
                        QString preview = names.mid(0, 4).join(", ");
                        if (names.size() > 4) preview += QString(" +%1 more").arg(names.size() - 4);
                        QString color = (lines.size() > 50) ? "#e67e22" : "#3498db";
                        html = QString("<p style='margin:0; font-size:34px; color:%1;'><b>%2</b></p>"
                                       "<p style='margin:4px 0 0 0;'><b>update(s) pending</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%3</p>")
                               .arg(color).arg(lines.size()).arg(preview.toHtmlEscaped());
                    } else {
                        html = "<p style='color:#888;'>Install <b>pacman-contrib</b> to enable update checks.</p>";
                    }
                    ui->dashUpdatesLabel->setTextFormat(Qt::RichText);
                    ui->dashUpdatesLabel->setText(html);
                    proc->deleteLater();
                });
        proc->start("checkupdates", QStringList());
        connect(proc, &QProcess::errorOccurred, [this, proc](QProcess::ProcessError) {
            ui->dashUpdatesLabel->setText("<p style='color:#888;'>Install <b>pacman-contrib</b> to enable update checks.</p>");
            proc->deleteLater();
        });
    }

    // ---- 📀 Safety ISO card ----
    {
        QDir isoDir(QDir::homePath() + "/iso/xiso/output");
        QFileInfoList isos = isoDir.entryInfoList(QStringList() << "*.iso", QDir::Files, QDir::Time);
        QString html;
        if (isos.isEmpty()) {
            html = "<p style='margin:0; font-size:34px; color:#e67e22;'><b>—</b></p>"
                   "<p style='margin:4px 0 0 0; color:#e67e22;'><b>No safety ISO yet</b></p>"
                   "<p style='margin:6px 0 0 0; color:#888;'>Create one in the System ISO tab —<br>"
                   "your insurance against drive failure.</p>";
        } else {
            const QFileInfo &newest = isos.first();
            qint64 days = newest.lastModified().daysTo(QDateTime::currentDateTime());
            QString age = (days == 0) ? "today" : (days == 1) ? "yesterday" : QString("%1 days ago").arg(days);
            QString color = (days > 30) ? "#e67e22" : "#27ae60";
            html = QString("<p style='margin:0; font-size:34px; color:%1;'><b>%2</b></p>"
                           "<p style='margin:4px 0 0 0; color:%1;'><b>last safety ISO</b></p>"
                           "<p style='margin:6px 0 0 0; color:#888;'>%3<br>%4</p>")
                   .arg(color, age, newest.fileName().toHtmlEscaped(), formatSize(newest.size()));
        }
        ui->dashIsoLabel->setTextFormat(Qt::RichText);
        ui->dashIsoLabel->setText(html);
    }

    // ---- 💾 Drive health card (sudo only if already cached — never prompts) ----
    {
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, proc](int, QProcess::ExitStatus) {
                    QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
                    QString html;
                    if (out == "NOAUTH") {
                        html = "<p style='color:#888;'>🔒 Health checks need a sudo session.<br><br>"
                               "Use <b>Load Labels</b> or <b>Health Check</b> in the Drives tab once, "
                               "then Refresh here.</p>";
                    } else if (out.isEmpty()) {
                        html = "<p style='color:#888;'>No SMART data<br>(is smartmontools installed?)</p>";
                    } else {
                        // Two-column table so many drives stay compact
                        const QStringList entries = out.split('\n', Qt::SkipEmptyParts);
                        html = "<table cellspacing='2'>";
                        for (int i = 0; i < entries.size(); i += 2) {
                            html += "<tr>";
                            for (int j = i; j < i + 2 && j < entries.size(); j++) {
                                QString e = entries[j];
                                QString color = e.contains("❌") ? "#c0392b" : e.contains("⚠️") ? "#e67e22" : "";
                                html += QString("<td style='padding-right:16px; %1'>%2</td>")
                                        .arg(color.isEmpty() ? "" : "color:" + color + "; font-weight:bold;",
                                             e.toHtmlEscaped());
                            }
                            html += "</tr>";
                        }
                        html += "</table>";
                    }
                    ui->dashDrivesLabel->setTextFormat(Qt::RichText);
                    ui->dashDrivesLabel->setText(html);
                    proc->deleteLater();
                });
        proc->start("bash", QStringList() << "-c" <<
            "if ! sudo -n true 2>/dev/null; then echo NOAUTH; exit 0; fi; "
            "for d in $(lsblk -dpno NAME,TYPE | awk '$2==\"disk\"{print $1}'); do "
            "  h=$(sudo -n smartctl -H \"$d\" 2>/dev/null | grep -Ei 'overall-health|SMART Health Status'); "
            "  case \"$h\" in *PASSED*|*OK*) v='✅';; *FAILED*) v='❌ FAILING';; *) v='?';; esac; "
            "  s=$(sudo -n smartctl -a \"$d\" 2>/dev/null | grep 'Available Spare:' | awk '{print $3}'); "
            "  if [ -n \"$s\" ] && [ \"${s%\\%}\" -lt 80 ] 2>/dev/null && [ \"$v\" = '✅' ]; then v=\"⚠️ spare $s\"; fi; "
            "  echo \"$(basename $d): $v\"; "
            "done");
    }
}

void MainWindow::on_dashRefreshButton_clicked() {
    refreshDashboard();
}
