
// Theme engine: application-wide XETAL card look, driven by three colors —
// accent, background, text. Empty background/text = inherit the system theme
// (the original look). All choices persist in QSettings and apply instantly.

#include <QColorDialog>
#include <functional>

struct ThemePreset {
    const char *name;
    const char *accent;
    const char *background; // empty = system palette
    const char *text;       // empty = auto from background
};

static const ThemePreset THEME_PRESETS[] = {
    {"System Default (XETAL Green)", "#27ae60", "",        ""},
    {"XETAL Dark",                   "#27ae60", "#1d1f24", ""},
    {"XETAL Red",                    "#c0392b", "#201a1a", ""},
    {"Midnight Blue",                "#3498db", "#0f1626", ""},
    {"Matrix",                       "#00cc44", "#0a0f0a", "#9dffb0"},
    {"Solar Light",                  "#e67e22", "#f5f1e8", ""},
    {"Purple Haze",                  "#9b59b6", "#191221", ""},
    {"Custom",                       "",        "",        ""},
};
static const int THEME_PRESET_COUNT = sizeof(THEME_PRESETS) / sizeof(THEME_PRESETS[0]);

void MainWindow::applyAppTheme() {
    QColor accent(themeAccent.isEmpty() ? "#27ae60" : themeAccent);
    QString aHex = accent.name();
    auto aRgba = [&accent](double alpha) {
        return QString("rgba(%1,%2,%3,%4)").arg(accent.red()).arg(accent.green()).arg(accent.blue()).arg(alpha);
    };

    QString style = QString(
        // --- Cards: every group box gets the accent stripe ---
        "QGroupBox {"
        "  background: rgba(127,127,127,0.07);"
        "  border: 1px solid rgba(127,127,127,0.25);"
        "  border-top: 3px solid %1;"
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
        "QPushButton:hover { background: %2; border-color: %1; }"
        "QPushButton:pressed { background: %3; }"
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
        "QTabBar::tab:selected { background: %4; border-top: 2px solid %1; font-weight: bold; }"
        "QTabBar::tab:hover:!selected { background: rgba(127,127,127,0.18); }"

        // --- Tables, trees, lists ---
        "QTableWidget, QTreeWidget, QListWidget {"
        "  border: 1px solid rgba(127,127,127,0.25);"
        "  border-radius: 8px;"
        "  gridline-color: rgba(127,127,127,0.12);"
        "  alternate-background-color: rgba(127,127,127,0.06);"
        "  selection-background-color: %3;"
        "}"
        "QHeaderView::section {"
        "  background: rgba(127,127,127,0.12);"
        "  border: none;"
        "  border-bottom: 2px solid %1;"
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
        "  border: 1px solid %1;"
        "}"

        // --- Progress bars ---
        "QProgressBar {"
        "  border: 1px solid rgba(127,127,127,0.3);"
        "  border-radius: 6px;"
        "  text-align: center;"
        "  background: rgba(127,127,127,0.08);"
        "}"
        "QProgressBar::chunk { background: %1; border-radius: 5px; }"

        // --- Splitters ---
        "QSplitter::handle { background: rgba(127,127,127,0.2); }"
        "QSplitter::handle:hover { background: %1; }")
        .arg(aHex, aRgba(0.25), aRgba(0.40), aRgba(0.18));

    // Optional full background/text override (presets beyond "System Default")
    if (!themeBg.isEmpty()) {
        QColor bg(themeBg);
        bool dark = bg.lightness() < 128;
        QColor base = dark ? bg.lighter(135) : bg.darker(104);        // inputs/views
        QColor text = themeText.isEmpty() ? (dark ? QColor("#e6e6e6") : QColor("#1c1c1c"))
                                          : QColor(themeText);
        style += QString(
            "QMainWindow, QDialog { background: %1; }"
            "QWidget { color: %2; }"
            "QTabWidget::pane { background: %1; }"
            "QMenuBar { background: %1; color: %2; }"
            "QMenuBar::item:selected { background: %3; }"
            "QMenu { background: %4; color: %2; border: 1px solid rgba(127,127,127,0.3); }"
            "QMenu::item:selected { background: %3; }"
            "QTableWidget, QTreeWidget, QListWidget, QTextEdit, QPlainTextEdit,"
            "QLineEdit, QComboBox, QSpinBox { background: %4; color: %2; }"
            "QHeaderView::section { color: %2; }"
            "QStatusBar { background: %1; color: %2; }")
            .arg(bg.name(), text.name(), aRgba(0.35), base.name());
    }

    setStyleSheet(style);
}

