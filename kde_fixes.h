// KDE Plasma known-issue fixes — Tweaks > KDE Plasma > "Fix Known Issues".
//
// These are not tuning knobs like the rest of kde_tweaks.h. Each row here is a
// specific, reproducible Plasma defect: the tool detects whether this session is
// actually affected, explains what it found, and repairs it.
//
// Everything is per-user and needs no root.
//
// THE ONE RULE THAT MATTERS HERE
// plasmashell keeps plasma-org.kde.plasma.desktop-appletsrc in memory and
// rewrites the whole file from that copy whenever anything changes. Editing it
// while plasmashell runs is a race you lose: your write survives until the next
// autosave and is then silently reverted. Every fix that touches that file stops
// plasmashell first and restarts it afterwards.

#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QThread>
#include <QBuffer>
#include <QImage>
#include <QMap>
#include <algorithm>
#include <functional>

// ── Shared plumbing ──────────────────────────────────────────────────────────

QString MainWindow::kdeAppletsRcPath() const {
    return QDir::homePath() + "/.config/plasma-org.kde.plasma.desktop-appletsrc";
}

// Runs a qdbus call against plasmashell's scripting interface. Plasma 6 ships
// qdbus6; qdbus is the Plasma 5 / distro-alias name. Empty return means the call
// did not land — usually "not a running Plasma session".
static QString kdePlasmaScript(const QString &script, int timeoutMs = 8000) {
    for (const QString &tool : {QStringLiteral("qdbus6"), QStringLiteral("qdbus")}) {
        QProcess proc;
        proc.start(tool, QStringList()
                             << "org.kde.plasmashell" << "/PlasmaShell"
                             << "org.kde.PlasmaShell.evaluateScript" << script);
        if (!proc.waitForStarted(2000)) continue;
        if (!proc.waitForFinished(timeoutMs)) { proc.kill(); continue; }
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) continue;
        return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    return QString();
}

static bool kdePlasmaShellRunning() {
    QProcess proc;
    proc.start("pgrep", QStringList() << "-x" << "plasmashell");
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

// Asks plasmashell to quit and waits for the process to actually be gone, so the
// caller knows the config file is no longer owned by anyone. Returns false if it
// is still alive after the grace period — callers must then abort rather than
// write underneath a live shell.
bool MainWindow::kdeStopPlasmaShell() {
    if (!kdePlasmaShellRunning()) return true;

    QProcess::execute("kquitapp6", QStringList() << "plasmashell");
    if (kdePlasmaShellRunning())
        QProcess::execute("kquitapp5", QStringList() << "plasmashell");

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10000) {
        if (!kdePlasmaShellRunning()) return true;
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);
        QThread::msleep(200);
    }
    return !kdePlasmaShellRunning();
}

void MainWindow::kdeStartPlasmaShell() {
    if (kdePlasmaShellRunning()) return;
    // startDetached double-forks, so the shell outlives this application.
    if (!QProcess::startDetached("plasmashell", QStringList()))
        QProcess::startDetached("kstart", QStringList() << "plasmashell");
}

// ── Reading the applets file ─────────────────────────────────────────────────
//
// Parsed by hand rather than through QSettings: QSettings mangles KConfig group
// syntax and the values here contain commas, quotes and colons that its INI
// codec rewrites. Detection only ever reads — every write goes through
// kwriteconfig6, which speaks KConfig properly.

// Splits the file into (group header, key/value map) in file order.
static QList<QPair<QString, QMap<QString, QString>>> kdeParseAppletsRc(const QString &path) {
    QList<QPair<QString, QMap<QString, QString>>> groups;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return groups;

    QTextStream in(&f);
    QString current;
    QMap<QString, QString> values;
    bool started = false;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith('[')) {
            if (started) groups.append({current, values});
            current = trimmed;
            values.clear();
            started = true;
            continue;
        }
        if (!started || trimmed.isEmpty() || trimmed.startsWith('#')) continue;
        const int eq = trimmed.indexOf('=');
        if (eq <= 0) continue;
        values.insert(trimmed.left(eq).trimmed(), trimmed.mid(eq + 1));
    }
    if (started) groups.append({current, values});
    return groups;
}

// Containment ids whose plugin is the Folder View desktop. These are the only
// containments the drag fix touches — panels and plain desktops are left alone.
QStringList MainWindow::kdeFolderContainmentIds() const {
    QStringList ids;
    static const QRegularExpression re(QStringLiteral("^\\[Containments\\]\\[(\\d+)\\]$"));
    for (const auto &group : kdeParseAppletsRc(kdeAppletsRcPath())) {
        const auto m = re.match(group.first);
        if (!m.hasMatch()) continue;
        if (group.second.value("plugin") == "org.kde.plasma.folder")
            ids << m.captured(1);
    }
    return ids;
}

// ── Issue 1: Folder View will not accept drags on a secondary screen ─────────
//
// Three independent causes, all of which look identical from the user's side —
// the icon refuses to move, or snaps back the instant it is dropped.
//
// Returns one line per cause found. Empty means this session is clean.
QStringList MainWindow::kdeScanFolderDragIssues() const {
    QStringList issues;
    const auto groups = kdeParseAppletsRc(kdeAppletsRcPath());
    const QStringList folders = kdeFolderContainmentIds();

    static const QRegularExpression containmentRe(QStringLiteral("^\\[Containments\\]\\[(\\d+)\\]$"));
    static const QRegularExpression generalRe(QStringLiteral("^\\[Containments\\]\\[(\\d+)\\]\\[General\\]$"));

    for (const auto &group : groups) {
        // (a) ScreenMapping — Plasma remembers which screen each desktop file
        //     belongs to so files do not shuffle when a monitor is unplugged.
        //     When that map goes stale it pins every file to one screen and the
        //     other Folder View rejects the drop.
        if (group.first == "[ScreenMapping]") {
            const QString disabled = group.second.value("itemsOnDisabledScreens").trimmed();
            if (!disabled.isEmpty()) {
                // Format: <count>,<activityId>,<count>,<path>,<path>...
                const int stale = disabled.split(',', Qt::SkipEmptyParts).size();
                issues << QObject::tr("Stale itemsOnDisabledScreens: %1 entries still assigned "
                                      "to a screen that is not connected.").arg(stale);
            }

            const QString mapping = group.second.value("screenMapping").trimmed();
            if (!mapping.isEmpty() && folders.size() > 1) {
                // Triples of path,screen,activityId — collect the screen column.
                const QStringList parts = mapping.split(',', Qt::SkipEmptyParts);
                QSet<QString> screens;
                for (int i = 1; i + 1 < parts.size(); i += 3) screens.insert(parts.at(i).trimmed());
                if (screens.size() == 1) {
                    issues << QObject::tr("screenMapping pins all %1 mapped desktop items to "
                                          "screen %2, even though %3 Folder View desktops exist.")
                                  .arg((parts.size() + 2) / 3)
                                  .arg(*screens.constBegin())
                                  .arg(folders.size());
                }
            }
            continue;
        }

        // (b) A sorted Folder View has no free positions to drop into. Manual
        //     arrangement is sortMode=-1; anything else makes Plasma recompute
        //     the layout and the icon springs back.
        const auto gm = generalRe.match(group.first);
        if (gm.hasMatch() && folders.contains(gm.captured(1))) {
            const QString sortMode = group.second.value("sortMode", "0").trimmed();
            if (sortMode != "-1")
                issues << QObject::tr("Desktop %1 has sortMode=%2 (sorted). Icons cannot be "
                                      "placed by hand unless sorting is Manual (-1).")
                              .arg(gm.captured(1), sortMode);
            continue;
        }

        // (c) A locked containment refuses every interaction, including drops.
        //     1 = Mutable, 2 = UserImmutable, 3 = SystemImmutable.
        const auto cm = containmentRe.match(group.first);
        if (cm.hasMatch() && folders.contains(cm.captured(1))) {
            const int imm = group.second.value("immutability", "1").trimmed().toInt();
            if (imm > 1)
                issues << QObject::tr("Desktop %1 has immutability=%2 — this containment is "
                                      "locked and will reject drops.")
                              .arg(cm.captured(1)).arg(imm);
        }
    }
    return issues;
}

void MainWindow::checkkdeFixFolderDragState() {
    if (!QFileInfo::exists(kdeAppletsRcPath())) {
        updateTweakStatusLabel(ui->kdeFixFolderDragStatusLabel, "No config", false);
        return;
    }
    const int n = kdeScanFolderDragIssues().size();
    // Green means nothing to fix, which is the inverse of the tweak rows above.
    updateTweakStatusLabel(ui->kdeFixFolderDragStatusLabel,
                           n == 0 ? "OK" : QString("%1 issue%2").arg(n).arg(n == 1 ? "" : "s"),
                           n == 0);
}

