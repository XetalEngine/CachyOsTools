
// Dashboard tab: styled system-at-a-glance cards. Every card loads
// asynchronously so opening the tab never blocks the UI.

#include <QGroupBox>

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
    // Card styling follows the current theme accent (re-applied on each refresh
    // so theme changes take effect immediately)
    QColor dashAccent(themeAccent.isEmpty() ? "#27ae60" : themeAccent);
    auto dashRgba = [&dashAccent](double alpha) {
        return QString("rgba(%1,%2,%3,%4)").arg(dashAccent.red()).arg(dashAccent.green()).arg(dashAccent.blue()).arg(alpha);
    };
    ui->dashboardTab->setStyleSheet(QString(
        "QGroupBox {"
        "  background: rgba(127,127,127,0.07);"
        "  border: 1px solid rgba(127,127,127,0.25);"
        "  border-top: 3px solid %1;"
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
        "QGroupBox QPushButton:hover { background: %2; }")
        .arg(dashAccent.name(), dashRgba(0.25)));

    // One-time layout setup: logo header, expanding grid, promo banner
    static bool dashStyled = false;
    if (!dashStyled) {
        dashStyled = true;

        ui->dashTitleLabel->setTextFormat(Qt::RichText);
        ui->dashTitleLabel->setText(
            QString("<img src=':/images/XetalEngine.png' height='44' style='vertical-align:middle;'>"
                    "&nbsp;&nbsp;<span style='font-size:15px; color:#888;'>%1</span>")
                .arg(tr("System at a glance")));

        // Language selector: flag dropdown, persisted choice, applied on restart
        ui->dashLangCombo->setIconSize(QSize(24, 16));
        ui->dashLangCombo->addItem(QIcon(":/images/flag_us.png"), "English", "en");
        ui->dashLangCombo->addItem(QIcon(":/images/flag_br.png"), "Português", "pt_BR");
        ui->dashLangCombo->addItem(QIcon(":/images/flag_de.png"), "Deutsch", "de");
        {
            // Show the effective language (env override wins, like in main.cpp)
            QString cur = qEnvironmentVariable("CACHYOSTOOLS_LANG");
            if (cur.isEmpty()) {
                cur = QSettings("CachyOsTools", "CachyOsTools").value("language/code", "en").toString();
            }
            int langIdx = ui->dashLangCombo->findData(cur);
            if (langIdx >= 0) ui->dashLangCombo->setCurrentIndex(langIdx);
        }
        // Self-updater: orange button appears when origin/main is ahead of us
        dashUpdateBtn = new QPushButton(ui->dashboardTab);
        dashUpdateBtn->setStyleSheet(
            "QPushButton { background:#e67e22; color:white; font-weight:bold; border-radius:6px; padding:6px 14px; }"
            "QPushButton:hover { background:#f39c12; }");
        dashUpdateBtn->hide();
        ui->dashTopLayout->insertWidget(2, dashUpdateBtn);
        connect(dashUpdateBtn, &QPushButton::clicked, this, &MainWindow::runAppUpdate);
        QTimer::singleShot(3000, this, &MainWindow::checkForAppUpdates);

        connect(ui->dashLangCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
            QString code = ui->dashLangCombo->itemData(index).toString();
            QSettings settings("CachyOsTools", "CachyOsTools");
            if (settings.value("language/code", "en").toString() == code) return;
            settings.setValue("language/code", code);
            settings.sync();
            if (QMessageBox::question(this, tr("Language Changed"),
                                      tr("The new language takes effect after a restart.\n\nRestart CachyOsTools now?"))
                == QMessageBox::Yes) {
                QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
                qApp->quit();
            }
        });

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
            "the brand-new IDE built <i>for Linux, on Linux</i>, from the same dev that made this app. "
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
                        else if (line.startsWith("RAM:")) {
                            const QStringList ram = line.mid(4).trimmed().split(' ', Qt::SkipEmptyParts);
                            if (ram.size() == 2) ramText = tr("%1 used of %2").arg(ram[0], ram[1]);
                        }
                    }
                    QString html = QString(
                        "<p style='margin:0;'><b>%1</b><br>"
                        "<span style='color:#888;'>%2</span></p>"
                        "<p style='margin:6px 0 0 0;'>%3</p>"
                        "<p style='margin:8px 0 0 0;'>RAM &nbsp;%4<br>"
                        "<span style='color:#888;'>%5</span></p>")
                        .arg(cpu.toHtmlEscaped(), gpu.toHtmlEscaped(),
                             tr("Kernel <b>%1</b> &nbsp;·&nbsp; up %2").arg(kernel.toHtmlEscaped(), uptime.toHtmlEscaped()),
                             dashUsageBar(ramPct), ramText.toHtmlEscaped());
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
            "free -h | awk '/^Mem:/{printf \"RAM: %s %s\\n\", $3, $2}'");
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
                                        "<span style='color:#888;'>%3</span></p>")
                                .arg(parts[0].toHtmlEscaped(), dashUsageBar(pct),
                                     tr("%1 used of %2").arg(parts[1], parts[2]));
                    }
                    ui->dashDiskLabel->setTextFormat(Qt::RichText);
                    ui->dashDiskLabel->setText(html.isEmpty() ? tr("No data") : html);
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
                                       "<p style='margin:4px 0 0 0; color:#27ae60;'><b>%1</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%2</p>")
                               .arg(tr("All services healthy"), tr("%1 services running").arg(running));
                    } else {
                        html = QString("<p style='margin:0; font-size:34px; color:#c0392b;'><b>%1</b></p>"
                                       "<p style='margin:4px 0 0 0; color:#c0392b;'><b>%2</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%3</p>")
                               .arg(failed.size())
                               .arg(tr("failed service(s)"), failed.mid(0, 4).join("<br>").toHtmlEscaped());
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
                        html = QString("<p style='margin:0; font-size:34px; color:#27ae60;'><b>0</b></p>"
                                       "<p style='margin:4px 0 0 0; color:#27ae60;'><b>%1</b></p>")
                               .arg(tr("System is up to date"));
                    } else if (exitCode == 0) {
                        QStringList names;
                        for (const QString &line : lines) names << line.section(' ', 0, 0);
                        QString preview = names.mid(0, 4).join(", ");
                        if (names.size() > 4) preview += tr(" +%1 more").arg(names.size() - 4);
                        QString color = (lines.size() > 50) ? "#e67e22" : "#3498db";
                        html = QString("<p style='margin:0; font-size:34px; color:%1;'><b>%2</b></p>"
                                       "<p style='margin:4px 0 0 0;'><b>%3</b></p>"
                                       "<p style='margin:6px 0 0 0; color:#888;'>%4</p>")
                               .arg(color).arg(lines.size()).arg(tr("update(s) pending"), preview.toHtmlEscaped());
                    } else {
                        html = tr("<p style='color:#888;'>Install <b>pacman-contrib</b> to enable update checks.</p>");
                    }
                    ui->dashUpdatesLabel->setTextFormat(Qt::RichText);
                    ui->dashUpdatesLabel->setText(html);
                    proc->deleteLater();
                });
        proc->start("checkupdates", QStringList());
        connect(proc, &QProcess::errorOccurred, [this, proc](QProcess::ProcessError) {
            ui->dashUpdatesLabel->setText(tr("<p style='color:#888;'>Install <b>pacman-contrib</b> to enable update checks.</p>"));
            proc->deleteLater();
        });
    }

    // ---- 📀 Safety ISO card ----
    {
        QDir isoDir(QDir::homePath() + "/iso/xiso/output");
        QFileInfoList isos = isoDir.entryInfoList(QStringList() << "*.iso", QDir::Files, QDir::Time);
        QString html;
        if (isos.isEmpty()) {
            html = QString("<p style='margin:0; font-size:34px; color:#e67e22;'><b>—</b></p>"
                           "<p style='margin:4px 0 0 0; color:#e67e22;'><b>%1</b></p>"
                           "<p style='margin:6px 0 0 0; color:#888;'>%2</p>")
                   .arg(tr("No safety ISO yet"),
                        tr("Create one in the System ISO tab —<br>your insurance against drive failure."));
        } else {
            const QFileInfo &newest = isos.first();
            qint64 days = newest.lastModified().daysTo(QDateTime::currentDateTime());
            QString age = (days == 0) ? tr("today") : (days == 1) ? tr("yesterday") : tr("%1 days ago").arg(days);
            QString color = (days > 30) ? "#e67e22" : "#27ae60";
            html = QString("<p style='margin:0; font-size:34px; color:%1;'><b>%2</b></p>"
                           "<p style='margin:4px 0 0 0; color:%1;'><b>%3</b></p>"
                           "<p style='margin:6px 0 0 0; color:#888;'>%4<br>%5</p>")
                   .arg(color, age, tr("last safety ISO"), newest.fileName().toHtmlEscaped(), formatSize(newest.size()));
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
                        html = tr("<p style='color:#888;'>🔒 Health checks need a sudo session.<br><br>"
                                  "Use <b>Load Labels</b> or <b>Health Check</b> in the Drives tab once, "
                                  "then Refresh here.</p>");
                    } else if (out.isEmpty()) {
                        html = tr("<p style='color:#888;'>No SMART data<br>(is smartmontools installed?)</p>");
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

// ---- Self-updater: git-based, source-build aware ----------------------------

// The repo the running binary came from: walk up from the executable until a
// git checkout of this project is found (root binary or a build/ subdir).
static QString xetalRepoDir() {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (dir.exists(".git") && dir.exists("CMakeLists.txt") && dir.exists("build.sh"))
            return dir.absolutePath();
        if (!dir.cdUp()) break;
    }
    return QString();
}

void MainWindow::checkForAppUpdates() {
    // Testing aid: CACHYOSTOOLS_FAKE_UPDATE=<n> pretends n commits are available
    const QString fake = qEnvironmentVariable("CACHYOSTOOLS_FAKE_UPDATE");
    if (!fake.isEmpty()) {
        dashUpdateBtn->setText(tr("⬆️ UPDATE — %1 new commit(s) available!").arg(fake.toInt()));
        dashUpdateBtn->show();
        return;
    }

    const QString repo = xetalRepoDir();
    if (repo.isEmpty()) return;                       // not running from a git checkout

    QProcess *proc = new QProcess(this);
    // Silent failure by design: no network / no remote just means no button
    QTimer *watchdog = new QTimer(proc);
    watchdog->setSingleShot(true);
    connect(watchdog, &QTimer::timeout, proc, &QProcess::kill);
    watchdog->start(20000);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int exitCode, QProcess::ExitStatus) {
                const QStringList lines = QString::fromUtf8(proc->readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
                if (exitCode == 0 && !lines.isEmpty()) {
                    bool ok = false;
                    int behind = lines[0].trimmed().toInt(&ok);
                    if (ok && behind > 0) {
                        dashUpdateBtn->setText(tr("⬆️ UPDATE — %1 new commit(s) available!").arg(behind));
                        dashUpdateBtn->setToolTip(lines.size() > 1
                            ? tr("Latest change: %1\nClick to pull, rebuild and restart.").arg(lines[1].trimmed())
                            : tr("Click to pull, rebuild and restart."));
                        dashUpdateBtn->show();
                    } else {
                        dashUpdateBtn->hide();
                    }
                }
                proc->deleteLater();
            });
    proc->start("bash", QStringList() << "-c" << QString(
        "cd '%1' && git fetch -q origin 2>/dev/null && "
        "git rev-list --count HEAD..origin/main 2>/dev/null && "
        "git log -1 --format=%%s origin/main 2>/dev/null").arg(repo));
}

