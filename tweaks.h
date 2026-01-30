#include <QComboBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

void MainWindow::showTweakInstructions(const QString &title, const QString &instructions)
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(title);
    dialog->setModal(false);
    dialog->resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    
    QTextEdit *textEdit = new QTextEdit(dialog);
    textEdit->setPlainText(instructions);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Monospace", 10));
    layout->addWidget(textEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *copyButton = new QPushButton("Copy to Clipboard", dialog);
    QPushButton *closeButton = new QPushButton("Close", dialog);
    
    connect(copyButton, &QPushButton::clicked, [textEdit]() {
        QApplication::clipboard()->setText(textEdit->toPlainText());
    });
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    dialog->show();
}


// Tweaks tab slots
void MainWindow::on_zramToggle_clicked() {
    QString instructions = R"(
# ZRAM Configuration Instructions
# ==============================
# 
# ZRAM creates compressed swap space in RAM, which can improve performance on systems with limited RAM.
# 
# To ENABLE ZRAM:
# Click the "Edit Config" button to create/modify the config file
# Copy ONLY the following content to /etc/systemd/zram-generator.conf:

[zram0]
zram-size = ram / 2

# Then run these commands:
# sudo systemctl daemon-reload
# sudo systemctl enable systemd-zram-setup@zram0.service
# sudo systemctl start systemd-zram-setup@zram0.service
# 
# To DISABLE ZRAM:
# sudo systemctl stop systemd-zram-setup@zram0.service
# sudo systemctl disable systemd-zram-setup@zram0.service
# sudo rm -f /etc/systemd/zram-generator.conf
# 
# To check if ZRAM is active:
# cat /proc/swaps
# lsblk | grep zram
)";
    
    showTweakInstructions("ZRAM Configuration", instructions);
}
void MainWindow::on_zramConfigButton_clicked() {
    // Check if zram config exists and show current state
    QFile zramConfig("/etc/systemd/zram-generator.conf");
    QString message = zramConfig.exists() ? "Opening existing ZRAM config file for editing." : "Creating new ZRAM config file for editing.";
    openConfigInNano("/etc/systemd/zram-generator.conf");
}
void MainWindow::on_cpuGovernorToggle_clicked() {
    QString instructions = R"(
# CPU Governor Configuration Instructions
# ===================================
# 
# CPU governors control how the CPU scales its frequency based on load.
# 
# To set PERFORMANCE governor (maximum performance, higher power consumption):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/default/cpupower:

GOVERNOR="performance"

# Then run this command:
# sudo systemctl restart cpupower
# 
# To set POWERSAVE governor (lower performance, better power efficiency):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/default/cpupower:

GOVERNOR="powersave"

# Then run this command:
# sudo systemctl restart cpupower
# 
# To check current governor:
# cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
# 
# Available governors on your system:
# cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
# 
# Note: Some systems may have additional governors like 'schedutil', 'ondemand', etc.
)";
    
    showTweakInstructions("CPU Governor Configuration", instructions);
}
void MainWindow::on_cpuGovernorConfigButton_clicked() {
    // Check current governor and show state
    QProcess proc;
    proc.start("cat", QStringList() << "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    proc.waitForFinished();
    QString currentGovernor = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    openConfigInNano("/etc/default/cpupower");
}
void MainWindow::on_ipv6Toggle_clicked() {
    QString instructions = R"(
# IPv6 Configuration Instructions
# ==============================
# 
# IPv6 can be enabled or disabled system-wide. Some applications may require IPv6 to function properly.
# 
# To ENABLE IPv6:
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/40-ipv6.conf:

net.ipv6.conf.all.disable_ipv6 = 0
net.ipv6.conf.default.disable_ipv6 = 0

# Then run this command:
# sudo sysctl -p /etc/sysctl.d/40-ipv6.conf
# 
# To DISABLE IPv6:
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/40-ipv6.conf:

net.ipv6.conf.all.disable_ipv6 = 1
net.ipv6.conf.default.disable_ipv6 = 1

# Then run this command:
# sudo sysctl -p /etc/sysctl.d/40-ipv6.conf
# 
# To check current IPv6 status:
# sysctl net.ipv6.conf.all.disable_ipv6
# sysctl net.ipv6.conf.default.disable_ipv6
)";
    
    showTweakInstructions("IPv6 Configuration", instructions);
}
void MainWindow::on_ipv6ConfigButton_clicked() {
    // Check current IPv6 state
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "net.ipv6.conf.all.disable_ipv6");
    proc.waitForFinished();
    QString ipv6Disabled = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status = (ipv6Disabled == "1") ? "disabled" : "enabled";
    openConfigInNano("/etc/sysctl.d/40-ipv6.conf");
}
void MainWindow::on_trimToggle_clicked() {
    QString instructions = R"(
# TRIM Configuration Instructions
# ==============================
# 
# TRIM helps maintain SSD performance by informing the drive which blocks are no longer in use.
# 
# To ENABLE TRIM:
# sudo systemctl enable fstrim.timer
# sudo systemctl start fstrim.timer
# 
# To DISABLE TRIM:
# sudo systemctl stop fstrim.timer
# sudo systemctl disable fstrim.timer
# 
# To manually run TRIM once:
# sudo fstrim -v /
# 
# To check if TRIM is supported by your SSD:
# sudo fstrim -v /
)";
    
    showTweakInstructions("TRIM Configuration", instructions);
}
void MainWindow::on_trimConfigButton_clicked() {
    // TRIM doesn't use a config file - it's managed via systemd timer
    QMessageBox::information(this, "TRIM Configuration", 
        "TRIM is managed via systemd timer, not a config file.\n\n"
        "To enable/disable TRIM, use the 'Apply' button or run:\n"
        "sudo systemctl enable fstrim.timer\n"
        "sudo systemctl start fstrim.timer\n\n"
        "TRIM can also be enabled via mount options in /etc/fstab\n"
        "by adding 'discard' option, but this is not recommended\n"
        "as it can impact performance.");
}
void MainWindow::on_tmpfsToggle_clicked() {
    QString instructions = R"(
# tmpfs Configuration Instructions
# ===============================
# 
# Mounting /tmp as tmpfs (RAM disk) can improve performance by keeping temporary files in RAM.
# 
# To ENABLE tmpfs for /tmp:
# Click the "Edit Config" button to create/modify the config file
# Add this line to /etc/fstab:

/tmp tmpfs tmpfs defaults,noatime,mode=1777 0 0

# Then reboot the system for changes to take effect:
# sudo reboot
# 
# To DISABLE tmpfs for /tmp:
# Click the "Edit Config" button to create/modify the config file
# Remove the tmpfs line from /etc/fstab
# 
# Then reboot the system for changes to take effect:
# sudo reboot
# 
# To check current /tmp mount status:
# findmnt /tmp
# mount | grep tmp
# 
# To temporarily mount /tmp as tmpfs (without editing fstab):
# sudo mount -t tmpfs -o defaults,noatime,mode=1777 tmpfs /tmp
)";
    
    showTweakInstructions("tmpfs Configuration", instructions);
}
void MainWindow::on_tmpfsConfigButton_clicked() {
    // Check if /tmp is mounted as tmpfs
    QProcess proc;
    proc.start("findmnt", QStringList() << "/tmp" << "-o" << "FSTYPE" << "--noheadings");
    proc.waitForFinished();
    QString fsType = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status = (fsType == "tmpfs") ? "mounted as tmpfs" : "not mounted as tmpfs";
    openConfigInNano("/etc/fstab");
}
void MainWindow::on_dnsToggle_clicked() {
    QString instructions = R"(
# DNS Configuration Instructions
# ============================
# 
# DNS (Domain Name System) configuration controls how your system resolves domain names to IP addresses.
# 
# To configure DNS servers manually:
# Click the "Edit Config" button to create/modify the config file
# Add or modify nameserver entries in /etc/resolv.conf:

nameserver 8.8.8.8
nameserver 8.8.4.4
nameserver 1.1.1.1

# Common DNS servers:
# - Google DNS: 8.8.8.8, 8.8.4.4
# - Cloudflare: 1.1.1.1, 1.0.0.1
# - OpenDNS: 208.67.222.222, 208.67.220.220
# - Quad9: 9.9.9.9, 149.112.112.112
# 
# To use your ISP's DNS servers (automatic):
# Remove or comment out all nameserver lines
# 
# To check current DNS configuration:
# cat /etc/resolv.conf
# 
# To test DNS resolution:
# nslookup google.com
# dig google.com
# 
# Note: Some systems use systemd-resolved or NetworkManager which may override /etc/resolv.conf
# Check if systemd-resolved is active: systemctl is-active systemd-resolved
)";
    
    showTweakInstructions("DNS Configuration", instructions);
}