void MainWindow::on_kdeFixFolderDragToggle_clicked() {
    QString found;
    const QStringList issues = kdeScanFolderDragIssues();
    if (issues.isEmpty()) {
        found = "Nothing detected on this session — the Fix button will still\n"
                "reset the screen mapping if you want to force it.\n";
    } else {
        found = "DETECTED ON THIS SESSION\n";
        for (const QString &i : issues) found += "  * " + i + "\n";
    }

    showTweakInstructions("Folder View Drag and Drop on a Secondary Screen",
QString(
R"(# Folder View — cannot drag icons or drop files on a secondary screen
# ==================================================================

# %1
# THE SYMPTOM
# Two monitors, both desktops set to Folder View, and one of them will not let
# you move an icon or drop a file. Nothing is reported — the icon simply snaps
# back, or the drop cursor never turns into a copy/move cursor.
#
# WHY IT HAPPENS
# There are three separate causes, and they look identical from the outside.
#
# 1. ScreenMapping goes stale.
#    ~/.config/plasma-org.kde.plasma.desktop-appletsrc has a [ScreenMapping]
#    group. It exists so that unplugging a monitor does not scatter your desktop
#    files: Plasma records which screen each file belongs to and puts it back.
#
#      screenMapping=<path>,<screen>,<activityId>,<path>,<screen>,<activityId>,...
#      itemsOnDisabledScreens=<count>,<activityId>,<count>,<path>,<path>,...
#
#    The map is written on every hotplug, resolution change, activity switch and
#    layout reset, and it is well known upstream to drift out of sync — Plasma
#    ships its own migration script for one corruption class of it
#    (folderview_fix_recursive_screenmapping.js) and the feature was rewritten
#    for 6.1 for exactly this reason.
#
#    Once a file is pinned to screen 0, the Folder View on screen 1 believes the
#    file already lives somewhere else and refuses the drop. The tell is a
#    screenMapping where every entry names the same screen while two or more
#    Folder View desktops exist, or a non-empty itemsOnDisabledScreens listing
#    files parked on a monitor you no longer have plugged in.
#
# 2. The desktop is sorted, not manually arranged.
#    Manual placement only exists when sorting is off. In the config that is
#    sortMode=-1; any other value means Plasma recomputes the icon grid, so an
#    icon you drag is immediately put back where the sort order says it goes.
#    In the GUI: right-click the desktop > Desktop and Wallpaper > Icons >
#    Sorting: Manual.
#
# 3. The containment is locked.
#    immutability=2 (UserImmutable) or 3 (SystemImmutable) on the containment
#    makes it reject every interaction, drops included. 1 is Mutable, which is
#    what you want. This is a per-desktop value, so one screen can be locked
#    while the other is not — which is exactly what "it works on the left
#    monitor" looks like.
#
# WHAT THE FIX BUTTON DOES
#   1. Copies desktop-appletsrc to ~/configbackups/ first.
#   2. Stops plasmashell. This is not optional: plasmashell holds the whole file
#      in memory and rewrites it from that copy, so an edit made while it runs is
#      reverted at the next autosave.
#   3. Deletes screenMapping and itemsOnDisabledScreens. Plasma rebuilds the map
#      from scratch, correctly, from where the icons actually are.
#   4. Sets sortMode=-1 and immutability=1 on every Folder View desktop.
#   5. Starts plasmashell again.
#
# WHAT IT COSTS
# Your desktop disappears for a few seconds while the shell restarts. Icon
# positions are kept (those live in `positions`, which is not touched). What is
# lost is the memory of which screen each file was on, so after the next monitor
# unplug the files may land on the remaining screen instead of being restored.
# That map rebuilds itself as you use the desktop.
#
# IF IT COMES BACK
# The mapping re-corrupting after a hotplug is the upstream bug, not this fix
# failing. Re-running it is harmless.
#
# FILE
# ~/.config/plasma-org.kde.plasma.desktop-appletsrc
)").arg(found));
}

void MainWindow::on_kdeFixFolderDragApplyButton_clicked() {
    const QString path = kdeAppletsRcPath();
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Folder View Fix"),
                             tr("%1 does not exist.\n\nIs this a Plasma session?").arg(path));
        return;
    }

    const QStringList issues = kdeScanFolderDragIssues();
    const QStringList folders = kdeFolderContainmentIds();
    if (folders.isEmpty()) {
        QMessageBox::information(this, tr("Folder View Fix"),
                                 tr("No Folder View desktops found in %1.\n\n"
                                    "Set at least one desktop to the Folder View layout first "
                                    "(right-click the desktop > Desktop and Wallpaper > "
                                    "Layout: Folder View).").arg(path));
        return;
    }

    QString detail = issues.isEmpty()
        ? tr("No problem was detected, but the reset can be applied anyway.\n\n")
        : tr("Detected:\n  • %1\n\n").arg(issues.join("\n  • "));

    const int ret = QMessageBox::question(
        this, tr("Fix Folder View Drag and Drop"),
        detail +
        tr("This will:\n"
           "  • back up desktop-appletsrc to ~/configbackups/\n"
           "  • quit plasmashell (the desktop and panels vanish briefly)\n"
           "  • delete screenMapping and itemsOnDisabledScreens\n"
           "  • set sortMode=-1 and immutability=1 on Folder View desktop(s): %1\n"
           "  • start plasmashell again\n\n"
           "Icon positions are preserved. Continue?").arg(folders.join(", ")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    // Back up before anything else, and refuse to continue without one.
    const QString backupDir = QDir::homePath() + "/configbackups";
    QDir().mkpath(backupDir);
    const QString backup = QString("%1/plasma-org.kde.plasma.desktop-appletsrc_%2.backup")
                               .arg(backupDir,
                                    QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    if (!QFile::copy(path, backup)) {
        QMessageBox::critical(this, tr("Folder View Fix"),
                              tr("Could not write a backup to %1.\n\nNothing was changed.")
                                  .arg(backup));
        return;
    }

    if (!kdeStopPlasmaShell()) {
        QMessageBox::critical(this, tr("Folder View Fix"),
                              tr("plasmashell did not quit.\n\n"
                                 "Nothing was changed — editing the config while plasmashell "
                                 "is running would be overwritten by it anyway."));
        return;
    }

    // kwriteconfig6 speaks KConfig, so nested groups and escaping are its problem
    // rather than ours. Values starting with '-' need '--' to stop option parsing.
    auto writeKey = [&](const QStringList &groups, const QString &key, const QString &value) {
        QStringList args{"--file", path};
        for (const QString &g : groups) args << "--group" << g;
        args << "--key" << key;
        if (value.isNull()) args << "--delete" << "";
        else                args << "--" << value;
        QProcess::execute("kwriteconfig6", args);
    };

    writeKey({"ScreenMapping"}, "screenMapping", QString());
    writeKey({"ScreenMapping"}, "itemsOnDisabledScreens", QString());
    for (const QString &id : folders) {
        writeKey({"Containments", id, "General"}, "sortMode", "-1");
        writeKey({"Containments", id}, "immutability", "1");
    }

    kdeStartPlasmaShell();

    QMessageBox::information(
        this, tr("Folder View Fix Applied"),
        tr("Screen mapping reset and %1 Folder View desktop(s) unlocked and set to manual "
           "arrangement.\n\nBackup: %2\n\nplasmashell is restarting — give it a few seconds, "
           "then try dragging a file onto the other monitor.")
            .arg(folders.size()).arg(backup));

    QTimer::singleShot(4000, this, &MainWindow::refreshTweaksStatus);
}

void MainWindow::on_kdeFixFolderDragBackupButton_clicked() {
    backupConfigFile(kdeAppletsRcPath(), "Plasma desktop, panel and screen-mapping layout");
}

void MainWindow::on_kdeFixFolderDragConfigButton_clicked() {
    QMessageBox::information(
        this, tr("Edit desktop-appletsrc"),
        tr("plasmashell rewrites this file from memory, so an edit made while it is running "
           "will be reverted.\n\nTo edit by hand: quit plasmashell first (Restart Plasma Shell "
           "below quits and restarts it), or use the Fix button, which handles that for you."));
    openConfigInNano(kdeAppletsRcPath());
}

// ── Issue 2: widgets locked ──────────────────────────────────────────────────

// Read live rather than from the file: the lock is a session-wide state that
// plasmashell owns, and the file only reflects it after a write.
int MainWindow::kdeWidgetsLockedState() const {
    const QString out = kdePlasmaScript(QStringLiteral("print(locked);"));
    if (out.isEmpty()) return -1;
    if (out.contains("true"))  return 1;
    if (out.contains("false")) return 0;
    return -1;
}

void MainWindow::checkkdeFixWidgetsLockedState() {
    const int state = kdeWidgetsLockedState();
    if (state < 0) {
        updateTweakStatusLabel(ui->kdeFixWidgetsLockedStatusLabel, "Unknown", false);
        return;
    }
    updateTweakStatusLabel(ui->kdeFixWidgetsLockedStatusLabel,
                           state == 1 ? "Locked" : "Unlocked", state == 0);
}

void MainWindow::on_kdeFixWidgetsLockedToggle_clicked() {
    showTweakInstructions("Plasma Widgets Locked",
R"(# Plasma widgets locked
# =====================
#
# THE SYMPTOM
# Nothing on the desktop or the panel can be moved. Desktop icons will not drag,
# applets have no handles, right-clicking offers no "Remove" or "Configure", and
# panel Edit Mode does nothing.
#
# WHY IT HAPPENS
# Plasma has a session-wide widget lock. It is easy to turn on by accident — it
# used to sit in the desktop context menu right next to entries people use daily,
# and some layout-switching or theme scripts set it and never clear it.
#
# It is also sticky in a confusing way: the lock is stored per containment as
# `immutability` in desktop-appletsrc, so a partially-applied lock can leave one
# monitor locked and another not. That reads as "drag and drop is broken on my
# second screen" rather than "widgets are locked", which is why this row exists
# separately from the Folder View fix above.
#
#   immutability=1  Mutable        — normal, everything can be moved
#   immutability=2  UserImmutable  — locked by you, unlockable from the GUI
#   immutability=3  SystemImmutable — locked by policy, GUI cannot unlock it
#
# WHAT THE FIX BUTTON DOES
# Sets the session-wide lock to off through plasmashell's own scripting
# interface (`locked = false`) rather than editing the config file, so it applies
# immediately and plasmashell does not overwrite it. No logout, no restart.
#
# BY HAND
# Right-click the desktop and pick "Unlock Widgets" if the entry is there. If it
# is not, the lock is SystemImmutable and comes from a system-wide kiosk policy
# in /etc/xdg/plasmarc or a kiosk profile — that one is deliberate and needs root
# to change.
)");
}

void MainWindow::on_kdeFixWidgetsLockedApplyButton_clicked() {
    const int state = kdeWidgetsLockedState();
    if (state < 0) {
        QMessageBox::warning(this, tr("Unlock Widgets"),
                             tr("Could not reach plasmashell over D-Bus.\n\n"
                                "Is this a running Plasma session?"));
        return;
    }
    if (state == 0) {
        QMessageBox::information(this, tr("Unlock Widgets"),
                                 tr("Widgets are already unlocked."));
        return;
    }
    kdePlasmaScript(QStringLiteral("locked = false;"));

    if (kdeWidgetsLockedState() == 1) {
        QMessageBox::warning(
            this, tr("Unlock Widgets"),
            tr("plasmashell still reports widgets as locked.\n\n"
               "That usually means the lock is SystemImmutable — set by a system-wide kiosk "
               "policy (/etc/xdg/plasmarc or a kiosk profile) rather than by you. Changing it "
               "needs root."));
    } else {
        QMessageBox::information(this, tr("Unlock Widgets"), tr("Widgets unlocked."));
    }
    QTimer::singleShot(500, this, &MainWindow::refreshTweaksStatus);
}

// ── Issue 3: corrupt / orphaned groups in desktop-appletsrc ──────────────────
//
// Two things this catches:
//   * group headers whose first component is a mangled "Containments" — the
//     signature of a hand edit or a sed script that ate the brackets, e.g.
//     [Containments158Appletsts][185][Configuration][Appearance]
//   * the same group header appearing twice, which makes KConfig keep only one
//     of them and silently drop the other's keys

// A header is mangled when its first component is "Containments" with something
// glued onto it — no legitimate top-level group is a prefix-plus-suffix of it.
// Shared by the scan and the removal so the two cannot disagree about what is
// unconditionally junk versus what is merely a later duplicate.
static bool kdeHeaderIsMangled(const QString &header) {
    static const QRegularExpression firstComponent(QStringLiteral("^\\[([^\\]]*)\\]"));
    const auto m = firstComponent.match(header);
    if (!m.hasMatch()) return true;
    const QString first = m.captured(1);
    return first != "Containments" && first.startsWith("Containments");
}

QStringList MainWindow::kdeScanConfigCorruption() const {
    QStringList bad;
    QSet<QString> seen;

    for (const auto &group : kdeParseAppletsRc(kdeAppletsRcPath())) {
        const QString header = group.first;

        if (seen.contains(header)) {
            if (!bad.contains(header)) bad << header;   // duplicate
            continue;
        }
        seen.insert(header);

        if (kdeHeaderIsMangled(header)) bad << header;
    }
    return bad;
}

void MainWindow::checkkdeFixConfigCorruptState() {
    if (!QFileInfo::exists(kdeAppletsRcPath())) {
        updateTweakStatusLabel(ui->kdeFixConfigCorruptStatusLabel, "No config", false);
        return;
    }
    const int n = kdeScanConfigCorruption().size();
    updateTweakStatusLabel(ui->kdeFixConfigCorruptStatusLabel,
                           n == 0 ? "OK" : QString("%1 bad").arg(n), n == 0);
}

void MainWindow::on_kdeFixConfigCorruptToggle_clicked() {
    const QStringList bad = kdeScanConfigCorruption();
    QString found;
    if (bad.isEmpty()) {
        found = "Nothing detected on this session.\n";
    } else {
        found = "DETECTED ON THIS SESSION\n";
        for (const QString &b : bad) found += "  * " + b + "\n";
    }

    showTweakInstructions("Corrupt Groups in desktop-appletsrc",
QString(
R"(# Corrupt or orphaned groups in plasma-org.kde.plasma.desktop-appletsrc
# =====================================================================

# %1
# WHAT THIS LOOKS FOR
#
# 1. Mangled group headers.
#    A valid header looks like
#      [Containments][158][Applets][185][Configuration][Appearance]
#    Corruption from a hand edit, a sed script or an interrupted write collapses
#    the brackets and produces things like
#      [Containments158Appletsts][185][Configuration][Appearance]
#    KConfig parses that happily as a top-level group nobody reads, so the
#    settings underneath it are silently inert. That is why a widget can lose its
#    configuration for no visible reason and reset to defaults.
#
# 2. Duplicate group headers.
#    If the same header appears twice, KConfig keeps one and drops the other's
#    keys. Which one wins is not something you want to depend on.
#
# WHAT THE FIX BUTTON DOES
#   1. Copies the file to ~/configbackups/.
#   2. Stops plasmashell — it rewrites this file from memory, so an edit made
#      while it runs is reverted.
#   3. Removes the offending groups and everything under them.
#   4. Starts plasmashell again.
#
# WHAT IT COSTS
# The removed groups are already dead settings, so in practice nothing. If a
# mangled group happened to be the only copy of a widget's configuration, that
# widget goes back to its defaults — it was already running on defaults, because
# nothing was reading that group.
#
# WHAT THIS DOES NOT DO
# It does not rebuild the file or reset your layout. Only the specific groups
# listed above are removed.
#
# FILE
# ~/.config/plasma-org.kde.plasma.desktop-appletsrc
)").arg(found));
}

void MainWindow::on_kdeFixConfigCorruptApplyButton_clicked() {
    const QString path = kdeAppletsRcPath();
    const QStringList bad = kdeScanConfigCorruption();
    if (bad.isEmpty()) {
        QMessageBox::information(this, tr("Config Integrity"),
                                 tr("No corrupt or duplicate groups found in %1.").arg(path));
        return;
    }

    const int ret = QMessageBox::question(
        this, tr("Remove Corrupt Groups"),
        tr("These %1 group(s) are unreadable by Plasma and their settings are inert:\n\n%2\n\n"
           "Remove them (and the keys under them)?\n\n"
           "The file is backed up to ~/configbackups/ first, and plasmashell is restarted.")
            .arg(bad.size()).arg(bad.join("\n")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const QString backupDir = QDir::homePath() + "/configbackups";
    QDir().mkpath(backupDir);
    const QString backup = QString("%1/plasma-org.kde.plasma.desktop-appletsrc_%2.backup")
                               .arg(backupDir,
                                    QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    if (!QFile::copy(path, backup)) {
        QMessageBox::critical(this, tr("Config Integrity"),
                              tr("Could not write a backup to %1.\n\nNothing was changed.")
                                  .arg(backup));
        return;
    }

    if (!kdeStopPlasmaShell()) {
        QMessageBox::critical(this, tr("Config Integrity"),
                              tr("plasmashell did not quit.\n\nNothing was changed."));
        return;
    }

    // Rewritten line by line rather than through kwriteconfig6, which has no
    // delete-group operation. A group runs from its header to the next header.
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly | QIODevice::Text)) {
        kdeStartPlasmaShell();
        QMessageBox::critical(this, tr("Config Integrity"),
                              tr("Could not read %1.").arg(path));
        return;
    }
    QStringList kept;
    bool dropping = false;
    int removedLines = 0;
    {
        QTextStream stream(&in);
        // A mangled header always goes. A duplicate must lose only its later
        // copies, so the first one through is kept and recorded.
        QSet<QString> emitted;
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            const QString trimmed = line.trimmed();
            if (trimmed.startsWith('[')) {
                dropping = bad.contains(trimmed) &&
                           (kdeHeaderIsMangled(trimmed) || emitted.contains(trimmed));
                if (!dropping) emitted.insert(trimmed);
            }
            if (dropping) { ++removedLines; continue; }
            kept << line;
        }
    }
    in.close();

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        kdeStartPlasmaShell();
        QMessageBox::critical(this, tr("Config Integrity"),
                              tr("Could not write %1.\n\nRestore from %2 if the desktop "
                                 "misbehaves.").arg(path, backup));
        return;
    }
    {
        QTextStream stream(&out);
        for (const QString &line : kept) stream << line << "\n";
    }
    out.close();

    kdeStartPlasmaShell();

    QMessageBox::information(
        this, tr("Corrupt Groups Removed"),
        tr("Removed %1 group(s), %2 line(s).\n\nBackup: %3\n\nplasmashell is restarting.")
            .arg(bad.size()).arg(removedLines).arg(backup));

    QTimer::singleShot(4000, this, &MainWindow::refreshTweaksStatus);
}

void MainWindow::on_kdeFixConfigCorruptBackupButton_clicked() {
    backupConfigFile(kdeAppletsRcPath(), "Plasma desktop and panel layout");
}

void MainWindow::on_kdeFixConfigCorruptConfigButton_clicked() {
    on_kdeFixFolderDragConfigButton_clicked();
}

// ── Issue 4: stale Plasma / Qt caches ────────────────────────────────────────

// The caches this fix clears, under ~/.cache. Matched by glob rather than exact
// name: several carry a version or a digest of the search path in the filename
// (ksycoca6_en_0Pk7KX...=, plasma_theme_default.kcache vs _v5.kcache), so a
// hardcoded list silently misses the very file that is stale.
static QStringList kdeCacheTargets() {
    static const QStringList patterns = {
        "plasmashell",            // per-applet compiled state
        "plasma_theme_*.kcache",  // theme colour/element lookups
        "plasma-svgelements*",    // rasterised theme SVGs
        "icon-cache.kcache",      // icon lookups
        "ksycoca*",               // service / mimetype / application-menu database
        "qmlcache",               // compiled QML
        "kwin",                   // compositor shader and effect cache
    };
    const QString c = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QDir dir(c);
    if (!dir.exists()) return {};

    QStringList targets;
    for (const QString &name : dir.entryList(patterns, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
        targets << dir.filePath(name);
    return targets;
}

static qint64 kdePathBytes(const QString &path) {
    const QFileInfo fi(path);
    if (!fi.exists()) return 0;
    if (fi.isFile()) return fi.size();
    qint64 total = 0;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); total += it.fileInfo().size(); }
    return total;
}

qint64 MainWindow::kdePlasmaCacheBytes() const {
    qint64 total = 0;
    for (const QString &t : kdeCacheTargets()) total += kdePathBytes(t);
    return total;
}

void MainWindow::checkkdeFixPlasmaCacheState() {
    const qint64 bytes = kdePlasmaCacheBytes();
    // Nothing here is an error condition — the number is informational, so the
    // label reports size and stays neutral-green whenever a cache exists at all.
    updateTweakStatusLabel(ui->kdeFixPlasmaCacheStatusLabel,
                           bytes == 0 ? "Empty"
                                      : QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1),
                           true);
}

void MainWindow::on_kdeFixPlasmaCacheToggle_clicked() {
    showTweakInstructions("Stale Plasma and Qt Caches",
R"(# Stale Plasma / Qt caches
# ========================
#
# THE SYMPTOMS
#   * Icons missing, wrong, or showing the generic "unknown file" glyph
#   * The panel comes up blank, or a widget shows a permanent loading spinner
#   * The application menu lists programs you uninstalled, or is missing ones you
#     installed
#   * A theme change applies to some elements and not others
#   * Plasma crash-loops immediately after a Plasma or Qt package upgrade
#
# WHY IT HAPPENS
# Plasma caches aggressively, and several of those caches are memory-mapped
# binary blobs rather than files that get revalidated:
#
#   ~/.cache/plasmashell/            per-applet compiled state
#   ~/.cache/plasma-svgelements*     rasterised theme SVGs
#   ~/.cache/plasma_theme_*.kcache   theme colour and element lookups
#   ~/.cache/icon-cache.kcache       icon lookups across every theme
#   ~/.cache/ksycoca6_en             the service/mimetype/menu database
#   ~/.cache/qmlcache/               compiled QML for everything Plasma draws
#   ~/.cache/kwin/                   compositor shader and effect cache
#
# A cache written by one version and read by the next is the usual failure. An
# upgrade normally bumps a version stamp, but partial upgrades, an interrupted
# write, or a package that forgets the bump leave a blob that is read as valid
# and is not.
#
# ksycoca is the specific one behind a wrong application menu: it is a compiled
# index of every .desktop file on the system, and if it is not rebuilt after a
# package change the menu describes a system you no longer have.
#
# WHAT THE FIX BUTTON DOES
#   1. Stops plasmashell.
#   2. Deletes the caches listed above.
#   3. Runs kbuildsycoca6 --noincremental to rebuild the service database from
#      scratch rather than patching the old one.
#   4. Starts plasmashell again.
#
# WHAT IT COSTS
# Nothing is configuration — every one of these is regenerated from data that
# still exists on disk. The next login is slower by a few seconds while the
# caches refill, and the first run of each Plasma effect may hitch once.
#
# IF THIS FIXES YOUR PROBLEM AND IT COMES BACK
# A cache that goes bad repeatedly usually means a failing disk or a filesystem
# that lost a write. Check `journalctl -b -p err` and the drive's SMART status.
)");
}

void MainWindow::on_kdeFixPlasmaCacheApplyButton_clicked() {
    QStringList present;
    for (const QString &t : kdeCacheTargets())
        if (QFileInfo::exists(t)) present << t;

    if (present.isEmpty()) {
        QMessageBox::information(this, tr("Clear Plasma Caches"),
                                 tr("None of the Plasma or Qt caches exist — nothing to clear."));
        return;
    }

    const qint64 bytes = kdePlasmaCacheBytes();
    const int ret = QMessageBox::question(
        this, tr("Clear Plasma and Qt Caches"),
        tr("Delete these %1 cache location(s) (%2 MB), rebuild the service database and "
           "restart plasmashell?\n\n%3\n\nNothing here is configuration — it is all "
           "regenerated. The next login is a few seconds slower.")
            .arg(present.size())
            .arg(bytes / (1024.0 * 1024.0), 0, 'f', 1)
            .arg(present.join("\n")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!kdeStopPlasmaShell()) {
        QMessageBox::critical(this, tr("Clear Plasma Caches"),
                              tr("plasmashell did not quit.\n\nNothing was changed — deleting "
                                 "caches underneath a running shell can crash it."));
        return;
    }

    QStringList failed;
    for (const QString &t : present) {
        const QFileInfo fi(t);
        const bool ok = fi.isDir() ? QDir(t).removeRecursively() : QFile::remove(t);
        if (!ok) failed << t;
    }

    if (QProcess::execute("kbuildsycoca6", QStringList() << "--noincremental") != 0)
        QProcess::execute("kbuildsycoca5", QStringList() << "--noincremental");

    kdeStartPlasmaShell();

    if (failed.isEmpty()) {
        QMessageBox::information(
            this, tr("Caches Cleared"),
            tr("Cleared %1 cache location(s) and rebuilt the service database.\n\n"
               "plasmashell is restarting. If icons or the application menu are still wrong "
               "afterwards, log out and back in — some caches are only re-read at session "
               "start.").arg(present.size()));
    } else {
        QMessageBox::warning(
            this, tr("Caches Partly Cleared"),
            tr("Cleared %1 of %2 location(s). These could not be removed:\n\n%3")
                .arg(present.size() - failed.size()).arg(present.size()).arg(failed.join("\n")));
    }

    QTimer::singleShot(4000, this, &MainWindow::refreshTweaksStatus);
}

// ── Issue 5: file-type icons the icon theme hides ────────────────────────────
//
// An application registers a custom file type (a MIME type with an <icon>) and
// drops its icon into ~/.local/share/icons/hicolor or /usr/share/icons/hicolor,
// exactly as the freedesktop spec says. Dolphin, the desktop and the file
// dialogs then show the file with the plain "generic document" glyph instead.
//
// This is not a cache problem and it is not random. KIconLoader looks up a
// mimetype icon like "application-x-lutris" theme by theme, and INSIDE each
// theme it also tries the dash-truncated fallbacks before moving on:
//
//     application-x-lutris → application-x-lutris → application → application-x-generic
//
// The active theme (Breeze, We10X, Tela, Papirus …) always ships
// application-x-generic, so the walk ends there — hicolor, which is always the
// LAST theme in the chain, is never reached. Any icon whose name starts with a
// media type (application-, text-, image-, video-, audio-, …) and lives only
// in hicolor is therefore invisible under every theme except hicolor itself.
// The icon appears to "come and go" only because a Plasma reset switches the
// theme (or a user copied the icon into one particular theme by hand).
//
// Two more traps make hand-fixing fail:
//   * Themes may declare KDE-Extensions=.svg in index.theme (Breeze does), so
//     KIconLoader will only ever look for .svg files inside them — copying a
//     .png into a theme directory does nothing.
//   * KIconLoader remembers the wrong (generic) resolution per process until it
//     receives the org.kde.KIconLoader.iconChanged D-Bus signal, so a fix that
//     just writes files does not show up in the running Dolphin or Plasma.
//
// The fix here is theme-side and app-agnostic: for every hidden icon it puts
// an .svg alias with the EXACT icon name into the mimetype directory of every
// theme in the active lookup chain (current theme → its parents → Breeze),
// under the user's own ~/.local/share/icons/<theme>/… overlay (KIconLoader
// merges theme directories across all XDG data dirs), so no system files are
// touched. Then it refreshes the caches and tells every running KDE app.


struct KdeThemeDirInfo {
    QString rel;      // "mimetypes/64", "mimes/scalable", "256x256/mimetypes"
    QString context;  // "MimeTypes", "Applications", ...
    QString type;     // "Fixed", "Scalable", "Threshold"
    int size = 0, minSize = 0, maxSize = 0, scale = 1;
};

struct KdeThemeInfo {
    QString name;
    QStringList roots;            // every <xdg icons dir>/<name> that exists, in search order
    QString indexPath;            // the index.theme KIconTheme would read
    QStringList inherits;
    QStringList kdeExtensions;    // KDE-Extensions or the KIconTheme default
    QList<KdeThemeDirInfo> dirs;  // Directories + ScaledDirectories that exist somewhere
    QSet<QString> icons;          // basenames present with an allowed extension in a listed dir
    bool valid = false;
};

struct KdeMimeIconIssue {
    QString name;         // icon name, e.g. application-x-lutris
    QString source;       // real file for it in hicolor (empty = none anywhere)
    QString resolved;     // what KIconLoader returns for it (kiconfinder), or predicted path
    QString hitTheme;     // theme in which the (wrong) hit was made
    QStringList refs;     // who expects it: "MIME application/x-lutris", "launcher foo.desktop"
    bool shadowed = false;   // a file exists but the theme walk stops on a different icon
    bool verified = false;   // 'resolved' came from kiconfinder, not from prediction
};

static const QStringList kKdeIconExts = {".png", ".svgz", ".svg", ".xpm"};

// XDG icon roots in KIconTheme order: ~/.local/share/icons first, then
// XDG_DATA_DIRS. ~/.icons is the legacy user location, still honoured.
static QStringList kdeIconRoots() {
    QStringList roots = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                  QStringLiteral("icons"),
                                                  QStandardPaths::LocateDirectory);
    const QString legacy = QDir::homePath() + "/.icons";
    if (QFileInfo(legacy).isDir() && !roots.contains(legacy)) roots.prepend(legacy);
    return roots;
}

static QString kdeUserIconRoot() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons";
}

