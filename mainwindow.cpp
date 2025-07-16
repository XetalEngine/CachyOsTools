#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "includes.h"

#include "mainwindow_d.h"
#include "drive.h"
#include "executecommands.h"
#include "shell.h"
#include "services.h"
#include "tweaks.h"
#include "systemconfigs.h"
#include "systemlogs.h"
#include "systembackup.h"
#include "systemrestore.h"
#include "current_system_iso_creator.h"

void MainWindow::setNumlockSetting(bool enabled) {
    QString value = enabled ? "1" : "0";
    QProcess::startDetached("kwriteconfig5", QStringList() << "--file" << "kcminputrc" << "--group" << "Keyboard" << "--key" << "NumLock" << value);
}

// Helper to run a sudo command in a terminal
void MainWindow::runSudoCommandInTerminal(const QString &cmd) {
    QStringList terminals = {"konsole", "gnome-terminal", "xterm", "alacritty", "kitty"};
    for (const QString &terminal : terminals) {
        if (QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << cmd)) {
            return;
        }
    }
    QMessageBox::warning(this, "Terminal Not Found", "Could not find a suitable terminal emulator. Please install one of: konsole, gnome-terminal, xterm, alacritty, or kitty");
}