void MainWindow::on_dnsConfigButton_clicked() {
    openConfigInNano("/etc/resolv.conf");
}
void MainWindow::on_showHiddenFilesToggle_clicked() {
    QString instructions = R"(
# Hidden Files Configuration Instructions
# =====================================
# 
# Configure file managers to show hidden files by default.
# 
# For KDE Dolphin:
# Click the "Edit Config" button to create/modify the config file
# Add or modify the [General] section in ~/.config/dolphinrc:

[General]
ShowHiddenFiles=true

# To hide hidden files by default, set ShowHiddenFiles=false
# 
# For GNOME Files (Nautilus):
# Open GNOME Files
# Press Ctrl+H to toggle hidden files
# Go to Edit > Preferences > Views
# Check "Show hidden and backup files"
# 
# For other file managers:
# - Thunar (Xfce): View > Show Hidden Files
# - PCManFM: View > Show Hidden
# - Ranger: Press 'zh' to toggle hidden files
# 
# To show all hidden files in terminal:
# ls -la
)";
    
    showTweakInstructions("Hidden Files Configuration", instructions);
}
void MainWindow::on_showHiddenFilesConfigButton_clicked() {
    // Check current Dolphin settings
    QString path = QDir::homePath() + "/.config/dolphinrc";
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("General");
    bool showHidden = settings.value("ShowHiddenFiles", false).toBool();
    settings.endGroup();
    QString status = showHidden ? "enabled" : "disabled";
    openConfigInNano(QDir::homePath() + "/.config/dolphinrc");
}
void MainWindow::on_mitigationsToggle_clicked() {
    QString instructions = R"(
# Security Mitigations Configuration Instructions
# ============================================
# 
# Spectre/Meltdown mitigations protect against CPU vulnerabilities but may impact performance.
# 
# To ENABLE mitigations (recommended for security):
# Click the "Edit Config" button to create/modify the config file
# Remove 'mitigations=off' from GRUB_CMDLINE_LINUX if present:
# GRUB_CMDLINE_LINUX="... other options ..."
# 
# Then update GRUB configuration:
# sudo grub-mkconfig -o /boot/grub/grub.cfg
# 
# Reboot for changes to take effect:
# sudo reboot
# 
# To DISABLE mitigations (for performance, not recommended):
# Click the "Edit Config" button to create/modify the config file
# Add 'mitigations=off' to GRUB_CMDLINE_LINUX:
# GRUB_CMDLINE_LINUX="... other options ... mitigations=off"
# 
# Then update GRUB configuration:
# sudo grub-mkconfig -o /boot/grub/grub.cfg
# 
# Reboot for changes to take effect:
# sudo reboot
# 
# To check current mitigation status:
# cat /sys/devices/system/cpu/vulnerabilities/*
)";
    
    showTweakInstructions("Security Mitigations Configuration", instructions);
}