static QString kdeStripIconExt(const QString &fileName) {
    for (const QString &e : kKdeIconExts)
        if (fileName.endsWith(e)) return fileName.left(fileName.size() - e.size());
    return fileName;
}

// The icon theme Plasma is using — same source KIconTheme::current() reads.
static QString kdeCurrentIconTheme() {
    const auto groups = kdeParseAppletsRc(QDir::homePath() + "/.config/kdeglobals");
    for (const auto &g : groups)
        if (g.first == "[Icons]" && g.second.contains("Theme") && !g.second.value("Theme").isEmpty())
            return g.second.value("Theme");
    return QStringLiteral("breeze");
}

static KdeThemeInfo kdeReadTheme(const QString &name) {
    KdeThemeInfo t;
    t.name = name;
    for (const QString &root : kdeIconRoots()) {
        const QString dir = root + "/" + name;
        if (!QFileInfo(dir).isDir()) continue;
        t.roots << dir;
        if (t.indexPath.isEmpty() && QFileInfo::exists(dir + "/index.theme"))
            t.indexPath = dir + "/index.theme";
    }
    if (t.indexPath.isEmpty()) return t;   // KIconTheme::isValid() == false

    QStringList dirNames;
    QMap<QString, QMap<QString, QString>> sections;
    for (const auto &g : kdeParseAppletsRc(t.indexPath)) {
        QString key = g.first;
        if (key.startsWith('[') && key.endsWith(']')) key = key.mid(1, key.size() - 2);
        sections.insert(key, g.second);
    }
    const auto head = sections.value("Icon Theme");
    auto splitList = [](const QString &s) {
        QStringList out;
        for (const QString &p : s.split(',', Qt::SkipEmptyParts)) {
            const QString v = p.trimmed();
            if (!v.isEmpty()) out << v;
        }
        return out;
    };
    t.inherits = splitList(head.value("Inherits"));
    t.kdeExtensions = splitList(head.value("KDE-Extensions"));
    if (t.kdeExtensions.isEmpty()) t.kdeExtensions = kKdeIconExts;
    dirNames = splitList(head.value("Directories")) + splitList(head.value("ScaledDirectories"));

    for (const QString &rel : dirNames) {
        const auto sec = sections.value(rel);
        KdeThemeDirInfo d;
        d.rel = rel;
        d.context = sec.value("Context");
        d.type = sec.value("Type", "Threshold");
        d.size = sec.value("Size").toInt();
        d.scale = sec.value("Scale", "1").toInt();
        d.minSize = sec.value("MinSize", QString::number(d.size)).toInt();
        d.maxSize = sec.value("MaxSize", QString::number(d.size)).toInt();
        if (d.size == 0) continue;   // KIconThemeDir treats Size=0 as invalid
        t.dirs << d;
        for (const QString &root : t.roots) {
            QDir qd(root + "/" + rel);
            if (!qd.exists()) continue;
            for (const QString &f : qd.entryList(QDir::Files | QDir::System)) {  // System: dangling symlinks
                for (const QString &e : t.kdeExtensions)
                    if (f.endsWith(e)) { t.icons.insert(f.left(f.size() - e.size())); break; }
            }
        }
    }
    t.valid = true;
    return t;
}

