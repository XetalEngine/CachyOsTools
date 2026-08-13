
// Dashboard -> 🌡️ Sensors & Power sub-tab (sits right after Devices).
//
// Left:  every hwmon reading on the box — temperatures, fans, voltages, power —
//        live-refreshing, colour-coded against each chip's own max/crit limits.
// Right: the things you actually turn when a reading is wrong — power profile,
//        CPU governor, battery charge limit, and the fancontrol service.
//
// Readings are unprivileged (sysfs and lm_sensors are world-readable). Only the
// three write actions need root, and each of those goes through a visible
// terminal like the rest of the app.

#include <QGroupBox>
#include <QFormLayout>
#include <QSplitter>
#include <QSpinBox>
#include "script_helpers.h"

// Current contents of a combo box. The power panel refills itself every couple
// of seconds; comparing against this keeps us from clearing a combo the user is
// in the middle of using when nothing actually changed.
static QStringList profileItems(QComboBox *combo) {
    QStringList items;
    for (int i = 0; i < combo->count(); ++i) items << combo->itemText(i);
    return items;
}

// ============================================================
// Tab construction
// ============================================================
void MainWindow::setupSensorsPowerTab() {
    sensSubTab = new QWidget();
    sensSubTab->setObjectName("sensorsPowerSubTab");
    QVBoxLayout *root = new QVBoxLayout(sensSubTab);

    // --- top bar ---------------------------------------------------------
    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *refreshBtn = new QPushButton(tr("🔄 Refresh"), sensSubTab);
    sensLiveCheck = new QCheckBox(tr("Live"), sensSubTab);
    sensLiveCheck->setToolTip(tr("Re-read every 2 seconds while this tab is open."));
    sensLiveCheck->setChecked(true);
    sensSummaryLabel = new QLabel(tr("Reading sensors…"), sensSubTab);
    sensSummaryLabel->setTextFormat(Qt::RichText);
    bar->addWidget(refreshBtn);
    bar->addWidget(sensLiveCheck);
    bar->addWidget(sensSummaryLabel, 1);
    root->addLayout(bar);

    QSplitter *split = new QSplitter(Qt::Horizontal, sensSubTab);

    // --- left: the sensor tree -------------------------------------------
    // Built parentless and handed to the splitter explicitly — creating them as
    // splitter children makes QSplitter auto-add them and the sizing goes odd.
    QWidget *leftBox = new QWidget();
    QVBoxLayout *leftLay = new QVBoxLayout(leftBox);
    leftLay->setContentsMargins(0, 0, 0, 0);

    sensMissingLabel = new QLabel(leftBox);
    sensMissingLabel->setWordWrap(true);
    sensMissingLabel->setTextFormat(Qt::RichText);
    sensMissingLabel->setVisible(false);
    leftLay->addWidget(sensMissingLabel);

    sensTree = new QTreeWidget(leftBox);
    sensTree->setColumnCount(4);
    sensTree->setHeaderLabels(QStringList() << tr("Sensor") << tr("Reading") << tr("High") << tr("Critical"));
    sensTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < 4; ++c) sensTree->header()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    sensTree->setAlternatingRowColors(true);
    sensTree->setRootIsDecorated(true);
    leftLay->addWidget(sensTree);

    QHBoxLayout *sensBar = new QHBoxLayout();
    QPushButton *detectBtn = new QPushButton(tr("🔎 Run sensors-detect"), leftBox);
    detectBtn->setToolTip(tr("Probes your hardware for sensor chips it can't find automatically. "
                             "Answer the defaults unless you know better."));
    sensBar->addStretch();
    sensBar->addWidget(detectBtn);
    leftLay->addLayout(sensBar);

    // --- right: the controls ---------------------------------------------
    QWidget *rightBox = new QWidget();
    QVBoxLayout *rightLay = new QVBoxLayout(rightBox);
    rightLay->setContentsMargins(0, 0, 0, 0);

    // Power profile + CPU governor
    QGroupBox *powBox = new QGroupBox(tr("⚡ Power"), rightBox);
    QFormLayout *powForm = new QFormLayout(powBox);
    powProfileCombo = new QComboBox(powBox);
    powProfileCombo->setToolTip(tr("power-profiles-daemon — the same switch your desktop's battery menu uses."));
    powForm->addRow(tr("Profile:"), powProfileCombo);
    powGovCombo = new QComboBox(powBox);
    powGovCombo->setToolTip(tr("CPU frequency governor, applied to every core. "
                               "Set it permanently on the Tweaks tab; this switch is for right now."));
    powForm->addRow(tr("CPU governor:"), powGovCombo);
    powEppCombo = new QComboBox(powBox);
    powEppCombo->setToolTip(tr("Energy/performance preference — the finer knob intel_pstate and amd_pstate expose."));
    powForm->addRow(tr("Energy preference:"), powEppCombo);
    // Values like "performance" and "balance_performance" must not be clipped
    for (QComboBox *c : {powProfileCombo, powGovCombo, powEppCombo})
        c->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    powProfileLabel = new QLabel(powBox);
    powProfileLabel->setStyleSheet("color:#888;");
    powProfileLabel->setWordWrap(true);
    powForm->addRow(powProfileLabel);
    rightLay->addWidget(powBox);

    // Battery
    powBatteryGroup = new QGroupBox(tr("🔋 Battery"), rightBox);
    QVBoxLayout *batLay = new QVBoxLayout(powBatteryGroup);
    powBatteryLabel = new QLabel(powBatteryGroup);
    powBatteryLabel->setTextFormat(Qt::RichText);
    powBatteryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    batLay->addWidget(powBatteryLabel);
    QHBoxLayout *thrLay = new QHBoxLayout();
    thrLay->addWidget(new QLabel(tr("Stop charging at:"), powBatteryGroup));
    powThresholdSpin = new QSpinBox(powBatteryGroup);
    powThresholdSpin->setRange(20, 100);
    powThresholdSpin->setSuffix(" %");
    powThresholdSpin->setValue(100);
    powThresholdSpin->setToolTip(tr("Keeping a laptop below ~80% dramatically slows battery wear. "
                                    "100% disables the limit."));
    thrLay->addWidget(powThresholdSpin);
    QPushButton *thrApplyBtn = new QPushButton(tr("Apply"), powBatteryGroup);
    QPushButton *thrPersistBtn = new QPushButton(tr("Apply + Keep After Reboot"), powBatteryGroup);
    thrPersistBtn->setToolTip(tr("Also installs a small systemd service that re-applies the limit on every boot."));
    thrLay->addWidget(thrApplyBtn);
    thrLay->addWidget(thrPersistBtn);
    thrLay->addStretch();
    batLay->addLayout(thrLay);
    rightLay->addWidget(powBatteryGroup);

    // Fans
    QGroupBox *fanBox = new QGroupBox(tr("🌀 Fan control"), rightBox);
    QVBoxLayout *fanLay = new QVBoxLayout(fanBox);
    fanStatusLabel = new QLabel(fanBox);
    fanStatusLabel->setTextFormat(Qt::RichText);
    fanStatusLabel->setWordWrap(true);
    fanLay->addWidget(fanStatusLabel);
    QHBoxLayout *fanBar = new QHBoxLayout();
    QPushButton *pwmBtn = new QPushButton(tr("🧪 Configure (pwmconfig)"), fanBox);
    QPushButton *fanStartBtn = new QPushButton(tr("▶️ Start"), fanBox);
    QPushButton *fanStopBtn = new QPushButton(tr("⏹️ Stop"), fanBox);
    QPushButton *fanBootBtn = new QPushButton(tr("🔁 Enable at Boot"), fanBox);
    QPushButton *fanEditBtn = new QPushButton(tr("✏️ Edit Curve"), fanBox);
    for (QPushButton *b : {pwmBtn, fanStartBtn, fanStopBtn, fanBootBtn, fanEditBtn}) fanBar->addWidget(b);
    fanBar->addStretch();
    fanLay->addLayout(fanBar);
    rightLay->addWidget(fanBox);

    rightLay->addStretch();

    split->addWidget(leftBox);
    split->addWidget(rightBox);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    root->addWidget(split, 1);   // the splitter, not the tool bar, absorbs the height

    // --- wiring ----------------------------------------------------------
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshSensorsPower);

    sensTimer = new QTimer(this);
    sensTimer->setInterval(2000);
    connect(sensTimer, &QTimer::timeout, this, [this]() {
        // Only poll when the panel is actually on screen.
        if (!sensLiveCheck->isChecked() || !isVisible() || isMinimized()) return;
        if (ui->tabWidget->currentWidget() != ui->dashboardTab) return;
        if (ui->dashSubTabs->currentWidget() != sensSubTab) return;
        refreshSensorsPower();
    });
    sensTimer->start();

    connect(detectBtn, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Run sensors-detect"),
                tr("sensors-detect probes your motherboard for sensor chips and writes the modules it finds "
                   "to /etc/modules-load.d/.\n\nIt asks a lot of yes/no questions — the defaults are safe. "
                   "Probing can briefly hang some machines, so close anything unsaved first.\n\nContinue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal("sudo sensors-detect; echo ''; echo 'Reload the modules or reboot, then Refresh."
                                 " Press Enter to close.'; read -r");
    });

    connect(powProfileCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        if (sensPopulating) return;
        const QString p = powProfileCombo->currentText();
        QProcess::execute("powerprofilesctl", QStringList() << "set" << p);
        QTimer::singleShot(400, this, [this]() { refreshPowerPanel(); });
    });

    connect(powGovCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        if (sensPopulating) return;
        const QString g = powGovCombo->currentText();
        if (!QRegularExpression("^[a-z_]+$").match(g).hasMatch()) return;
        runScriptInTerminal(repairScript(QString(
            "for f in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo %1 > \"$f\"; done\n"
            "echo 'Governor is now:'; cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\n"
            "echo 'This lasts until reboot — the Tweaks tab makes it permanent.'\n").arg(g)),
            "set_governor");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    });

    connect(powEppCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        if (sensPopulating) return;
        const QString e = powEppCombo->currentText();
        if (!QRegularExpression("^[a-z_]+$").match(e).hasMatch()) return;
        runScriptInTerminal(repairScript(QString(
            "for f in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do echo %1 > \"$f\"; done\n"
            "echo 'Energy preference is now:'; cat /sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference\n").arg(e)),
            "set_epp");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    });

    auto applyThreshold = [this](bool persist) {
        if (sensBatteryPath.isEmpty()) return;
        const int pct = powThresholdSpin->value();
        const QString bat = QFileInfo(sensBatteryPath).fileName();
        if (!QRegularExpression("^[A-Za-z0-9_.-]+$").match(bat).hasMatch()) return;

        QString body = QString(
            "T=/sys/class/power_supply/%1/charge_control_end_threshold\n"
            "[ -w \"$T\" ] || T=/sys/class/power_supply/%1/charge_stop_threshold\n"
            "echo %2 > \"$T\"\n"
            "echo \"Charge limit set to $(cat \"$T\")%.\"\n").arg(bat).arg(pct);

        if (persist) {
            body += QString(
                "UNIT=/etc/systemd/system/battery-charge-threshold.service\n"
                "cat > \"$UNIT\" <<'EOF'\n"
                "[Unit]\n"
                "Description=Set battery charge threshold (CachyOsTools)\n"
                "After=multi-user.target\n"
                "StartLimitBurst=0\n"
                "\n"
                "[Service]\n"
                "Type=oneshot\n"
                "Restart=on-failure\n"
                "ExecStart=/bin/bash -c 'T=/sys/class/power_supply/%1/charge_control_end_threshold;"
                " [ -w \"$T\" ] || T=/sys/class/power_supply/%1/charge_stop_threshold; echo %2 > \"$T\"'\n"
                "\n"
                "[Install]\n"
                "WantedBy=multi-user.target\n"
                "EOF\n"
                "systemctl daemon-reload\n"
                "systemctl enable battery-charge-threshold.service\n"
                "echo 'Installed battery-charge-threshold.service — the limit survives reboots now.'\n"
                "echo 'Remove it later with: sudo systemctl disable --now battery-charge-threshold.service'\n")
                .arg(bat).arg(pct);
        }
        runScriptInTerminal(repairScript(body), "battery_threshold");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    };
    connect(thrApplyBtn, &QPushButton::clicked, this, [applyThreshold]() { applyThreshold(false); });
    connect(thrPersistBtn, &QPushButton::clicked, this, [applyThreshold]() { applyThreshold(true); });

    connect(pwmBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/usr/bin/pwmconfig")) {
            QMessageBox::information(this, tr("lm_sensors Missing"),
                tr("pwmconfig ships with lm_sensors:\n\n  sudo pacman -S lm_sensors"));
            return;
        }
        if (QMessageBox::warning(this, tr("Configure Fan Curves"),
                tr("pwmconfig stops each fan in turn to work out which sensor it belongs to.\n\n"
                   "Your machine runs fanless for a few seconds at a time during this. It is the standard "
                   "procedure and normally harmless, but do not run it under load — close games, compiles "
                   "and renders first.\n\nContinue?"),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
        runSudoCommandInTerminal("sudo pwmconfig; echo ''; echo 'Press Enter to close.'; read -r");
    });
    connect(fanStartBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/etc/fancontrol")) {
            QMessageBox::information(this, tr("No Fan Curve Yet"),
                tr("/etc/fancontrol does not exist. Run Configure (pwmconfig) first — it writes that file."));
            return;
        }
        runScriptInTerminal(repairScript("systemctl start fancontrol\nsystemctl --no-pager status fancontrol || true\n"),
                            "fancontrol_start");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    });
    connect(fanStopBtn, &QPushButton::clicked, this, [this]() {
        runScriptInTerminal(repairScript(
            "systemctl stop fancontrol\n"
            "echo 'fancontrol stopped — the BIOS/EC takes the fans back over.'\n"), "fancontrol_stop");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    });
    connect(fanBootBtn, &QPushButton::clicked, this, [this]() {
        runScriptInTerminal(repairScript("systemctl enable --now fancontrol\nsystemctl --no-pager status fancontrol || true\n"),
                            "fancontrol_enable");
        QTimer::singleShot(2500, this, [this]() { refreshPowerPanel(); });
    });
    connect(fanEditBtn, &QPushButton::clicked, this, [this]() {
        if (!QFile::exists("/etc/fancontrol")) {
            QMessageBox::information(this, tr("No Fan Curve Yet"),
                tr("/etc/fancontrol does not exist yet. Run Configure (pwmconfig) to generate it."));
            return;
        }
        backupConfigFile("/etc/fancontrol", "fancontrol curve");
        openBuiltinEditor("/etc/fancontrol");
    });
}