void MainWindow::on_performanceHacksToggle_clicked() {
    QString instructions = R"(
# Performance Hacks Configuration Instructions
# ========================================
# 
# These performance optimizations are contributed by forAUR from CachyOS forums.
# 
# 1. Making locate locating again:
# sudo pacman -S plocate
# 
# Click the "Edit Config" button to edit /etc/updatedb.conf:
PRUNE_BIND_MOUNTS = "no"
PRUNEFS = ""
PRUNENAMES = ".git .hg .svn"
PRUNEPATHS = ""

# 2. Gas, gas, gas! (Set performance mode):
# sudo powerprofilesctl set performance

# 3. Remove plymouth from boot (faster boot):
# Click the "Edit Config" button to edit /etc/mkinitcpio.conf
# Remove 'plymouth' from the HOOKS line

# Then run these commands:
# sudo pacman -Rns cachyos-plymouth-bootanimation plymouth-kcm plymouth
# sudo mkinitcpio -P

# Credits: forAUR from CachyOS forums
)";
    
    showTweakInstructions("Performance Hacks Configuration", instructions);
}

void MainWindow::on_performanceHacksConfigButton1_clicked() {
    // Open the updatedb.conf config file for plocate configuration
    openConfigInNano("/etc/updatedb.conf");
}

void MainWindow::on_performanceHacksConfigButton2_clicked() {
    // Open the mkinitcpio.conf config file for removing plymouth
    openConfigInNano("/etc/mkinitcpio.conf");
}
void MainWindow::on_mitigationsConfigButton_clicked() {
    // Check current mitigations state
    QProcess proc;
    proc.start("grep", QStringList() << "-q" << "mitigations=off" << "/etc/default/grub");
    QString status = (proc.exitCode() == 0) ? "disabled" : "enabled";
    openConfigInNano("/etc/default/grub");
}

void MainWindow::on_ptraceToggle_clicked() {
    QString instructions = R"(
# ptrace Configuration Instructions
# ===============================
# 
# ptrace is a system call used by debuggers and some security tools to trace processes.
# By default, non-root users cannot use ptrace on processes they don't own.
# 
# To ENABLE ptrace for non-root users (allows debugging and security tools):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/10-ptrace.conf:

kernel.yama.ptrace_scope = 0

# Then run this command to apply the changes:
# sudo sysctl -p /etc/sysctl.d/10-ptrace.conf
# 
# To DISABLE ptrace for non-root users (more secure, prevents debugging):
# Click the "Edit Config" button to create/modify the config file
# Copy this content to /etc/sysctl.d/10-ptrace.conf:

kernel.yama.ptrace_scope = 1

# Then run this command to apply the changes:
# sudo sysctl -p /etc/sysctl.d/10-ptrace.conf
# 
# ptrace_scope values:
# 0 - No restrictions (any process can ptrace any other process)
# 1 - Restricted ptrace (only parent processes can ptrace their children)
# 2 - Admin-only ptrace (only root can use ptrace)
# 3 - No ptrace (ptrace is completely disabled)
# 
# To check current ptrace scope:
# sysctl kernel.yama.ptrace_scope
# 
# Note: Some debugging tools, security scanners, and development tools require ptrace access.
# Disabling ptrace can break these tools but improves security.
)";
    
    showTweakInstructions("ptrace Configuration", instructions);
}

void MainWindow::on_ptraceConfigButton_clicked() {
    // Check current ptrace scope
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "kernel.yama.ptrace_scope");
    proc.waitForFinished();
    QString currentScope = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    QString status;
    switch (currentScope.toInt()) {
        case 0: status = "unrestricted"; break;
        case 1: status = "restricted"; break;
        case 2: status = "admin-only"; break;
        case 3: status = "disabled"; break;
        default: status = "unknown"; break;
    }
    openConfigInNano("/etc/sysctl.d/10-ptrace.conf");
}

void MainWindow::on_hidepidToggle_clicked() {
    QString instructions = R"(
# hidepid Configuration Instructions
# ================================
# 
# hidepid is a mount option for /proc that controls process visibility.
# It prevents users from seeing processes they don't own, improving security.
# 
# To ENABLE hidepid (more secure, hides other users' processes):
# Click the "Edit Config" button to create/modify the config file
# Find the /proc line in /etc/fstab and add hidepid=2:
# 
# proc /proc proc defaults,hidepid=2 0 0
# 
# Then remount /proc to apply changes:
# sudo mount -o remount /proc
# 
# To DISABLE hidepid (less secure, shows all processes):
# Click the "Edit Config" button to create/modify the config file
# Find the /proc line in /etc/fstab and remove hidepid=2:
# 
# proc /proc proc defaults 0 0
# 
# Then remount /proc to apply changes:
# sudo mount -o remount /proc
# 
# hidepid values:
# 0 - No hiding (default, all processes visible)
# 1 - Hide processes from other users (processes not owned by current user)
# 2 - Hide processes from other users and hide /proc/PID directories
# 
# To check current hidepid setting:
# mount | grep proc
# 
# To test if hidepid is working:
# ls /proc | grep -E '^[0-9]+$' | head -5
# 
# Note: Some system monitoring tools and debugging utilities may not work
# properly with hidepid enabled. This is a security vs functionality trade-off.
# 
# Security benefits:
# - Prevents process enumeration attacks
# - Hides sensitive process information
# - Reduces information leakage
)";
    
    showTweakInstructions("hidepid Configuration", instructions);
}

void MainWindow::on_hidepidConfigButton_clicked() {
    // Check current hidepid setting
    QProcess proc;
    proc.start("mount", QStringList());
    proc.waitForFinished();
    QString mountOutput = QString::fromUtf8(proc.readAllStandardOutput());
    QString status = "not set";
    
    if (mountOutput.contains("hidepid=1")) {
        status = "hidepid=1 (hide other users' processes)";
    } else if (mountOutput.contains("hidepid=2")) {
        status = "hidepid=2 (hide other users' processes and directories)";
    } else if (mountOutput.contains("proc")) {
        status = "hidepid=0 (no hiding, all processes visible)";
    }
    
    openConfigInNano("/etc/fstab");
}

// Tweaks tab status checking and apply functions
void MainWindow::on_tweaksTabWidget_currentChanged(int index) {
    Q_UNUSED(index)
    refreshTweaksStatus();
}