// KIconLoaderPrivate::addBaseThemes(): current theme, its inherited themes
// (depth first, hicolor skipped), then the Qt fallback theme (Breeze under
// Plasma), then hicolor last. Returned WITHOUT hicolor — callers treat it apart.
static QList<KdeThemeInfo> kdeIconThemeChain(const QString &current) {
    QList<KdeThemeInfo> chain;
    QSet<QString> seen;
    std::function<void(const QString &)> add = [&](const QString &name) {
        if (name == "hicolor" || seen.contains(name)) return;
        seen.insert(name);
        KdeThemeInfo t = kdeReadTheme(name);
        if (!t.valid) return;
        chain << t;
        for (const QString &parent : t.inherits) add(parent);
    };
    add(current);
    if (chain.isEmpty()) add("breeze");   // KIconTheme::defaultThemeName()
    add("breeze");                        // QIcon::fallbackThemeName() under Plasma
    return chain;
}

// Replicates KIconLoaderPrivate::findMatchingIcon() for ONE theme: returns the
// icon name the walk stops on inside this theme (the exact name, a truncated
// prefix, or the <media>-x-generic icon), or empty if this theme yields nothing.
static QString kdeThemeWalk(const KdeThemeInfo &t, const QString &name) {
    static const QSet<QString> mediaTypes = {"text", "application", "image", "audio", "inode", "video",
                                             "message", "model", "multipart", "x-content", "x-epoc"};
    bool genericFallback = name.endsWith("-x-generic");
    QString cur = name;
    while (!cur.isEmpty()) {
        if (t.icons.contains(cur)) return cur;
        if (genericFallback) break;
        const int rindex = cur.lastIndexOf('-');
        if (rindex > 1) {
            cur.truncate(rindex);
            if (cur.endsWith("-x")) cur.chop(2);
        } else if (mediaTypes.contains(cur)) {
            cur += "-x-generic";
            genericFallback = true;
        } else {
            break;
        }
    }
    return QString();
}