// ============================================================
// Sensor readings
// ============================================================
void MainWindow::refreshSensorsPower() {
    refreshPowerPanel();

    if (sensBusy) return;   // a 2s tick must never stack processes
    sensBusy = true;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        sensBusy = false;
        const QString out = QString::fromUtf8(proc->readAllStandardOutput());
        proc->deleteLater();

        struct Reading { QString chip, label, unit, value, max, crit; };
        QList<Reading> readings;
        bool haveSensorsBin = false;
        // NVMe and some SuperIO chips report unset thresholds as a sentinel that
        // decodes to nonsense like 65261.8 °C. Treat those as "no limit".
        auto sane = [](const QString &unit, const QString &raw) -> QString {
            if (raw.isEmpty()) return raw;
            if (unit != "temp") return raw;
            const double d = raw.toDouble();
            return (d > 200.0 || d < -100.0) ? QString() : raw;
        };
        for (const QString &line : out.split('\n')) {
            if (line == "SENSORSBIN=1") { haveSensorsBin = true; continue; }
            if (!line.startsWith("S|")) continue;
            const QStringList f = line.mid(2).split('|');
            if (f.size() < 6 || f[3].isEmpty()) continue;   // f[3] is the reading itself
            readings.append({f[0], f[1], f[2], f[3], sane(f[2], f[4]), sane(f[2], f[5])});
        }

        if (readings.isEmpty()) {
            sensTree->clear();
            sensChipItems.clear();
            sensRowItems.clear();
            sensMissingLabel->setVisible(true);
            sensMissingLabel->setText(haveSensorsBin
                ? tr("<b style='color:#e67e22;'>No sensor chips reported anything.</b><br>"
                     "Your board's chip may need probing — try <i>Run sensors-detect</i> below.")
                : tr("<b style='color:#e67e22;'>lm_sensors is not installed.</b><br>"
                     "Install it for full readings:<br><code>sudo pacman -S lm_sensors</code><br>"
                     "then run <i>sensors-detect</i>."));
            sensSummaryLabel->setText(tr("<span style='color:#888;'>No readings.</span>"));
            return;
        }
        sensMissingLabel->setVisible(false);

        auto fmt = [](const QString &unit, const QString &raw) -> QString {
            bool ok = false;
            const double d = raw.toDouble(&ok);
            if (!ok) return raw;
            if (unit == "temp")  return QString::number(d, 'f', 1) + " °C";
            if (unit == "fan")   return QString::number(d, 'f', 0) + " RPM";
            if (unit == "pct")   return QString::number(d, 'f', 0) + " %";
            if (unit == "in")    return QString::number(d, 'f', 2) + " V";
            if (unit == "power") return QString::number(d, 'f', 1) + " W";
            if (unit == "curr")  return QString::number(d, 'f', 2) + " A";
            if (unit == "energy")return QString::number(d, 'f', 1) + " J";
            return QString::number(d, 'f', 1);
        };

        // Update in place: rebuilding the tree every 2s would fight the user's
        // scroll position and collapse whatever they expanded.
        QSet<QString> seen;
        double hottest = -300; QString hottestName;
        int fanCount = 0, alerts = 0;

        for (const Reading &r : readings) {
            const QString key = r.chip + "|" + r.label;
            seen.insert(key);

            QTreeWidgetItem *chipItem = sensChipItems.value(r.chip);
            if (!chipItem) {
                chipItem = new QTreeWidgetItem(sensTree);
                chipItem->setText(0, r.chip);
                chipItem->setFirstColumnSpanned(true);
                QFont f = chipItem->font(0);
                f.setBold(true);
                chipItem->setFont(0, f);
                chipItem->setExpanded(true);
                sensChipItems.insert(r.chip, chipItem);
            }

            QTreeWidgetItem *item = sensRowItems.value(key);
            if (!item) {
                item = new QTreeWidgetItem(chipItem);
                item->setText(0, r.label);
                sensRowItems.insert(key, item);
            }
            item->setText(1, fmt(r.unit, r.value));
            item->setText(2, r.max.isEmpty()  ? "—" : fmt(r.unit, r.max));
            item->setText(3, r.crit.isEmpty() ? "—" : fmt(r.unit, r.crit));

            // Colour temperatures against the chip's own limits, not a guess
            QColor colour;
            if (r.unit == "temp") {
                const double v = r.value.toDouble();
                const double crit = r.crit.toDouble();
                const double high = r.max.toDouble();
                if (crit > 0 && v >= crit)            { colour = QColor("#c0392b"); alerts++; }
                else if (high > 0 && v >= high)       { colour = QColor("#e67e22"); alerts++; }
                else if (crit > 0 && v >= crit * 0.9) { colour = QColor("#e67e22"); alerts++; }
                else if (crit <= 0 && high <= 0 && v >= 85) { colour = QColor("#e67e22"); alerts++; }
                else colour = QColor("#27ae60");
                if (v > hottest) { hottest = v; hottestName = r.label; }
            } else if (r.unit == "fan") {
                fanCount++;
                // A fan wired up but reading zero is worth noticing
                colour = (r.value.toDouble() <= 0) ? QColor("#e67e22") : QColor("#27ae60");
            }
            if (colour.isValid()) item->setForeground(1, colour);
        }

        // Drop rows that disappeared (module unloaded, GPU asleep)
        for (auto it = sensRowItems.begin(); it != sensRowItems.end(); ) {
            if (seen.contains(it.key())) { ++it; continue; }
            delete it.value();
            it = sensRowItems.erase(it);
        }
        for (auto it = sensChipItems.begin(); it != sensChipItems.end(); ) {
            if (it.value()->childCount() > 0) { ++it; continue; }
            delete it.value();
            it = sensChipItems.erase(it);
        }

        QString summary;
        if (hottest > -300)
            summary += QString("<b style='color:%1;'>%2 %3 °C</b> <span style='color:#888;'>(%4)</span>")
                           .arg(hottest >= 85 ? "#c0392b" : (hottest >= 70 ? "#e67e22" : "#27ae60"),
                                QString("🌡️"), QString::number(hottest, 'f', 1), hottestName.toHtmlEscaped());
        if (fanCount)
            summary += QString("<span style='color:#888;'> · 🌀 %1</span>").arg(tr("%n fan(s)", "", fanCount));
        if (alerts)
            summary += QString("<span style='color:#e67e22;'> · ⚠️ %1</span>").arg(tr("%n reading(s) near a limit", "", alerts));
        sensSummaryLabel->setText(summary);
    });

    proc->start("bash", QStringList() << "-c" << R"BASH(
if command -v sensors >/dev/null 2>&1; then
  echo "SENSORSBIN=1"
  sensors -u 2>/dev/null | awk '
    function flush() {
      if (label != "" && inp != "") printf "S|%s|%s|%s|%s|%s|%s\n", chip, label, unit, inp, max, crit
      label=""; inp=""; max=""; crit=""; unit=""
    }
    /^Adapter:/ { next }
    /^[^ \t]/ {
      if ($0 ~ /:$/) { flush(); label=substr($0,1,length($0)-1) }
      else { flush(); chip=$0 }
      next
    }
    {
      line=$0; sub(/^[ \t]+/,"",line)
      n=index(line,":"); if (n==0) next
      key=substr(line,1,n-1); val=substr(line,n+1)
      gsub(/^[ \t]+|[ \t]+$/,"",val)
      if (unit=="") { u=key; sub(/[0-9].*$/,"",u); unit=u }
      if (key ~ /_input$/) inp=val
      else if (key ~ /_average$/ && inp=="") inp=val
      else if (key ~ /_max$/) max=val
      else if (key ~ /_crit$/ && crit=="") crit=val
    }
    END { flush() }'
else
  echo "SENSORSBIN=0"
  for h in /sys/class/hwmon/hwmon*; do
    [ -r "$h/name" ] || continue
    chip=$(cat "$h/name")
    for f in "$h"/temp*_input "$h"/fan*_input "$h"/in*_input; do
      [ -r "$f" ] || continue
      b=${f%_input}; n=$(basename "$b")
      if [ -r "${b}_label" ]; then lbl=$(cat "${b}_label"); else lbl=$n; fi
      v=$(cat "$f" 2>/dev/null) || continue
      mx=""; ct=""
      [ -r "${b}_max" ] && mx=$(cat "${b}_max")
      [ -r "${b}_crit" ] && ct=$(cat "${b}_crit")
      case "$n" in
        temp*) u=temp
               v=$(awk "BEGIN{printf \"%.3f\", $v/1000}")
               [ -n "$mx" ] && mx=$(awk "BEGIN{printf \"%.3f\", $mx/1000}")
               [ -n "$ct" ] && ct=$(awk "BEGIN{printf \"%.3f\", $ct/1000}") ;;
        fan*)  u=fan ;;
        in*)   u=in; v=$(awk "BEGIN{printf \"%.3f\", $v/1000}") ;;
        *)     u=other ;;
      esac
      echo "S|$chip|$lbl|$u|$v|$mx|$ct"
    done
  done