void MainWindow::refreshTweaksStatus() {
    checkZramState();
    checkCpuGovernorState();
    checkIpv6State();
    checkTrimState();
    checkTmpfsState();
    checkDnsState();
    checkShowHiddenFilesState();
    checkMitigationsState();
    checkPtraceState();
    checkHidepidState();
    checkPerformanceHacksState();
    checkSwappinessState();
    checkIoSchedulerState();
    checkThpState();
    checkTcpOptimizationsState();
    checkFirewallState();
    checkPacmanOptimizationsState();
    checkJournaldState();
}

void MainWindow::updateTweakStatusLabel(QLabel *label, const QString &status, bool enabled) {
    if (!label) return;
    label->setText(status);
    if (enabled) {
        label->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 4px; border-radius: 3px;");
    } else {
        label->setStyleSheet("background-color: #f44336; color: white; font-weight: bold; padding: 4px; border-radius: 3px;");
    }
}

void MainWindow::checkZramState() {
    QProcess proc;
    proc.start("systemctl", QStringList() << "is-active" << "systemd-zram-setup@zram0.service");
    proc.waitForFinished();
    bool enabled = (proc.exitCode() == 0);
    updateTweakStatusLabel(ui->zramStatusLabel, enabled ? "Enabled" : "Disabled", enabled);
}

void MainWindow::checkCpuGovernorState() {
    QProcess proc;
    proc.start("cat", QStringList() << "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    proc.waitForFinished();
    QString governor = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    bool isPerformance = (governor == "performance");
    updateTweakStatusLabel(ui->cpuGovernorStatusLabel, governor.isEmpty() ? "Unknown" : governor, isPerformance);
}

void MainWindow::checkIpv6State() {
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "net.ipv6.conf.all.disable_ipv6");
    proc.waitForFinished();
    QString disabled = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    bool enabled = (disabled != "1");
    updateTweakStatusLabel(ui->ipv6StatusLabel, enabled ? "Enabled" : "Disabled", enabled);
}

void MainWindow::checkTrimState() {
    QProcess proc;
    proc.start("systemctl", QStringList() << "is-enabled" << "fstrim.timer");
    proc.waitForFinished();
    bool enabled = (proc.exitCode() == 0);
    updateTweakStatusLabel(ui->trimStatusLabel, enabled ? "Enabled" : "Disabled", enabled);
}

void MainWindow::checkTmpfsState() {
    QProcess proc;
    proc.start("findmnt", QStringList() << "/tmp" << "-o" << "FSTYPE" << "--noheadings");
    proc.waitForFinished();
    QString fsType = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    bool enabled = (fsType == "tmpfs");
    updateTweakStatusLabel(ui->tmpfsStatusLabel, enabled ? "Enabled" : "Disabled", enabled);
}

void MainWindow::checkDnsState() {
    QFile resolv("/etc/resolv.conf");
    bool hasNameservers = false;
    if (resolv.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&resolv);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("nameserver") && !line.startsWith("#")) {
                hasNameservers = true;
                break;
            }
        }
        resolv.close();
    }
    updateTweakStatusLabel(ui->dnsStatusLabel, hasNameservers ? "Configured" : "Auto", hasNameservers);
}

void MainWindow::checkShowHiddenFilesState() {
    QString path = QDir::homePath() + "/.config/dolphinrc";
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("General");
    bool enabled = settings.value("ShowHiddenFiles", false).toBool();
    settings.endGroup();
    updateTweakStatusLabel(ui->showHiddenFilesStatusLabel, enabled ? "Enabled" : "Disabled", enabled);
}

void MainWindow::checkMitigationsState() {
    QProcess proc;
    proc.start("grep", QStringList() << "-q" << "mitigations=off" << "/etc/default/grub");
    proc.waitForFinished();
    bool disabled = (proc.exitCode() == 0);
    updateTweakStatusLabel(ui->mitigationsStatusLabel, disabled ? "Disabled" : "Enabled", !disabled);
}

void MainWindow::checkPtraceState() {
    QProcess proc;
    proc.start("sysctl", QStringList() << "-n" << "kernel.yama.ptrace_scope");
    proc.waitForFinished();
    QString scope = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    bool enabled = (scope == "0");
    QString status;
    if (scope == "0") status = "Unrestricted";
    else if (scope == "1") status = "Restricted";
    else if (scope == "2") status = "Admin-only";
    else if (scope == "3") status = "Disabled";
    else status = "Unknown";
    updateTweakStatusLabel(ui->ptraceStatusLabel, status, enabled);
}

void MainWindow::checkHidepidState() {
    QProcess proc;
    proc.start("mount", QStringList());
    proc.waitForFinished();
    QString mountOutput = QString::fromUtf8(proc.readAllStandardOutput());
    bool enabled = mountOutput.contains("hidepid=1") || mountOutput.contains("hidepid=2");
    QString status = "Disabled";
    if (mountOutput.contains("hidepid=2")) status = "Level 2";
    else if (mountOutput.contains("hidepid=1")) status = "Level 1";
    updateTweakStatusLabel(ui->hidepidStatusLabel, status, enabled);
}

void MainWindow::checkPerformanceHacksState() {
    // Check if plocate is installed and updatedb.conf is configured
    QProcess proc;
    proc.start("pacman", QStringList() << "-Q" << "plocate");
    proc.waitForFinished();
    bool plocateInstalled = (proc.exitCode() == 0);
    
    // Check if updatedb.conf has the performance settings
    QFile updatedb("/etc/updatedb.conf");
    bool updatedbConfigured = false;
    if (updatedb.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&updatedb);
        QString content = in.readAll();
        updatedb.close();
        updatedbConfigured = content.contains("PRUNE_BIND_MOUNTS = \"no\"") && 
                            content.contains("PRUNEFS = \"\"");
    }
    
    // Check if plymouth is removed from mkinitcpio
    QFile mkinitcpio("/etc/mkinitcpio.conf");
    bool plymouthRemoved = false;
    if (mkinitcpio.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&mkinitcpio);
        QString content = in.readAll();
        mkinitcpio.close();
        // Check if HOOKS line doesn't contain plymouth
        QRegularExpression hooksRe("^HOOKS=.*");
        QRegularExpressionMatchIterator i = hooksRe.globalMatch(content);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString hooksLine = match.captured(0);
            if (!hooksLine.contains("plymouth")) {
                plymouthRemoved = true;
                break;
            }
        }
    }
    
    bool enabled = plocateInstalled && updatedbConfigured && plymouthRemoved;
    QString status = "Partial";
    if (enabled) status = "Enabled";
    else if (!plocateInstalled && !updatedbConfigured && !plymouthRemoved) status = "Disabled";
    else status = "Partial";
    
    updateTweakStatusLabel(ui->performanceHacksStatusLabel, status, enabled);
}