// Best real file for an icon name inside hicolor (all roots): prefer scalable
// SVG, else the largest raster. Empty if hicolor has nothing under that name.
static QString kdeFindHicolorSource(const QString &name) {
    QString best;
    int bestScore = -1;
    for (const QString &root : kdeIconRoots()) {
        const QString hi = root + "/hicolor";
        if (!QFileInfo(hi).isDir()) continue;
        QDirIterator it(hi, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QString f = it.fileName();
            if (kdeStripIconExt(f) != name || f == name) continue;
            int score = 0;
            if (f.endsWith(".svg") || f.endsWith(".svgz")) score = 100000;
            else {
                static const QRegularExpression sz(QStringLiteral("(\\d+)x\\d+"));
                const auto m = sz.match(it.filePath());
                score = m.hasMatch() ? m.captured(1).toInt() : 1;
            }
            if (score > bestScore) { bestScore = score; best = it.filePath(); }
        }
    }
    return best;
}

// Ground truth: ask KIconLoader itself. Returns the resolved path, empty when
// nothing resolves. ok=false when no kiconfinder binary exists on this system.
static QString kdeKIconFinder(const QString &name, bool *ok = nullptr) {
    static QString tool;
    static bool probed = false;
    if (!probed) {
        probed = true;
        for (const QString &c : {QStringLiteral("kiconfinder6"), QStringLiteral("kiconfinder5"), QStringLiteral("kiconfinder")})
            if (!QStandardPaths::findExecutable(c).isEmpty()) { tool = c; break; }
    }
    if (ok) *ok = !tool.isEmpty();
    if (tool.isEmpty()) return QString();
    QProcess p;
    p.start(tool, QStringList() << name);
    if (!p.waitForFinished(4000)) { p.kill(); return QString(); }
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

// Every icon name something on this machine expects to exist as a FILE-TYPE
// icon: what hicolor ships in its mimetypes dirs (user + system), plus what the
// shared-mime-info databases register as <icon>, plus what the user's own
// launchers reference. Value = human-readable list of who references it.
static QMap<QString, QStringList> kdeCollectIconCandidates(bool includeLaunchers,
                                                            QSet<QString> *referenced = nullptr) {
    QMap<QString, QStringList> out;
    QMap<QString, int> hicolorHits;
    for (const QString &root : kdeIconRoots()) {
        const QString hi = root + "/hicolor";
        if (!QFileInfo(hi).isDir()) continue;
        QDirIterator it(hi, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            if (!it.filePath().contains("/mimetypes/")) continue;
            const QString n = kdeStripIconExt(it.fileName());
            if (n == it.fileName()) continue;
            hicolorHits[n]++;
        }
    }
    for (auto it = hicolorHits.constBegin(); it != hicolorHits.constEnd(); ++it)
        out[it.key()] << QObject::tr("%n file(s) in hicolor", nullptr, it.value());
    const QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &d : dataDirs) {
        QFile f(d + "/mime/icons");
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            const int c = line.indexOf(':');
            if (c <= 0) continue;
            const QString icon = line.mid(c + 1).trimmed();
            if (icon.isEmpty() || icon.startsWith('/')) continue;
            out[icon] << QObject::tr("MIME type %1").arg(line.left(c));
            if (referenced) referenced->insert(icon);
        }
    }
    if (includeLaunchers) {
        const QString apps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
        QDirIterator it(apps, QStringList() << "*.desktop", QDir::Files);
        while (it.hasNext()) {
            it.next();
            for (const auto &g : kdeParseAppletsRc(it.filePath())) {
                if (g.first != "[Desktop Entry]") continue;
                const QString icon = g.second.value("Icon").trimmed();
                if (icon.isEmpty() || icon.startsWith('/')) break;
                out[icon] << QObject::tr("launcher %1").arg(it.fileName());
                if (referenced) referenced->insert(icon);
                break;
            }
        }
    }
    return out;
}

