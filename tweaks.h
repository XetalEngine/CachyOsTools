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
    // Check if fstrim timer is enabled
    QProcess proc;
    proc.start("systemctl", QStringList() << "is-enabled" << "fstrim.timer");
    proc.waitForFinished();
    QString status = (proc.exitCode() == 0) ? "enabled" : "disabled";
    openConfigInNano("/etc/fstab");
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

// Tweaks tab state checking functions removed - now using instruction dialogs instead