// Apply functions
void MainWindow::on_zramApplyButton_clicked() {
    int ret = QMessageBox::question(this, "Apply ZRAM", 
        "This will enable ZRAM (compressed RAM swap).\n\n"
        "A config file will be created at /etc/systemd/zram-generator.conf\n"
        "and the service will be started.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QString cmd = "sudo bash -c 'echo \"[zram0]\" > /etc/systemd/zram-generator.conf && "
                  "echo \"zram-size = ram / 2\" >> /etc/systemd/zram-generator.conf && "
                  "systemctl daemon-reload && "
                  "systemctl enable systemd-zram-setup@zram0.service && "
                  "systemctl start systemd-zram-setup@zram0.service'";
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'ZRAM enabled successfully!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

void MainWindow::on_cpuGovernorApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("CPU Governor");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Select CPU Governor:", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItems({"performance", "powersave", "ondemand", "conservative", "schedutil"});
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    QString governor = combo->currentText();
    QString cmd = QString("sudo bash -c 'echo \"GOVERNOR=\\\"%1\\\"\" > /etc/default/cpupower && systemctl restart cpupower'").arg(governor);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'CPU Governor set to " + governor + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

void MainWindow::on_ipv6ApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("IPv6 Configuration");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Enable or disable IPv6?", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItems({"Enable", "Disable"});
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    bool enable = (combo->currentText() == "Enable");
    QString value = enable ? "0" : "1";
    QString cmd = QString("sudo bash -c 'echo \"net.ipv6.conf.all.disable_ipv6 = %1\" > /etc/sysctl.d/40-ipv6.conf && "
                          "echo \"net.ipv6.conf.default.disable_ipv6 = %1\" >> /etc/sysctl.d/40-ipv6.conf && "
                          "sysctl -p /etc/sysctl.d/40-ipv6.conf'").arg(value);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'IPv6 " + (enable ? "enabled" : "disabled") + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

void MainWindow::on_trimApplyButton_clicked() {
    int ret = QMessageBox::question(this, "Apply TRIM", 
        "This will enable automatic TRIM for SSDs.\n\n"
        "The fstrim.timer service will be enabled and started.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QString cmd = "sudo systemctl enable fstrim.timer && sudo systemctl start fstrim.timer";
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'TRIM enabled successfully!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

void MainWindow::on_tmpfsApplyButton_clicked() {
    int ret = QMessageBox::warning(this, "Apply tmpfs", 
        "⚠️ WARNING: This will mount /tmp as tmpfs (RAM disk).\n\n"
        "This requires editing /etc/fstab and a REBOOT.\n\n"
        "All files in /tmp will be lost on reboot.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QMessageBox::information(this, "Manual Step Required", 
        "The tmpfs configuration will be added to /etc/fstab.\n\n"
        "You will need to REBOOT for this to take effect.\n\n"
        "Opening /etc/fstab for editing...");
    
    openConfigInNano("/etc/fstab");
    QMessageBox::information(this, "Next Steps", 
        "Add this line to /etc/fstab:\n\n"
        "/tmp tmpfs tmpfs defaults,noatime,mode=1777 0 0\n\n"
        "Then save and REBOOT.");
}

void MainWindow::on_dnsApplyButton_clicked() {
    QMessageBox::information(this, "DNS Configuration", 
        "DNS configuration requires manual editing.\n\n"
        "Opening /etc/resolv.conf for editing...\n\n"
        "Add nameserver lines like:\n"
        "nameserver 8.8.8.8\n"
        "nameserver 8.8.4.4");
    openConfigInNano("/etc/resolv.conf");
}

void MainWindow::on_showHiddenFilesApplyButton_clicked() {
    QString path = QDir::homePath() + "/.config/dolphinrc";
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup("General");
    settings.setValue("ShowHiddenFiles", true);
    settings.endGroup();
    QMessageBox::information(this, "Applied", "Hidden files will now be shown by default in Dolphin.\n\nYou may need to restart Dolphin for changes to take effect.");
    refreshTweaksStatus();
}

void MainWindow::on_mitigationsApplyButton_clicked() {
    int ret = QMessageBox::warning(this, "Security Warning", 
        "⚠️ WARNING: Disabling mitigations reduces security!\n\n"
        "This will modify GRUB configuration.\n"
        "You will need to update GRUB and REBOOT.\n\n"
        "Do you want to DISABLE mitigations?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QMessageBox::information(this, "Manual Step Required", 
        "Opening /etc/default/grub for editing...\n\n"
        "Add 'mitigations=off' to GRUB_CMDLINE_LINUX,\n"
        "then run: sudo grub-mkconfig -o /boot/grub/grub.cfg\n"
        "and REBOOT.");
    openConfigInNano("/etc/default/grub");
}

void MainWindow::on_performanceHacksApplyButton_clicked() {
    QMessageBox::information(this, "Performance Hacks", 
        "These optimizations require multiple steps:\n\n"
        "1. Install plocate: sudo pacman -S plocate\n"
        "2. Edit /etc/updatedb.conf\n"
        "3. Set performance mode: sudo powerprofilesctl set performance\n"
        "4. Edit /etc/mkinitcpio.conf to remove plymouth\n"
        "5. Run: sudo mkinitcpio -P\n\n"
        "See Info button for detailed instructions.");
}

void MainWindow::on_ptraceApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("ptrace Configuration");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Select ptrace scope:", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItem("0 - Unrestricted (any process can ptrace any other)", "0");
    combo->addItem("1 - Restricted (only parent can ptrace children)", "1");
    combo->addItem("2 - Admin-only (only root can use ptrace)", "2");
    combo->addItem("3 - Disabled (ptrace completely disabled)", "3");
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    QString scope = combo->currentData().toString();
    QString cmd = QString("sudo bash -c 'echo \"kernel.yama.ptrace_scope = %1\" > /etc/sysctl.d/10-ptrace.conf && "
                          "sysctl -p /etc/sysctl.d/10-ptrace.conf'").arg(scope);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'ptrace scope set to " + scope + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

void MainWindow::on_hidepidApplyButton_clicked() {
    int ret = QMessageBox::warning(this, "Security Configuration", 
        "⚠️ WARNING: This requires editing /etc/fstab and remounting /proc.\n\n"
        "This will hide processes from other users.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QMessageBox::information(this, "Manual Step Required", 
        "Opening /etc/fstab for editing...\n\n"
        "Find the /proc line and add hidepid=2:\n"
        "proc /proc proc defaults,hidepid=2 0 0\n\n"
        "Then run: sudo mount -o remount /proc");
    openConfigInNano("/etc/fstab");
}// New Arch Linux Tweaks Implementation

// Swappiness
void MainWindow::on_swappinessToggle_clicked() {
    QString instructions = R"(
# Swappiness Configuration Instructions
# ===================================
# 
# Swappiness controls how aggressively the kernel swaps memory pages to disk.
# Lower values keep more data in RAM, higher values swap more aggressively.
# 
# Recommended values:
# - Desktop: 10-30 (less swapping, better responsiveness)
# - Server: 1-10 (minimal swapping)
# - Default: 60
# 
# To set swappiness:
# Click the "Apply" button or edit /etc/sysctl.d/99-swappiness.conf:
vm.swappiness=10

# Then apply: sudo sysctl -p /etc/sysctl.d/99-swappiness.conf
# 
# To check current swappiness:
# cat /proc/sys/vm/swappiness
)";
    showTweakInstructions("Swappiness Configuration", instructions);
}

void MainWindow::on_swappinessConfigButton_clicked() {
    openConfigInNano("/etc/sysctl.d/99-swappiness.conf");
}

void MainWindow::checkSwappinessState() {
    QProcess proc;
    proc.start("cat", QStringList() << "/proc/sys/vm/swappiness");
    proc.waitForFinished();
    QString swappiness = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    int value = swappiness.toInt();
    bool optimized = (value <= 30);
    updateTweakStatusLabel(ui->swappinessStatusLabel, QString("Value: %1").arg(swappiness), optimized);
}

void MainWindow::on_swappinessApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Swappiness Configuration");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Set swappiness value (1-100):", &dialog);
    layout->addWidget(label);
    QSpinBox *spinBox = new QSpinBox(&dialog);
    spinBox->setRange(1, 100);
    spinBox->setValue(10);
    spinBox->setToolTip("Lower values = less swapping (recommended: 10-30)");
    layout->addWidget(spinBox);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    int value = spinBox->value();
    QString cmd = QString("sudo bash -c 'echo \"vm.swappiness=%1\" > /etc/sysctl.d/99-swappiness.conf && "
                          "sysctl -p /etc/sysctl.d/99-swappiness.conf'").arg(value);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'Swappiness set to " + QString::number(value) + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

// I/O Scheduler
void MainWindow::on_ioSchedulerToggle_clicked() {
    QString instructions = R"(
# I/O Scheduler Configuration Instructions
# =======================================
# 
# I/O schedulers control how the kernel handles disk I/O requests.
# Different schedulers are optimized for different storage types.
# 
# Recommended schedulers:
# - NVMe SSDs: none, mq-deadline, kyber
# - SATA SSDs: mq-deadline, bfq
# - HDDs: bfq, mq-deadline
# 
# To set I/O scheduler:
# Click the "Apply" button or create /etc/udev/rules.d/60-ioschedulers.rules:
# ACTION=="add|change", KERNEL=="sd[a-z]|nvme[0-9]*", ATTR{queue/scheduler}="mq-deadline"
# 
# Then reload: sudo udevadm control --reload-rules
# 
# To check current scheduler:
# cat /sys/block/nvme0n1/queue/scheduler
)";
    showTweakInstructions("I/O Scheduler Configuration", instructions);
}

void MainWindow::on_ioSchedulerConfigButton_clicked() {
    openConfigInNano("/etc/udev/rules.d/60-ioschedulers.rules");
}

void MainWindow::checkIoSchedulerState() {
    QProcess proc;
    proc.start("cat", QStringList() << "/sys/block/nvme0n1/queue/scheduler");
    proc.waitForFinished();
    QString scheduler = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (scheduler.isEmpty()) {
        proc.start("cat", QStringList() << "/sys/block/sda/queue/scheduler");
        proc.waitForFinished();
        scheduler = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    QString current = scheduler;
    if (scheduler.contains("[")) {
        QRegularExpression re("\\[([^\\]]+)\\]");
        QRegularExpressionMatch match = re.match(scheduler);
        if (match.hasMatch()) {
            current = match.captured(1);
        }
    }
    bool optimized = (current == "none" || current == "mq-deadline" || current == "kyber");
    updateTweakStatusLabel(ui->ioSchedulerStatusLabel, current.isEmpty() ? "Unknown" : current, optimized);
}

void MainWindow::on_ioSchedulerApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("I/O Scheduler Configuration");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Select I/O Scheduler:", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItems({"none", "mq-deadline", "kyber", "bfq"});
    combo->setCurrentText("mq-deadline");
    combo->setToolTip("none/mq-deadline/kyber for NVMe, mq-deadline/bfq for SATA");
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    QString scheduler = combo->currentText();
    QString cmd = QString("sudo bash -c 'echo \"ACTION==\\\"add|change\\\", KERNEL==\\\"sd[a-z]|nvme[0-9]*\\\", ATTR{queue/scheduler}=\\\"%1\\\"\" > /etc/udev/rules.d/60-ioschedulers.rules && "
                          "udevadm control --reload-rules && "
                          "udevadm trigger'").arg(scheduler);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'I/O Scheduler set to " + scheduler + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

// Transparent Huge Pages
void MainWindow::on_thpToggle_clicked() {
    QString instructions = R"(
# Transparent Huge Pages (THP) Configuration Instructions
# ======================================================
# 
# THP improves performance by using larger memory pages, reducing TLB misses.
# However, it can cause latency spikes in some workloads.
# 
# Options:
# - always: Always use THP (may cause latency spikes)
# - madvise: Use THP only for MADV_HUGEPAGE regions (recommended)
# - never: Disable THP
# 
# To configure THP:
# Click the "Apply" button or edit /etc/sysctl.d/99-thp.conf:
vm.nr_hugepages=0
kernel.shmmax=68719476736
kernel.shmall=4194304
# 
# For madvise mode (recommended):
# echo madvise | sudo tee /sys/kernel/mm/transparent_hugepage/enabled
# echo madvise | sudo tee /sys/kernel/mm/transparent_hugepage/defrag
# 
# To check current THP status:
# cat /sys/kernel/mm/transparent_hugepage/enabled
)";
    showTweakInstructions("Transparent Huge Pages Configuration", instructions);
}

void MainWindow::on_thpConfigButton_clicked() {
    openConfigInNano("/etc/sysctl.d/99-thp.conf");
}

void MainWindow::checkThpState() {
    QProcess proc;
    proc.start("cat", QStringList() << "/sys/kernel/mm/transparent_hugepage/enabled");
    proc.waitForFinished();
    QString thp = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    bool enabled = thp.contains("[madvise]") || thp.contains("[always]");
    QString status = "Disabled";
    if (thp.contains("[madvise]")) status = "Madvise";
    else if (thp.contains("[always]")) status = "Always";
    else if (thp.contains("[never]")) status = "Never";
    updateTweakStatusLabel(ui->thpStatusLabel, status, enabled);
}

void MainWindow::on_thpApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Transparent Huge Pages");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Select THP mode:", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItem("madvise (Recommended)", "madvise");
    combo->addItem("always (May cause latency)", "always");
    combo->addItem("never (Disabled)", "never");
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    QString mode = combo->currentData().toString();
    QString cmd = QString("sudo bash -c 'echo %1 > /sys/kernel/mm/transparent_hugepage/enabled && "
                          "echo %1 > /sys/kernel/mm/transparent_hugepage/defrag'").arg(mode);
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'THP set to " + mode + "!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

// TCP Optimizations
void MainWindow::on_tcpOptimizationsToggle_clicked() {
    QString instructions = R"(
# TCP Optimizations Configuration Instructions
# ===========================================
# 
# TCP optimizations improve network performance and reduce latency.
# 
# Common optimizations:
# - TCP Fast Open: Reduces connection latency
# - TCP Congestion Control: BBR for better throughput
# - TCP Window Scaling: Better performance on high-latency links
# 
# To configure TCP optimizations:
# Click the "Apply" button or edit /etc/sysctl.d/99-tcp-optimizations.conf:
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_congestion_control = bbr
# 
# Then apply: sudo sysctl -p /etc/sysctl.d/99-tcp-optimizations.conf
# 
# Note: BBR requires kernel 4.9+ and may need to be loaded: modprobe tcp_bbr
)";
    showTweakInstructions("TCP Optimizations Configuration", instructions);
}