// Predicts what KIconLoader resolves 'name' to, theme by theme. Fills hitTheme
// and returns the icon NAME the walk stops on (empty = nothing at all).
static QString kdePredictResolution(const QList<KdeThemeInfo> &chain, const KdeThemeInfo &hicolor,
                                    const QString &name, QString *hitTheme) {
    for (const KdeThemeInfo &t : chain) {
        const QString hit = kdeThemeWalk(t, name);
        if (!hit.isEmpty()) { if (hitTheme) *hitTheme = t.name; return hit; }
    }
    if (hicolor.valid) {
        const QString hit = kdeThemeWalk(hicolor, name);
        if (!hit.isEmpty()) { if (hitTheme) *hitTheme = "hicolor"; return hit; }
    }
    // KIconLoader::iconPath() last resort: unthemed <icons dir>/<name>.ext and pixmaps
    QStringList flat = kdeIconRoots();
    flat += QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "pixmaps", QStandardPaths::LocateDirectory);
    for (const QString &d : flat)
        for (const QString &e : kKdeIconExts)
            if (QFileInfo::exists(d + "/" + name + e)) { if (hitTheme) *hitTheme = d; return name; }
    return QString();
}

static const char *kKdeMimeFixMarker = "<!-- cachyostools-mime-icon-fix -->";

// True if 'path' is an alias this tool wrote: a REGULAR file (never a symlink —
// icon themes are built from thousands of symlinks and none of them are ours)
// whose head carries the marker comment.
static bool kdeIsOurAlias(const QString &path) {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (fi.isSymLink() || !fi.isFile()) return false;
    QFile f(path);
    return f.open(QIODevice::ReadOnly) && f.read(512).contains(kKdeMimeFixMarker);
}

// The scan. verify=true additionally asks kiconfinder for every candidate
// (~40 ms each) and trusts its answer over the prediction.
static QList<KdeMimeIconIssue> kdeScanMimeIcons(bool verify, bool *finderAvailable = nullptr) {
    QList<KdeMimeIconIssue> issues;
    const QString current = kdeCurrentIconTheme();
    const QList<KdeThemeInfo> chain = kdeIconThemeChain(current);
    const KdeThemeInfo hicolor = kdeReadTheme("hicolor");
    QSet<QString> referencedNames;
    const QMap<QString, QStringList> candidates = kdeCollectIconCandidates(true, &referencedNames);
    bool finderOk = false;
    if (verify) kdeKIconFinder(QStringLiteral("unknown"), &finderOk);
    if (finderAvailable) *finderAvailable = finderOk;

    for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
        KdeMimeIconIssue is;
        is.name = it.key();
        is.refs = it.value();
        is.refs.removeDuplicates();
        is.source = kdeFindHicolorSource(is.name);

        QString hitName = kdePredictResolution(chain, hicolor, is.name, &is.hitTheme);
        if (verify && finderOk) {
            const QString path = kdeKIconFinder(is.name);
            is.verified = true;
            is.resolved = path;
            hitName = path.isEmpty() ? QString() : kdeStripIconExt(QFileInfo(path).fileName());
            if (!path.isEmpty()) {
                is.hitTheme.clear();
                for (const KdeThemeInfo &t : chain)
                    for (const QString &r : t.roots)
                        if (path.startsWith(r + "/")) is.hitTheme = t.name;
                if (is.hitTheme.isEmpty() && path.contains("/hicolor/")) is.hitTheme = "hicolor";
            }
        } else if (!hitName.isEmpty()) {
            is.resolved = hitName;
        }

        if (hitName == is.name) {
            // Resolves — but if only through an alias of ours in the top theme
            // while a fallback theme in the chain (Breeze after a reset, say)
            // still lacks it, the fix should propagate the alias there too.
            const QString via = is.verified ? is.resolved : QString();
            bool gap = false;
            if (kdeIsOurAlias(via) && !is.source.isEmpty())
                for (const KdeThemeInfo &t : chain)
                    if (!t.icons.contains(is.name) && !kdeThemeWalk(t, is.name).isEmpty()) gap = true;
            if (!gap) continue;
            is.shadowed = true;
            is.refs << QObject::tr("protected in %1 only").arg(is.hitTheme);
            issues << is;
            continue;
        }
        is.shadowed = !hitName.isEmpty() && !is.source.isEmpty();
        // Not shadowed and no file of its own: only worth reporting if a MIME
        // type or a launcher actually asks for it.
        if (!is.shadowed && !referencedNames.contains(is.name)) continue;
        issues << is;
    }
    return issues;
}

// The icon-theme Context an icon belongs to, read off the folder hicolor keeps
// it in ("…/mimetypes/x.png" → MimeTypes, "…/apps/x.png" → Applications).
static QString kdeContextForSource(const QString &source) {
    static const QMap<QString, QString> map = {
        {"mimetypes", "MimeTypes"}, {"apps", "Applications"}, {"actions", "Actions"},
        {"places", "Places"}, {"devices", "Devices"}, {"categories", "Categories"},
        {"status", "Status"}, {"emblems", "Emblems"}, {"animations", "Animations"},
        {"emotes", "Emotes"}, {"intl", "International"},
    };
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (source.contains("/" + it.key() + "/")) return it.value();
    return QStringLiteral("MimeTypes");
}

// Chooses the directory of a theme to overlay an icon into for a Context: a
// Scalable dir with the widest range, else the largest one; symbolic dirs are
// never used. Empty if the theme has no directory for that Context.
static QString kdePickThemeDir(const KdeThemeInfo &t, const QString &context) {
    // Conventional folder for the context ("apps/…" rather than Breeze's
    // equally valid "preferences/…"), preferred when several qualify.
    static const QMap<QString, QString> conventional = {
        {"MimeTypes", "mime"}, {"Applications", "apps"}, {"Actions", "actions"}, {"Places", "places"},
        {"Devices", "devices"}, {"Categories", "categories"}, {"Status", "status"}, {"Emblems", "emblems"},
    };
    QString best;
    int bestScore = -1;
    for (const KdeThemeDirInfo &d : t.dirs) {
        if (d.context != context || d.scale != 1) continue;
        if (d.rel.contains("symbolic", Qt::CaseInsensitive)) continue;
        int score = (d.type == "Scalable") ? 100000 + (d.maxSize - d.minSize) : d.size;
        const QString folder = d.rel.section('/', 0, 0);
        if (folder.startsWith(conventional.value(context, "\x01"), Qt::CaseInsensitive)) score += 50000;
        if (score > bestScore) { bestScore = score; best = d.rel; }
    }
    return best;
}