void MainWindow::loadThemeSettings() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    themeAccent = settings.value("theme/accent", "#27ae60").toString();
    themeBg = settings.value("theme/background", "").toString();
    themeText = settings.value("theme/text", "").toString();
    int preset = settings.value("theme/preset", 0).toInt();
    if (preset >= 0 && preset < THEME_PRESET_COUNT) {
        ui->themePresetCombo->setCurrentIndex(preset);
    }
    updateThemeSwatches();
}

void MainWindow::saveThemeSettings() {
    QSettings settings("CachyOsTools", "CachyOsTools");
    settings.setValue("theme/accent", themeAccent);
    settings.setValue("theme/background", themeBg);
    settings.setValue("theme/text", themeText);
    settings.setValue("theme/preset", ui->themePresetCombo->currentIndex());
}

// Paints the picker buttons as swatches of their current color
void MainWindow::updateThemeSwatches() {
    auto swatch = [](QPushButton *button, const QString &color, const QString &emptyLabel) {
        if (color.isEmpty()) {
            button->setText(emptyLabel);
            button->setStyleSheet("");
        } else {
            QColor c(color);
            QString textColor = (c.lightness() < 128) ? "white" : "black";
            button->setText(color);
            button->setStyleSheet(QString("background: %1; color: %2; border: 1px solid rgba(127,127,127,0.5);"
                                          "border-radius: 6px; padding: 5px 10px; font-weight: bold;")
                                  .arg(color, textColor));
        }
    };
    swatch(ui->themeAccentButton, themeAccent, "Pick...");
    swatch(ui->themeBgButton, themeBg, "System (default)");
    swatch(ui->themeTextButton, themeText, "Auto");
}

void MainWindow::on_themePresetCombo_activated(int index) {
    if (index < 0 || index >= THEME_PRESET_COUNT) return;
    const ThemePreset &preset = THEME_PRESETS[index];
    if (QString(preset.name) != "Custom") {
        themeAccent = preset.accent;
        themeBg = preset.background;
        themeText = preset.text;
    }
    saveThemeSettings();
    updateThemeSwatches();
    applyAppTheme();
    refreshDashboard();
}

// Custom pickers: changing any color switches the preset to "Custom" and applies live
static void xetalPickColor(MainWindow *w, QString &target, const QString &fallback, const QString &title,
                           std::function<void()> after) {
    QColor initial(target.isEmpty() ? fallback : target);
    QColor picked = QColorDialog::getColor(initial, w, title);
    if (!picked.isValid()) return;
    target = picked.name();
    after();
}

void MainWindow::on_themeAccentButton_clicked() {
    xetalPickColor(this, themeAccent, "#27ae60", "Pick Accent Color", [this]() {
        ui->themePresetCombo->setCurrentIndex(THEME_PRESET_COUNT - 1); // Custom
        saveThemeSettings(); updateThemeSwatches(); applyAppTheme(); refreshDashboard();
    });
}

void MainWindow::on_themeBgButton_clicked() {
    xetalPickColor(this, themeBg, "#1d1f24", "Pick Background Color", [this]() {
        ui->themePresetCombo->setCurrentIndex(THEME_PRESET_COUNT - 1);
        saveThemeSettings(); updateThemeSwatches(); applyAppTheme(); refreshDashboard();
    });
}

void MainWindow::on_themeTextButton_clicked() {
    xetalPickColor(this, themeText, "#e6e6e6", "Pick Text Color", [this]() {
        ui->themePresetCombo->setCurrentIndex(THEME_PRESET_COUNT - 1);
        saveThemeSettings(); updateThemeSwatches(); applyAppTheme(); refreshDashboard();
    });
}

void MainWindow::on_themeResetButton_clicked() {
    themeAccent = "#27ae60";
    themeBg = "";
    themeText = "";
    ui->themePresetCombo->setCurrentIndex(0);
    saveThemeSettings();
    updateThemeSwatches();
    applyAppTheme();
    refreshDashboard();
}
