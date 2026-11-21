#pragma once
#include <QtWidgets>
#include "ui_components/presentation.h"
#include "ui_components/panel_widgets.h"
#include "ui_components/network_page.h"
#include "ui_components/grub_editor.h"
#include "themepresets.h"

class CachyThemePreview final : public QAbstractButton {
    QString title;
    QColor accent, background, foreground;
public:
    CachyThemePreview(const ThemePreset &preset, QWidget *parent) : QAbstractButton(parent),
        title(QString::fromUtf8(preset.name)), accent(preset.accent), background(preset.background),
        foreground(QString(preset.text).isEmpty() ? (background.lightness() < 128 ? "#eef0f4" : "#20252c") : preset.text) {
        setCheckable(true); setCursor(Qt::PointingHandCursor); setAccessibleName(title);
        setToolTip(tr("Apply %1 throughout CachyOsTools").arg(title));
        setMinimumSize(180, 112); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    QSize sizeHint() const override { return QSize(250, 112); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        const QRectF bounds = QRectF(rect()).adjusted(2, 2, -2, -2);
        painter.setPen(QPen(isChecked() || hasFocus() ? accent : palette().color(QPalette::Mid), isChecked() || hasFocus() ? 2 : 1));
        painter.setBrush(background); painter.drawRoundedRect(bounds, 9, 9);
        const QRectF rail(bounds.left()+12, bounds.top()+12, 44, 48);
        painter.fillRect(rail, background.lightness() < 128 ? background.lighter(150) : background.darker(110));
        painter.fillRect(rail.adjusted(5, 7, -5, -31), accent);
        painter.setPen(QPen(foreground, 2));
        painter.drawLine(QPointF(rail.right()+12, rail.top()+6), QPointF(bounds.right()-16, rail.top()+6));
        painter.setOpacity(.35);
        for (int y : {23, 35, 47}) painter.drawLine(QPointF(rail.right()+12, rail.top()+y), QPointF(bounds.right()-36, rail.top()+y));
        painter.setOpacity(1); painter.setPen(foreground);
        auto textFont = font(); textFont.setBold(true); painter.setFont(textFont);
        painter.drawText(bounds.adjusted(12, 72, -12, -8), Qt::AlignLeft | Qt::AlignVCenter, title);
        if (isChecked()) { painter.setBrush(accent); painter.setPen(Qt::NoPen); painter.drawEllipse(QPointF(bounds.right()-18,bounds.bottom()-19),4,4); }
    }
};

void MainWindow::setupModernInterface() {
    // Keep the original tab objects, preferences and backend signal handlers.
    grubEditorTab = new GrubEditor(CachyUi::toolPath("grub_editor.py"), ui->tabWidget);
    ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->driveToolsTab) + 1, grubEditorTab, tr("GRUB Editor"));

    auto *network = new NetworkPage(this, ui->networkTab);
    ui->networkTab->layout()->removeWidget(ui->networkTabWidget);
    ui->networkTabWidget->hide(); ui->networkTab->layout()->addWidget(network);
    ui->networkTab->setProperty("cachyNetworkRedesigned", true);
    connect(network, &NetworkPage::refreshRequested, this, [this](const QString &section) {
        if (section == "Connections") {
            refreshNetworkInfo(); refreshInterfaceConfig(); refreshConnectionsList(); refreshWifiNetworks(false);
        } else if (section == "Virtual networks") {
            refreshBridges(); refreshLibvirtNetworks();
        } else if (section == "Diagnostics") {
            refreshNetworkInfo(); refreshInterfaceStats(); refreshOpenPorts();
        } else if (section == "Shared folders") refreshNetworkShares();
    });

    CachyUi::tweakCards(ui->tweaksTabWidget);
    auto *gallery = new QGroupBox(tr("Color presets"), ui->themeTab);
    gallery->setObjectName("themeGallery");
    auto *galleryLayout = new QGridLayout(gallery);
    auto *themeButtons = new QButtonGroup(gallery);
    for (int index=1; index<THEME_PRESET_COUNT-1; ++index) {
        auto *button = new CachyThemePreview(THEME_PRESETS[index], gallery);
        themeButtons->addButton(button, index);
        galleryLayout->addWidget(button, (index-1)/3, (index-1)%3);
        button->setChecked(ui->themePresetCombo->currentIndex() == index);
        connect(button, &QAbstractButton::clicked, this, [this, index] {
            ui->themePresetCombo->setCurrentIndex(index);
            emit ui->themePresetCombo->activated(index);
        });
    }
    connect(ui->themePresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), gallery, [themeButtons](int selected) {
        themeButtons->setExclusive(false);
        for (auto *button : themeButtons->buttons()) button->setChecked(themeButtons->id(button) == selected);
        themeButtons->setExclusive(true);
    });
    if (auto *layout = qobject_cast<QBoxLayout *>(ui->themeTab->layout())) layout->insertWidget(0, gallery);
    struct Page { QWidget *widget; QString title; QString description; };
    const QList<Page> pages{
        {ui->dashboardTab, tr("Your system, at a glance"), tr("Hardware, accounts, kernels and desktop windows.")},
        {ui->driveToolsTab, tr("Drives & storage"), tr("Physical drives and their partitions, grouped together. Select a volume to see its actions.")},
        {ui->shellToolsTab, tr("Shell & shortcuts"), tr("Your aliases, shell configuration and application shortcuts.")},
        {ui->servicesTab, tr("Services & startup"), tr("Inspect services and choose which applications start with your session.")},
        {ui->tweaksTab, tr("System tweaks"), tr("Find a setting, check its current value, and choose the action you need.")},
        {ui->kvmTab, tr("Virtual machines"), tr("GPU passthrough guides and your existing KVM setup tools.")},
        {ui->configTab, tr("System configuration"), tr("Open, inspect and edit configuration files with your existing backup and authentication controls.")},
        {ui->logsTab, tr("Logs & reports"), tr("Search system reports and follow the journal.")},
        {ui->backupTab, tr("System backup"), tr("Choose the source and destination before starting your backup.")},
        {ui->restoreTab, tr("System restore"), tr("Select a saved backup and review where it will be restored.")},
        {ui->isoCreatorTab, tr("System ISO Creator"), tr("Create a bootable copy of your system, with your chosen exclusions and installer options.")},
        {ui->packageManagerTab, tr("Install & update software"), tr("Repository packages, AUR helpers and updates in their familiar tabs.")},
        {ui->uninstallTab, tr("Installed software"), tr("Find installed packages, review their details and manage unused dependencies.")},
        {cleanerTab, tr("System cleaner"), tr("Measure space first, review cleanup choices, or explore the disk usage map.")},
        {ui->themeTab, tr("Make it yours"), tr("Choose a CachyOsTools palette or adjust your own colors. Changes apply throughout the app.")}
    };
    for (const auto &page : pages) CachyUi::pageHeading(page.widget, page.title, page.description);
    CachyUi::polishViews(this);
    applyAppTheme();
}