// Writes <name>.svg into <user root>/<theme>/<dir>/ that shows 'source'.
// Always a regular file carrying the marker (never a symlink): plain SVG
// sources are copied with the marker prepended, .svgz is inflated first,
// rasters are re-encoded (≤256 px) and embedded — so the alias is valid under
// KDE-Extensions=.svg themes too. Never overwrites anything that is not ours.
static bool kdeWriteThemeAlias(const QString &theme, const QString &dirRel, const QString &name,
                               const QString &source, QString *outPath, QString *err) {
    const QString dir = kdeUserIconRoot() + "/" + theme + "/" + dirRel;
    if (!QDir().mkpath(dir)) { *err = QObject::tr("cannot create %1").arg(dir); return false; }
    const QString path = dir + "/" + name + ".svg";
    if (outPath) *outPath = path;

    const QFileInfo fi(path);
    if (fi.exists() || fi.isSymLink()) {
        if (!kdeIsOurAlias(path)) { *err = QObject::tr("%1 exists and is not ours — left alone").arg(path); return false; }
        QFile::remove(path);
    }

    QByteArray body;
    if (source.endsWith(".svg") || source.endsWith(".svgz")) {
        if (source.endsWith(".svgz")) {
            QProcess gz;
            gz.start("gzip", QStringList() << "-dc" << source);
            if (gz.waitForFinished(10000) && gz.exitCode() == 0) body = gz.readAllStandardOutput();
        } else {
            QFile f(source);
            if (f.open(QIODevice::ReadOnly)) body = f.readAll();
        }
        if (!body.contains("<svg")) body.clear();   // not usable as text — fall through to raster
        else {
            // Put the marker after the XML declaration if there is one.
            const int decl = body.indexOf("?>");
            const int at = (body.startsWith("<?xml") && decl > 0) ? decl + 2 : 0;
            body.insert(at, QByteArray("\n") + kKdeMimeFixMarker + "\n");
        }
    }
    if (body.isEmpty()) {
        QImage img(source);
        if (img.isNull()) { *err = QObject::tr("cannot decode %1").arg(source); return false; }
        if (img.width() > 256 || img.height() > 256)
            img = img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QByteArray png;
        QBuffer buf(&png);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        body = QString(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n%1\n"
            "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
            "width=\"%2\" height=\"%3\" viewBox=\"0 0 %2 %3\">\n"
            "  <image width=\"%2\" height=\"%3\" xlink:href=\"data:image/png;base64,%4\"/>\n"
            "</svg>\n").arg(kKdeMimeFixMarker).arg(img.width()).arg(img.height())
                        .arg(QString::fromLatin1(png.toBase64())).toUtf8();
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { *err = QObject::tr("cannot write %1").arg(path); return false; }
    f.write(body);
    f.close();
    return true;
}

// Removes aliases this tool wrote earlier that no longer serve anything: the
// hicolor source vanished (the app was updated or removed) or nothing on the
// system references the name any more. Only marker files are candidates —
// never symlinks, never anything else — and as a last line of defence the
// pass refuses to act at all if it would remove an implausible number.
static QStringList kdePruneOurAliases(const QList<KdeThemeInfo> &chain, const QSet<QString> &wanted) {
    QStringList doomed;
    const QString userRoot = kdeUserIconRoot();
    for (const KdeThemeInfo &t : chain) {
        for (const KdeThemeDirInfo &d : t.dirs) {
            QDir qd(userRoot + "/" + t.name + "/" + d.rel);
            if (!qd.exists()) continue;
            for (const QString &f : qd.entryList(QStringList() << "*.svg", QDir::Files | QDir::NoSymLinks)) {
                const QString path = qd.filePath(f);
                if (!kdeIsOurAlias(path)) continue;
                const QString name = kdeStripIconExt(f);
                if (wanted.contains(name) && !kdeFindHicolorSource(name).isEmpty()) continue;
                doomed << path;
            }
        }
    }
    if (doomed.size() > 50) return {};   // something is off — leave everything alone
    QStringList removed;
    for (const QString &p : doomed)
        if (kdeIsOurAlias(p) && QFile::remove(p)) removed << p;
    return removed;
}

// What the icons KCM does after a theme change: refresh the on-disk caches
// that exist, then tell every running KIconLoader to drop what it remembers.
static QStringList kdeRefreshIconLookup(const QStringList &touchedThemes) {
    QStringList notes;
    const QString userRoot = kdeUserIconRoot();
    for (const QString &t : touchedThemes) {
        const QString dir = userRoot + "/" + t;
        if (!QFileInfo(dir).isDir()) continue;
        // Bumping the theme root mtime is what QIconLoader / GTK check before trusting a cache.
        QFile stamp(dir + "/.cachyostools-touch");
        if (stamp.open(QIODevice::WriteOnly)) { stamp.close(); QFile::remove(stamp.fileName()); }
        const QString cache = dir + "/icon-theme.cache";
        if (QFileInfo::exists(cache)) {
            if (QProcess::execute("gtk-update-icon-cache", QStringList() << "-f" << "-t" << "-q" << dir) != 0) {
                QFile::remove(cache);
                notes << QObject::tr("removed stale %1").arg(cache);
            } else {
                notes << QObject::tr("refreshed %1").arg(cache);
            }
        }
    }
    const QString mime = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/mime";
    if (QFileInfo(mime + "/packages").isDir())
        QProcess::execute("update-mime-database", QStringList() << mime);
    const QString apps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (QFileInfo(apps).isDir())
        QProcess::execute("update-desktop-database", QStringList() << "-q" << apps);
    // KF5 kept an on-disk icon cache; KF6 no longer does, but it costs nothing.
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/icon-cache.kcache");
    // KIconLoader::emitChange() for every group — every KDE app re-resolves icons.
    bool signalled = false;
    if (!QStandardPaths::findExecutable("dbus-send").isEmpty()) {
        signalled = true;
        for (int g = 0; g < 6; ++g)
            if (QProcess::execute("dbus-send", QStringList() << "--session" << "--type=signal" << "/KIconLoader"
                                  << "org.kde.KIconLoader.iconChanged" << QString("int32:%1").arg(g)) != 0)
                signalled = false;
    }
    notes << (signalled ? QObject::tr("notified running KDE applications (org.kde.KIconLoader.iconChanged)")
                        : QObject::tr("could not send org.kde.KIconLoader.iconChanged — restart Dolphin/Plasma to see the change"));
    if (QProcess::execute("kbuildsycoca6", QStringList()) != 0)
        QProcess::execute("kbuildsycoca5", QStringList());
    return notes;
}

void MainWindow::checkkdeFixMimeIconsState() {
    // Prediction only — no processes — so the tab opens instantly.
    const auto issues = kdeScanMimeIcons(false);
    int shadowed = 0;
    for (const auto &i : issues) if (i.shadowed) ++shadowed;
    if (shadowed > 0)
        updateTweakStatusLabel(ui->kdeFixMimeIconsStatusLabel, tr("%1 hidden").arg(shadowed), false);
    else if (!issues.isEmpty())
        updateTweakStatusLabel(ui->kdeFixMimeIconsStatusLabel, tr("%1 missing").arg(issues.size()), false);
    else
        updateTweakStatusLabel(ui->kdeFixMimeIconsStatusLabel, tr("OK"), true);
}

QStringList MainWindow::kdeScanMimeIconIssues() const {
    QStringList out;
    for (const auto &i : kdeScanMimeIcons(true)) {
        if (i.shadowed)
            out << tr("%1 — hidden by %2 (KDE shows %3 instead); file: %4; used by: %5")
                       .arg(i.name, i.hitTheme.isEmpty() ? tr("the icon theme") : i.hitTheme,
                            i.resolved.isEmpty() ? tr("a generic icon") : QFileInfo(i.resolved).fileName(),
                            i.source, i.refs.join(", "));
        else if (!i.resolved.isEmpty())
            out << tr("%1 — has no icon file of its own (KDE shows %2); used by: %3")
                       .arg(i.name, QFileInfo(i.resolved).fileName(), i.refs.join(", "));
        else
            out << tr("%1 — no icon file found anywhere; used by: %2").arg(i.name, i.refs.join(", "));
    }
    return out;
}

void MainWindow::on_kdeFixMimeIconsToggle_clicked() {
    showTweakInstructions("File-Type Icons Hidden by the Icon Theme",
R"(# File-type icons hidden by the icon theme
# ========================================
#
# THE SYMPTOMS
#   * A file type an application registered (a project file, a save game, a
#     custom document) shows the plain "generic document" glyph in Dolphin, on
#     the desktop and in file dialogs — even though the app installed an icon
#     and the association itself works (double-click opens the right program).
#   * The icon was there once and disappeared after a Plasma reset, a theme
#     change, or a login. Or it shows in one icon theme and not in another.
#   * A launcher in ~/.local/share/applications has Icon= pointing at a name
#     that no theme provides.
#
# WHY IT HAPPENS
# Applications install file-type icons into the "hicolor" theme, which the
# freedesktop spec defines as the fallback every other theme inherits. That is
# correct — and KDE breaks it in a very specific way.
#
# KIconLoader resolves an icon theme by theme: your theme first, then the
# themes it inherits, then Breeze, then hicolor LAST. But inside each theme it
# also tries the "generic" fallbacks before moving to the next theme:
#
#     application-x-lutris  →  application-x-lutris   (not in theme)
#                           →  application            (not in theme)
#                           →  application-x-generic  (every theme has this!)
#
# The walk stops on the theme's generic-document icon and hicolor is never
# consulted. Any file-type icon whose name begins with a media type
# (application-, text-, image-, video-, audio-, …) and lives only in hicolor
# is invisible under every theme except hicolor itself. Lutris, Linux Studio,
# emerald themes — anything installed the standard way — are all affected.
#
# It looks intermittent because a Plasma reset switches the theme back to
# Breeze, or because someone copied the icon into one particular theme by
# hand and then changed theme.
#
# Two extra traps defeat manual fixes:
#   * Themes may set KDE-Extensions=.svg in index.theme (Breeze does). KDE then
#     only looks for .svg files inside that theme — copying a .png in does nothing.
#   * Every running KDE program remembers the wrong answer until it receives
#     the org.kde.KIconLoader.iconChanged D-Bus signal. Writing files alone
#     changes nothing on screen.
#
# WHAT THE SCAN CHECKS
#   Every icon that something on this system expects as a file-type icon:
#     - all icons in the mimetypes folders of hicolor (user and system)
#     - every <icon> registered in the shared-mime-info databases
#     - Icon= of every launcher in ~/.local/share/applications
#   For each one it asks KDE's own resolver (kiconfinder6) what it returns.
#   "Hidden" = a real icon file exists but KDE resolves the name to a different
#   icon (the same truncation also bites launcher icons: "web-browser-test"
#   becomes the theme's "web-browser"). "Missing" = nothing provides the icon at all (reported, not fixable
#   here — the application has to ship one).
#
# WHAT THE FIX BUTTON DOES
#   1. For every hidden icon, writes an .svg alias with the exact icon name into
#      the matching folder (mimetypes — or apps, for a hidden launcher icon) of
#      every theme in your active lookup chain (your theme, its parents,
#      Breeze), inside your own overlay
#      ~/.local/share/icons/<theme>/… — KDE merges theme folders across all XDG
#      data dirs, so no system file is touched and no theme package is edited.
#      SVG sources are copied, PNG sources are embedded (≤256 px), so the alias
#      is valid under KDE-Extensions=.svg themes. Every alias is a plain file
#      carrying a marker comment; the tool never creates or touches symlinks.
#   2. Refreshes the caches: icon-theme.cache of touched user themes,
#      update-mime-database, update-desktop-database, kbuildsycoca6.
#   3. Sends org.kde.KIconLoader.iconChanged so Dolphin, Plasma and every other
#      running KDE app re-resolve their icons immediately (press F5 in an open
#      Dolphin view if it does not repaint by itself).
#
# WHAT IT COSTS
#   A handful of small files under ~/.local/share/icons. If you switch to a theme
#   that is not in today's chain, run the fix again — the status turns red.
#   Aliases are marked so re-running replaces only what this tool wrote.
#
# THE RIGHT FIX FOR APPLICATION DEVELOPERS
#   Give the file-type icon a name KDE cannot truncate into a generic one:
#   a vendor-prefixed name with no media-type prefix, e.g.
#       <icon name="linux-studio-2026-project"/>    instead of application-x-…
#   installed as PNG into hicolor/<size>x<size>/mimetypes/. Then it resolves under
#   every theme, first try, with no per-theme alias.
)");
}

void MainWindow::on_kdeFixMimeIconsApplyButton_clicked() {
    bool finderOk = false;
    const auto issues = kdeScanMimeIcons(true, &finderOk);
    QList<KdeMimeIconIssue> hidden, missing;
    for (const auto &i : issues) (i.shadowed ? hidden : missing) << i;

    if (hidden.isEmpty()) {
        QSet<QString> wanted;
        const auto allCandidates = kdeCollectIconCandidates(true);
        for (auto it = allCandidates.constBegin(); it != allCandidates.constEnd(); ++it) wanted.insert(it.key());
        const QStringList pruned = kdePruneOurAliases(kdeIconThemeChain(kdeCurrentIconTheme()), wanted);
        QString msg = tr("No file-type icon is hidden by the current icon theme.");
        if (!pruned.isEmpty())
            msg += tr("\n\nRemoved %1 alias file(s) written earlier that nothing references any more.").arg(pruned.size());
        if (!missing.isEmpty()) {
            QStringList m;
            for (const auto &i : missing) m << QString("  • %1 (%2)").arg(i.name, i.refs.join(", "));
            msg += tr("\n\n%1 icon name(s) are referenced but no theme provides a file for them — "
                      "the owning application has to ship one:\n\n%2").arg(missing.size()).arg(m.join("\n"));
        }
        if (!finderOk)
            msg += tr("\n\nNote: kiconfinder6 is not installed, so this is a prediction rather than "
                      "KDE's own answer.");
        QMessageBox::information(this, tr("File-Type Icons"), msg);
        return;
    }

    const QString current = kdeCurrentIconTheme();
    const QList<KdeThemeInfo> chain = kdeIconThemeChain(current);
    QStringList chainNames;
    for (const auto &t : chain) chainNames << t.name;

    QStringList lines;
    for (const auto &i : hidden)
        lines << QString("  • %1 → shows %2").arg(i.name, i.resolved.isEmpty() ? tr("generic")
                                                                            : QFileInfo(i.resolved).fileName());
    const int ret = QMessageBox::question(
        this, tr("Fix Hidden File-Type Icons"),
        tr("%1 file-type icon(s) exist on disk but are hidden by the icon theme:\n\n%2\n\n"
           "Write .svg aliases for them into your overlay of these themes:\n  %3\n"
           "(under %4), refresh the icon caches and notify running KDE applications?")
            .arg(hidden.size()).arg(lines.join("\n")).arg(chainNames.join(", ")).arg(kdeUserIconRoot()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (ret != QMessageBox::Yes) return;

    QStringList written, skipped;
    QSet<QString> touchedThemes;
    for (const auto &i : hidden) {
        for (const KdeThemeInfo &t : chain) {
            if (t.icons.contains(i.name)) continue;          // this theme already has the real name
            const QString context = kdeContextForSource(i.source);
            const QString dirRel = kdePickThemeDir(t, context);
            if (dirRel.isEmpty()) { skipped << tr("%1: theme %2 has no %3 folder").arg(i.name, t.name, context); continue; }
            QString path, err;
            if (kdeWriteThemeAlias(t.name, dirRel, i.name, i.source, &path, &err)) {
                written << path;
                touchedThemes.insert(t.name);
            } else {
                skipped << QString("%1: %2").arg(i.name, err);
            }
        }
    }

    QSet<QString> wanted;
    const auto allCandidates = kdeCollectIconCandidates(true);
    for (auto it = allCandidates.constBegin(); it != allCandidates.constEnd(); ++it) wanted.insert(it.key());
    const QStringList pruned = kdePruneOurAliases(chain, wanted);
    for (const QString &p : pruned)
        for (const KdeThemeInfo &t : chain)
            if (p.startsWith(kdeUserIconRoot() + "/" + t.name + "/")) touchedThemes.insert(t.name);

    const QStringList notes = kdeRefreshIconLookup(touchedThemes.values());

    // Verify with KDE's resolver now that the aliases are in place.
    QStringList stillHidden;
    if (finderOk) {
        for (const auto &i : hidden) {
            const QString p = kdeKIconFinder(i.name);
            if (kdeStripIconExt(QFileInfo(p).fileName()) != i.name)
                stillHidden << QString("  • %1 → %2").arg(i.name, p.isEmpty() ? tr("nothing") : p);
        }
    }

    QString report = tr("Wrote %1 alias file(s) for %2 icon(s).\n").arg(written.size()).arg(hidden.size());
    if (!written.isEmpty()) report += "\n" + written.join("\n") + "\n";
    if (!skipped.isEmpty()) report += tr("\nSkipped:\n%1\n").arg(skipped.join("\n"));
    if (!pruned.isEmpty()) report += tr("\nRemoved %1 alias file(s) that nothing references any more.\n").arg(pruned.size());
    report += "\n" + notes.join("\n") + "\n";
    if (finderOk) {
        report += stillHidden.isEmpty()
            ? tr("\nVerified: KDE now resolves every one of them to its own icon.")
            : tr("\nStill not resolving correctly:\n%1").arg(stillHidden.join("\n"));
    }
    if (!missing.isEmpty()) {
        QStringList m;
        for (const auto &i : missing) m << QString("  • %1 (%2)").arg(i.name, i.refs.join(", "));
        report += tr("\n\nNot fixable here — referenced but no file exists in any theme:\n%1").arg(m.join("\n"));
    }
    report += tr("\n\nOpen Dolphin views may need F5 to repaint.");

    if (stillHidden.isEmpty())
        QMessageBox::information(this, tr("File-Type Icons Fixed"), report);
    else
        QMessageBox::warning(this, tr("File-Type Icons Partly Fixed"), report);

    QTimer::singleShot(1500, this, &MainWindow::refreshTweaksStatus);
}

// ── Section actions ──────────────────────────────────────────────────────────

void MainWindow::on_kdeRescanIssuesButton_clicked() {
    refreshKdeTweaksStatus();

    QStringList report;
    const QStringList drag = kdeScanFolderDragIssues();
    const QStringList bad  = kdeScanConfigCorruption();
    const int locked = kdeWidgetsLockedState();
    const QStringList icons = kdeScanMimeIconIssues();

    if (!drag.isEmpty())
        report << tr("Folder View drag and drop:\n  • %1").arg(drag.join("\n  • "));
    if (locked == 1)
        report << tr("Plasma widgets are locked.");
    if (!bad.isEmpty())
        report << tr("Corrupt groups in desktop-appletsrc:\n  • %1").arg(bad.join("\n  • "));
    if (!icons.isEmpty())
        report << tr("File-type icons hidden by the icon theme or missing:\n  • %1").arg(icons.join("\n  • "));

    if (report.isEmpty()) {
        QMessageBox::information(this, tr("Scan Complete"),
                                 tr("No known issues detected in this Plasma session."));
    } else {
        QMessageBox::warning(this, tr("Scan Complete"),
                             tr("Found %1 issue area(s):\n\n%2\n\nUse the Info button on each "
                                "row for what it means, and Fix to repair it.")
                                 .arg(report.size()).arg(report.join("\n\n")));
    }
}

void MainWindow::on_kdeRestartShellButton_clicked() {
    const int ret = QMessageBox::question(
        this, tr("Restart Plasma Shell"),
        tr("Quit and restart plasmashell?\n\nThe desktop and panels disappear for a few "
           "seconds. Open windows are not affected."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!kdeStopPlasmaShell()) {
        QMessageBox::warning(this, tr("Restart Plasma Shell"),
                             tr("plasmashell did not quit within 10 seconds. Nothing was done."));
        return;
    }
    kdeStartPlasmaShell();
    QTimer::singleShot(4000, this, &MainWindow::refreshTweaksStatus);
}

void MainWindow::on_kdeRestartKwinButton_clicked() {
    const bool wayland = qgetenv("XDG_SESSION_TYPE").toLower() == "wayland";
    if (wayland) {
        QMessageBox::information(
            this, tr("Restart KWin"),
            tr("On Wayland, KWin is the display server — restarting it would end your session "
               "and close every window.\n\nUse Desktop Effects in System Settings to toggle an "
               "individual effect, or log out and back in."));
        return;
    }

    const int ret = QMessageBox::question(
        this, tr("Restart KWin"),
        tr("Replace the running KWin compositor?\n\nWindows stay open but lose their "
           "decorations for a moment. This clears effect and compositing glitches without "
           "logging out."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    if (!QProcess::startDetached("kwin_x11", QStringList() << "--replace"))
        QProcess::startDetached("kwin", QStringList() << "--replace");

    QMessageBox::information(this, tr("Restart KWin"),
                             tr("KWin is being replaced. Give it a couple of seconds."));
}