fi

# lm_sensors never sees the NVIDIA proprietary stack — ask the driver directly.
if command -v nvidia-smi >/dev/null 2>&1; then
  nvidia-smi --query-gpu=name,temperature.gpu,fan.speed,power.draw \
             --format=csv,noheader,nounits 2>/dev/null |
  while IFS=, read -r name t fan pw; do
    name=$(echo "$name" | sed 's/^ *//; s/ *$//')
    t=$(echo "$t" | tr -d ' '); fan=$(echo "$fan" | tr -d ' '); pw=$(echo "$pw" | tr -d ' ')
    case "$t"   in ''|*[!0-9.]*) ;; *) echo "S|nvidia ($name)|GPU temp|temp|$t||" ;; esac
    case "$fan" in ''|*[!0-9.]*) ;; *) echo "S|nvidia ($name)|GPU fan|pct|$fan||" ;; esac
    case "$pw"  in ''|*[!0-9.]*) ;; *) echo "S|nvidia ($name)|GPU power|power|$pw||" ;; esac
  done
fi
)BASH");
}

// ============================================================
// Power / battery / fan controls
// ============================================================
void MainWindow::refreshPowerPanel() {
    if (sensPowerBusy) return;
    sensPowerBusy = true;

    QProcess *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int, QProcess::ExitStatus) {
        sensPowerBusy = false;
        QHash<QString, QString> v;
        for (const QString &line : QString::fromUtf8(proc->readAllStandardOutput()).split('\n')) {
            const int eq = line.indexOf('=');
            if (eq > 0) v.insert(line.left(eq), line.mid(eq + 1).trimmed());
        }
        proc->deleteLater();

        sensPopulating = true;   // refilling combos must not fire "user changed it"

        // --- power profile ---
        const QStringList profiles = v.value("PROFILES").split(' ', Qt::SkipEmptyParts);
        if (profiles.isEmpty()) {
            powProfileCombo->clear();
            powProfileCombo->setEnabled(false);
            if (v.value("TLP") == "1")
                powProfileLabel->setText(tr("TLP is managing power. power-profiles-daemon is not installed, "
                                            "so there is no profile switch here — use <code>sudo tlp ac</code> / "
                                            "<code>sudo tlp bat</code>."));
            else
                powProfileLabel->setText(tr("No power-profiles-daemon. Install <code>power-profiles-daemon</code> "
                                            "for one-click Performance / Balanced / Power Saver."));
        } else {
            powProfileCombo->setEnabled(true);
            if (profileItems(powProfileCombo) != profiles) {
                powProfileCombo->clear();
                powProfileCombo->addItems(profiles);
            }
            powProfileCombo->setCurrentText(v.value("PROFILE"));
            powProfileLabel->setText(v.value("DEGRADED").isEmpty()
                ? QString()
                : tr("⚠️ Performance mode is degraded: %1").arg(v.value("DEGRADED")));
        }

        // --- governor ---
        const QStringList govs = v.value("GOVS").split(' ', Qt::SkipEmptyParts);
        if (govs.isEmpty()) {
            powGovCombo->clear();
            powGovCombo->setEnabled(false);
        } else {
            powGovCombo->setEnabled(true);
            if (profileItems(powGovCombo) != govs) {
                powGovCombo->clear();
                powGovCombo->addItems(govs);
            }
            powGovCombo->setCurrentText(v.value("GOV"));
        }

        // --- energy performance preference ---
        const QStringList epps = v.value("EPPS").split(' ', Qt::SkipEmptyParts);
        if (epps.isEmpty()) {
            powEppCombo->clear();
            powEppCombo->setEnabled(false);
            powEppCombo->setToolTip(tr("Your CPU driver does not expose an energy/performance preference."));
        } else {
            powEppCombo->setEnabled(true);
            if (profileItems(powEppCombo) != epps) {
                powEppCombo->clear();
                powEppCombo->addItems(epps);
            }
            powEppCombo->setCurrentText(v.value("EPP"));
        }

        // --- battery ---
        sensBatteryPath = v.value("BATPATH");
        if (sensBatteryPath.isEmpty()) {
            powBatteryGroup->setVisible(false);
        } else {
            powBatteryGroup->setVisible(true);
            const double full = v.value("BATFULL").toDouble();
            const double design = v.value("BATDESIGN").toDouble();
            const int health = (design > 0) ? qRound(full / design * 100.0) : 0;
            const QString status = v.value("BATSTATUS");
            const double watts = v.value("BATPOWER").toDouble() / 1000000.0;

            auto row = [](const QString &k, const QString &val) {
                return QString("<tr><td style='color:#888; padding:2px 14px 2px 0;'>%1</td>"
                               "<td style='padding:2px 0;'><b>%2</b></td></tr>").arg(k, val.toHtmlEscaped());
            };
            QString html = "<table>";
            html += row(tr("Charge"), v.value("BATCAP") + " %  (" + status + ")");
            if (health > 0)
                html += row(tr("Health"), tr("%1 % of design capacity").arg(health));
            if (watts > 0.05)
                html += row(status == "Charging" ? tr("Charging at") : tr("Drawing"),
                            QString::number(watts, 'f', 1) + " W");
            if (!v.value("BATCYCLES").isEmpty() && v.value("BATCYCLES") != "0")
                html += row(tr("Cycles"), v.value("BATCYCLES"));
            html += row(tr("Power source"), v.value("AC") == "1" ? tr("AC adapter") : tr("Battery"));
            html += "</table>";
            powBatteryLabel->setText(html);

            const QString thr = v.value("BATTHRESH");
            if (thr.isEmpty()) {
                powThresholdSpin->setEnabled(false);
                powThresholdSpin->setToolTip(tr("This battery does not expose a charge limit "
                                                "(no charge_control_end_threshold in sysfs)."));
            } else {
                powThresholdSpin->setEnabled(true);
                powThresholdSpin->setValue(qBound(20, thr.toInt(), 100));
            }
        }

        // --- fancontrol ---
        if (v.value("FANCONTROLBIN") != "1") {
            fanStatusLabel->setText(tr("<span style='color:#888;'>fancontrol is not installed "
                                       "(package: <code>lm_sensors</code>). Your BIOS/EC is driving the fans.</span>"));
        } else {
            const QString state = v.value("FANCONTROLSTATE");
            const bool active = (state == "active");
            const bool haveCurve = v.value("FANCONFIG") == "1";
            fanStatusLabel->setText(QString("<b style='color:%1;'>%2</b> <span style='color:#888;'>· %3</span>")
                .arg(active ? "#27ae60" : "#888",
                     active ? tr("🟢 fancontrol running") : tr("⚪ fancontrol stopped"),
                     haveCurve ? tr("curve at /etc/fancontrol · %1 at boot")
                                    .arg(v.value("FANCONTROLENABLED") == "enabled" ? tr("enabled") : tr("disabled"))
                               : tr("no curve configured yet")));
        }

        sensPopulating = false;
    });

    proc->start("bash", QStringList() << "-c" << R"BASH(
if command -v powerprofilesctl >/dev/null 2>&1; then
  echo "PROFILE=$(powerprofilesctl get 2>/dev/null)"
  echo "PROFILES=$(powerprofilesctl list 2>/dev/null | grep -oE '^[ *]*[a-z-]+:' | tr -d ' *:' | tr '\n' ' ')"
fi
command -v tlp >/dev/null 2>&1 && echo "TLP=1"

C=/sys/devices/system/cpu/cpu0/cpufreq
[ -r "$C/scaling_governor" ] && echo "GOV=$(cat $C/scaling_governor)"
[ -r "$C/scaling_available_governors" ] && echo "GOVS=$(cat $C/scaling_available_governors)"
[ -r "$C/energy_performance_preference" ] && echo "EPP=$(cat $C/energy_performance_preference)"
[ -r "$C/energy_performance_available_preferences" ] && echo "EPPS=$(cat $C/energy_performance_available_preferences)"

for b in /sys/class/power_supply/*; do
  [ -r "$b/type" ] || continue
  [ "$(cat $b/type)" = "Battery" ] || continue
  echo "BATPATH=$b"
  [ -r "$b/capacity" ]    && echo "BATCAP=$(cat $b/capacity)"
  [ -r "$b/status" ]      && echo "BATSTATUS=$(cat $b/status)"
  [ -r "$b/cycle_count" ] && echo "BATCYCLES=$(cat $b/cycle_count)"
  if   [ -r "$b/energy_full" ]; then
    echo "BATFULL=$(cat $b/energy_full)"; echo "BATDESIGN=$(cat $b/energy_full_design 2>/dev/null)"
  elif [ -r "$b/charge_full" ]; then
    echo "BATFULL=$(cat $b/charge_full)"; echo "BATDESIGN=$(cat $b/charge_full_design 2>/dev/null)"
  fi
  if [ -r "$b/power_now" ]; then
    echo "BATPOWER=$(cat $b/power_now)"
  elif [ -r "$b/current_now" ] && [ -r "$b/voltage_now" ]; then
    echo "BATPOWER=$(awk "BEGIN{printf \"%d\", $(cat $b/current_now)*$(cat $b/voltage_now)/1000000}")"
  fi
  for t in charge_control_end_threshold charge_stop_threshold; do
    [ -r "$b/$t" ] && { echo "BATTHRESH=$(cat $b/$t)"; break; }
  done
  break
done
for a in /sys/class/power_supply/*; do
  [ -r "$a/type" ] || continue
  [ "$(cat $a/type)" = "Mains" ] || continue
  [ -r "$a/online" ] && echo "AC=$(cat $a/online)"
  break
done

command -v fancontrol >/dev/null 2>&1 && echo "FANCONTROLBIN=1" || echo "FANCONTROLBIN=0"
[ -s /etc/fancontrol ] && echo "FANCONFIG=1" || echo "FANCONFIG=0"
echo "FANCONTROLSTATE=$(systemctl is-active fancontrol 2>/dev/null)"
echo "FANCONTROLENABLED=$(systemctl is-enabled fancontrol 2>/dev/null)"
)BASH");
}
