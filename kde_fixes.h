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

// ── Section actions ──────────────────────────────────────────────────────────

void MainWindow::on_kdeRescanIssuesButton_clicked() {
    refreshKdeTweaksStatus();

    QStringList report;
    const QStringList drag = kdeScanFolderDragIssues();
    const QStringList bad  = kdeScanConfigCorruption();
    const int locked = kdeWidgetsLockedState();

    if (!drag.isEmpty())
        report << tr("Folder View drag and drop:\n  • %1").arg(drag.join("\n  • "));
    if (locked == 1)
        report << tr("Plasma widgets are locked.");
    if (!bad.isEmpty())
        report << tr("Corrupt groups in desktop-appletsrc:\n  • %1").arg(bad.join("\n  • "));

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