void MainWindow::on_tcpOptimizationsConfigButton_clicked() {
    openConfigInNano("/etc/sysctl.d/99-tcp-optimizations.conf");
}

void MainWindow::checkTcpOptimizationsState() {
    QFile config("/etc/sysctl.d/99-tcp-optimizations.conf");
    bool configured = config.exists();
    updateTweakStatusLabel(ui->tcpOptimizationsStatusLabel, configured ? "Configured" : "Not Set", configured);
}

void MainWindow::on_tcpOptimizationsApplyButton_clicked() {
    int ret = QMessageBox::question(this, "Apply TCP Optimizations", 
        "This will create TCP optimization settings.\n\n"
        "These optimizations improve network performance.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QString cmd = "sudo bash -c 'cat > /etc/sysctl.d/99-tcp-optimizations.conf << EOF\n"
                  "net.core.rmem_max = 134217728\n"
                  "net.core.wmem_max = 134217728\n"
                  "net.ipv4.tcp_rmem = 4096 87380 67108864\n"
                  "net.ipv4.tcp_wmem = 4096 65536 67108864\n"
                  "net.ipv4.tcp_fastopen = 3\n"
                  "net.ipv4.tcp_congestion_control = bbr\n"
                  "EOF\n"
                  "sysctl -p /etc/sysctl.d/99-tcp-optimizations.conf'";
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'TCP optimizations applied!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

// Firewall
void MainWindow::on_firewallToggle_clicked() {
    QString instructions = R"(
# Firewall Configuration Instructions
# ===================================
# 
# Firewalls protect your system from unauthorized network access.
# 
# Available firewalls:
# - firewalld: Dynamic firewall daemon (recommended for desktop)
# - ufw: Uncomplicated Firewall (simple to use)
# - iptables: Traditional firewall (advanced)
# 
# To enable firewalld:
# sudo systemctl enable --now firewalld
# 
# To enable ufw:
# sudo systemctl enable --now ufw
# sudo ufw enable
# 
# To check firewall status:
# sudo firewall-cmd --state (for firewalld)
# sudo ufw status (for ufw)
)";
    showTweakInstructions("Firewall Configuration", instructions);
}

void MainWindow::on_firewallConfigButton_clicked() {
    QMessageBox::information(this, "Firewall Configuration", 
        "Firewall configuration depends on which firewall you're using:\n\n"
        "For firewalld:\n"
        "sudo firewall-cmd --permanent --add-service=http\n"
        "sudo firewall-cmd --reload\n\n"
        "For ufw:\n"
        "sudo ufw allow 22/tcp\n"
        "sudo ufw reload");
}

void MainWindow::checkFirewallState() {
    QProcess proc;
    proc.start("systemctl", QStringList() << "is-active" << "firewalld");
    proc.waitForFinished();
    bool firewalldActive = (proc.exitCode() == 0);
    
    if (!firewalldActive) {
        proc.start("systemctl", QStringList() << "is-active" << "ufw");
        proc.waitForFinished();
        bool ufwActive = (proc.exitCode() == 0);
        if (ufwActive) {
            updateTweakStatusLabel(ui->firewallStatusLabel, "UFW Active", true);
            return;
        }
    } else {
        updateTweakStatusLabel(ui->firewallStatusLabel, "Firewalld Active", true);
        return;
    }
    
    updateTweakStatusLabel(ui->firewallStatusLabel, "Disabled", false);
}

void MainWindow::on_firewallApplyButton_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Firewall Configuration");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("Select firewall to enable:", &dialog);
    layout->addWidget(label);
    QComboBox *combo = new QComboBox(&dialog);
    combo->addItem("firewalld (Recommended)", "firewalld");
    combo->addItem("ufw (Simple)", "ufw");
    layout->addWidget(combo);
    QPushButton *okButton = new QPushButton("Apply", &dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", &dialog);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    QString firewall = combo->currentData().toString();
    QString cmd;
    if (firewall == "firewalld") {
        cmd = "sudo systemctl enable --now firewalld";
    } else {
        cmd = "sudo systemctl enable --now ufw && sudo ufw --force enable";
    }
    
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd + " && echo 'Firewall enabled!' && read")) {
            QTimer::singleShot(2000, this, &MainWindow::refreshTweaksStatus);
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator.");
}