void MainWindow::runAppUpdate() {
    const QString repo = xetalRepoDir();
    if (repo.isEmpty()) {
        QMessageBox::information(this, tr("Not a Git Install"),
            tr("The running app was not started from a git checkout, so it cannot self-update."));
        return;
    }

    // Uncommitted changes would make git pull fail — tell the user up front
    QProcess st;
    st.start("git", QStringList() << "-C" << repo << "status" << "--porcelain");
    st.waitForFinished(3000);
    const bool dirty = !QString::fromUtf8(st.readAllStandardOutput()).trimmed().isEmpty();
    QString warning = dirty
        ? tr("\n\n⚠ The repo has local uncommitted changes — the pull will refuse to overwrite them. Commit or stash first if the update fails.")
        : QString();

    if (QMessageBox::question(this, tr("Update CachyOsTools"),
            tr("This will, in a visible terminal:\n\n"
               "1. Pull the newest version into:\n    %1\n"
               "2. Rebuild the app\n"
               "3. Offer to restart when done%2\n\nContinue?").arg(repo, warning)) != QMessageBox::Yes)
        return;

    const QString script = QString(
        "cd '%1' || exit 1; "
        "echo '== Pulling latest version...'; "
        "git pull --ff-only || { echo; echo '❌ Pull failed (local changes or diverged history).'; read -p 'Press Enter to close...'; exit 1; }; "
        "echo; echo '== Rebuilding...'; "
        "BD=$(ls -td build/*/ 2>/dev/null | head -1); "
        "if [ -n \"$BD\" ] && [ -f \"${BD}CMakeCache.txt\" ]; then "
        "  cmake --build \"$BD\" -j$(nproc) && cp -f \"${BD}CachyOsTools\" ./CachyOsTools; "
        "else ./build.sh; fi "
        "&& { echo; echo '== ✅ Update complete.'; } || echo '❌ Build failed - see output above.'; "
        "read -p 'Press Enter to close this window...'").arg(repo);

    // Run as a child process (not detached) so we know when the update finished
    QString term;
    for (const QString &t : {QString("konsole"), QString("gnome-terminal"), QString("xterm"), QString("alacritty"), QString("kitty")}) {
        if (!QStandardPaths::findExecutable(t).isEmpty()) { term = t; break; }
    }
    if (term.isEmpty()) {
        QMessageBox::warning(this, tr("Terminal Not Found"),
            tr("Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty"));
        return;
    }
    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
                proc->deleteLater();
                if (QMessageBox::question(this, tr("Restart CachyOsTools?"),
                        tr("If the update succeeded, a restart loads the new version.\n\nRestart now?")) == QMessageBox::Yes) {
                    QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
                    qApp->quit();
                }
            });
    proc->start(term, QStringList() << "-e" << "bash" << "-c" << script);
}