// Pacman Optimizations
void MainWindow::on_pacmanOptimizationsToggle_clicked() {
    QString instructions = R"(
# Pacman Optimizations Configuration Instructions
# ==============================================
# 
# Pacman optimizations improve package management performance.
# 
# Recommended settings in /etc/pacman.conf:
# - ParallelDownloads = 10 (faster downloads)
# - Color (enabled by default)
# - ILoveCandy (progress bar)
# 
# To configure Pacman:
# Click the "Apply" button or edit /etc/pacman.conf:
# 
# Uncomment or add:
# Color
# ParallelDownloads = 10
# 
# In [options] section, add:
# ILoveCandy
)";
    showTweakInstructions("Pacman Optimizations Configuration", instructions);
}

void MainWindow::on_pacmanOptimizationsConfigButton_clicked() {
    openConfigInNano("/etc/pacman.conf");
}

void MainWindow::checkPacmanOptimizationsState() {
    QFile pacman("/etc/pacman.conf");
    bool hasParallel = false;
    bool hasColor = false;
    bool hasCandy = false;
    
    if (pacman.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&pacman);
        QString content = in.readAll();
        pacman.close();
        hasParallel = content.contains("ParallelDownloads");
        hasColor = content.contains("Color") && !content.contains("#Color");
        hasCandy = content.contains("ILoveCandy");
    }
    
    bool optimized = hasParallel && hasColor;
    QString status = "Partial";
    if (optimized && hasCandy) status = "Optimized";
    else if (optimized) status = "Partial";
    else status = "Default";
    
    updateTweakStatusLabel(ui->pacmanOptimizationsStatusLabel, status, optimized);
}

void MainWindow::on_pacmanOptimizationsApplyButton_clicked() {
    int ret = QMessageBox::question(this, "Apply Pacman Optimizations", 
        "This will add optimizations to /etc/pacman.conf:\n\n"
        "- ParallelDownloads = 10\n"
        "- Color (if not already enabled)\n"
        "- ILoveCandy (progress bar)\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QMessageBox::information(this, "Manual Configuration", 
        "Opening /etc/pacman.conf for editing...\n\n"
        "Add these lines in the [options] section:\n"
        "Color\n"
        "ParallelDownloads = 10\n"
        "ILoveCandy");
    openConfigInNano("/etc/pacman.conf");
}

// Journald Settings
void MainWindow::on_journaldToggle_clicked() {
    QString instructions = R"(
# Journald Settings Configuration Instructions
# ============================================
# 
# Journald manages system logs. Limiting log size prevents disk space issues.
# 
# To configure journald:
# Click the "Apply" button or edit /etc/systemd/journald.conf:
# 
# Set log size limits:
# SystemMaxUse=500M
# SystemKeepFree=1G
# SystemMaxFileSize=50M
# 
# Then restart: sudo systemctl restart systemd-journald
# 
# To check current journal size:
# journalctl --disk-usage
)";
    showTweakInstructions("Journald Settings Configuration", instructions);
}

void MainWindow::on_journaldConfigButton_clicked() {
    openConfigInNano("/etc/systemd/journald.conf");
}

void MainWindow::checkJournaldState() {
    QFile journald("/etc/systemd/journald.conf");
    bool configured = false;
    if (journald.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&journald);
        QString content = in.readAll();
        journald.close();
        configured = content.contains("SystemMaxUse") || content.contains("SystemKeepFree");
    }
    updateTweakStatusLabel(ui->journaldStatusLabel, configured ? "Configured" : "Default", configured);
}

void MainWindow::on_journaldApplyButton_clicked() {
    int ret = QMessageBox::question(this, "Apply Journald Settings", 
        "This will add log size limits to /etc/systemd/journald.conf:\n\n"
        "SystemMaxUse=500M\n"
        "SystemKeepFree=1G\n"
        "SystemMaxFileSize=50M\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    
    QMessageBox::information(this, "Manual Configuration", 
        "Opening /etc/systemd/journald.conf for editing...\n\n"
        "Uncomment and set:\n"
        "SystemMaxUse=500M\n"
        "SystemKeepFree=1G\n"
        "SystemMaxFileSize=50M\n\n"
        "Then restart: sudo systemctl restart systemd-journald");
    openConfigInNano("/etc/systemd/journald.conf");
}
