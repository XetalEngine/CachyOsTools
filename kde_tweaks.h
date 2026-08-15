// KDE Plasma tweaks — KWin compositor pacing and Qt cache placement.
//
// Everything in here is PER-USER and needs no root. Plasma sources every *.sh in
// ~/.config/plasma-workspace/env/ at session start, before kwin and plasmashell
// launch. That is the only point early enough for the driver and Qt to see these
// variables — a shell rc is too late, because neither process is started from a
// login shell.
//
// One file per option, so each row here is independently enabled, removed and
// inspected. Resetting Plasma empties that directory, which is how a working set
// of these silently disappears.

#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>

// ── Helpers ──────────────────────────────────────────────────────────────────

QString MainWindow::kdeEnvDir() const {
    return QDir::homePath() + "/.config/plasma-workspace/env";
}

QString MainWindow::kdeEnvFile(const QString &basename) const {
    return kdeEnvDir() + "/" + basename;
}

bool MainWindow::kdeEnvEnabled(const QString &basename) const {
    return QFileInfo::exists(kdeEnvFile(basename));
}

// Writes the snippet and makes it executable. Plasma only sources files it can
// execute, so a missing chmod is a silent no-op that looks like the tweak simply
// not working.
bool MainWindow::kdeEnvWrite(const QString &basename, const QString &body) {
    QDir().mkpath(kdeEnvDir());
    const QString path = kdeEnvFile(basename);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    QTextStream out(&f);
    out << "#!/bin/sh\n"
        << "# Written by CachyOsTools — Tweaks > KDE Plasma.\n"
        << "# Delete this file and log out/in to undo just this option.\n\n"
        << body;
    f.close();
    f.setPermissions(f.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup);
    return true;
}

bool MainWindow::kdeEnvRemove(const QString &basename) {
    const QString path = kdeEnvFile(basename);
    if (!QFileInfo::exists(path)) return true;
    return QFile::remove(path);
}

// Every option shares this: enable writes the snippet, disable deletes it, and
// either way nothing changes until the session restarts.
void MainWindow::kdeEnvToggleApply(const QString &basename, const QString &body,
                                   const QString &title) {
    const bool on = kdeEnvEnabled(basename);
    const int ret = QMessageBox::question(
        this, title,
        on ? tr("This option is currently ENABLED.\n\nRemove it?\n\n%1").arg(kdeEnvFile(basename))
           : tr("This option is currently DISABLED.\n\nEnable it?\n\nWrites: %1").arg(kdeEnvFile(basename)),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const bool ok = on ? kdeEnvRemove(basename) : kdeEnvWrite(basename, body);
    if (!ok) {
        QMessageBox::warning(this, title, tr("Could not write %1").arg(kdeEnvFile(basename)));
        return;
    }
    QMessageBox::information(
        this, title,
        tr("%1.\n\nLog out and back in for this to take effect — KWin needs the "
           "variable in its environment when it starts, so restarting KWin alone "
           "will not pick it up.")
            .arg(on ? tr("Removed") : tr("Enabled")));
    refreshTweaksStatus();
}

void MainWindow::kdeEnvEditConfig(const QString &basename, const QString &body) {
    if (!kdeEnvEnabled(basename)) {
        const int ret = QMessageBox::question(
            this, tr("KDE Tweak"),
            tr("%1 does not exist yet.\n\nCreate it with the default contents so you can edit it?")
                .arg(kdeEnvFile(basename)),
            QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
        if (!kdeEnvWrite(basename, body)) {
            QMessageBox::warning(this, tr("KDE Tweak"), tr("Could not create the file."));
            return;
        }
    }
    openConfigInNano(kdeEnvFile(basename));
}

void MainWindow::kdeEnvBackup(const QString &basename, const QString &description) {
    if (!kdeEnvEnabled(basename)) {
        QMessageBox::information(this, tr("Nothing to Back Up"),
                                 tr("%1 does not exist — the option is not enabled.")
                                     .arg(kdeEnvFile(basename)));
        return;
    }
    backupConfigFile(kdeEnvFile(basename), description);
}

// ── The option bodies ────────────────────────────────────────────────────────
// Kept as functions rather than string constants so the info text, the apply and
// the editor all show byte-identical content.

static QString kdeBodyGlMaxFrames() {
    return "# Cap how many frames the NVIDIA driver may queue ahead of the compositor.\n"
           "# KWin paces its animations against frame SUBMISSION, so a deep queue makes a\n"
           "# smooth animation arrive in uneven bursts — the classic drag-to-top maximize\n"
           "# stutter on X11.\n"
           "export __GL_MaxFramesAllowed=1\n";
}

static QString kdeBodyTripleBuffer() {
    return "# Tell KWin the driver is triple buffering. Left unset it can assume a\n"
           "# buffering depth the NVIDIA driver is not using and pace animations to it.\n"
           "export KWIN_TRIPLE_BUFFER=1\n";
}

static QString kdeBodyGlYield() {
    return "# How the NVIDIA driver waits for the GPU. The default spins on the CPU, which\n"
           "# competes with the compositor for whichever core it is scheduled on.\n"
           "export __GL_YIELD=USLEEP\n";
}

static QString kdeBodyShaderCacheRam() {
    return "# NVIDIA's compiled shader cache, on tmpfs instead of the disk.\n"
           "# A cache miss compiles a shader mid-frame, which is one very long frame at\n"
           "# exactly the moment an effect first runs.\n"
           "#\n"
           "# Trade-off: XDG_RUNTIME_DIR is cleared at logout, so the cache is rebuilt once\n"
           "# per session. Point this at $HOME/.cache/nvidia to keep it across reboots.\n"
           "export __GL_SHADER_DISK_CACHE=1\n"
           "export __GL_SHADER_DISK_CACHE_PATH=\"${XDG_RUNTIME_DIR:-/tmp}/nvidia-glcache\"\n"
           "export __GL_SHADER_DISK_CACHE_SKIP_CLEANUP=1\n"
           "mkdir -p \"$__GL_SHADER_DISK_CACHE_PATH\" 2>/dev/null\n";
}

static QString kdeBodyQmlCacheRam() {
    return "# Compiled QML (.qmlc) on tmpfs instead of ~/.cache. This is the cache behind\n"
           "# everything KWin and Plasma draw.\n"
           "export QML_DISK_CACHE_PATH=\"${XDG_RUNTIME_DIR:-/tmp}/qmlcache\"\n"
           "mkdir -p \"$QML_DISK_CACHE_PATH\" 2>/dev/null\n";
}

static QString kdeBodyPipelineCacheRam() {
    return "# Qt Quick's graphics pipeline cache on tmpfs.\n"
           "export QSG_RHI_PIPELINE_CACHE_LOAD=\"${XDG_RUNTIME_DIR:-/tmp}/qsg-pipeline.cache\"\n"
           "export QSG_RHI_PIPELINE_CACHE_SAVE=\"${XDG_RUNTIME_DIR:-/tmp}/qsg-pipeline.cache\"\n";
}

static QString kdeBodyQtCacheRam() {
    return "# The whole Qt cache root on tmpfs — the blunt version of the two options\n"
           "# above, and it supersedes them. Every Qt cache for this session lives in RAM\n"
           "# and is discarded at logout.\n"
           "export QT_CACHE_HOME=\"${XDG_RUNTIME_DIR:-/tmp}/qtcache\"\n"
           "mkdir -p \"$QT_CACHE_HOME\" 2>/dev/null\n";
}

static QString kdeBodyDisableQmlDiskCache() {
    return "# Compile QML in memory every run and never touch a cache file.\n"
           "#\n"
           "# This is NOT the same as relocating the cache: it trades a disk read for a\n"
           "# full recompile, so it usually makes hitching worse. Useful when a stale or\n"
           "# corrupt cache is the actual problem.\n"
           "export QML_DISABLE_DISK_CACHE=1\n";
}

// ── Slots: one set per option ────────────────────────────────────────────────

#define KDE_TWEAK_SLOTS(Slot, File, Body, Title, Desc, Info)                       \
    void MainWindow::on_##Slot##Toggle_clicked() {                                 \
        showTweakInstructions(Title, Info);                                        \
    }                                                                              \
    void MainWindow::on_##Slot##ApplyButton_clicked() {                            \
        kdeEnvToggleApply(File, Body(), Title);                                    \
    }                                                                              \
    void MainWindow::on_##Slot##ConfigButton_clicked() {                           \
        kdeEnvEditConfig(File, Body());                                            \
    }                                                                              \
    void MainWindow::on_##Slot##BackupButton_clicked() {                           \
        kdeEnvBackup(File, Desc);                                                  \
    }                                                                              \
    void MainWindow::check##Slot##State() {                                        \
        const bool on = kdeEnvEnabled(File);                                       \
        updateTweakStatusLabel(ui->Slot##StatusLabel, on ? "Enabled" : "Disabled", on); \
    }

KDE_TWEAK_SLOTS(kdeGlMaxFrames, "10-gl-maxframes.sh", kdeBodyGlMaxFrames,
    "NVIDIA Frame Queue (__GL_MaxFramesAllowed)", "NVIDIA frame queue depth",
R"(# NVIDIA Frame Queue — __GL_MaxFramesAllowed
# =========================================
#
# WHAT IT DOES
# The NVIDIA driver is allowed to queue several frames ahead of the compositor.
# KWin times its animations against when a frame is SUBMITTED, not when it is
# displayed, so a deep queue makes an evenly-generated animation arrive on screen
# unevenly. Setting the queue to 1 keeps submission and display in step.
#
# WHEN IT HELPS
# This is the usual fix for a window-maximize animation that stutters on X11 with
# the proprietary NVIDIA driver — dragging a window to the top edge, for example.
#
# COST
# A shallower queue can cost a little throughput in GPU-bound fullscreen work.
# It affects the whole session, games included.
#
# FILE
# ~/.config/plasma-workspace/env/10-gl-maxframes.sh
#
#   export __GL_MaxFramesAllowed=1
#
# Takes effect at the next login: KWin must have the variable in its environment
# when it starts. Restarting KWin alone will not do it.
)")

KDE_TWEAK_SLOTS(kdeTripleBuffer, "11-kwin-triple-buffer.sh", kdeBodyTripleBuffer,
    "KWin Triple Buffering (KWIN_TRIPLE_BUFFER)", "KWin triple buffering hint",
R"(# KWin Triple Buffering — KWIN_TRIPLE_BUFFER
# =========================================
#
# WHAT IT DOES
# Tells KWin to expect triple buffering. The NVIDIA driver triple buffers by
# default; if KWin assumes a different depth it paces animations against a
# pipeline that does not match reality, which shows up as uneven motion rather
# than dropped frames.
#
# WHEN IT HELPS
# Pairs with __GL_MaxFramesAllowed. If you are testing one at a time, try that one
# first — this is the smaller of the two effects.
#
# COST
# Adds up to one frame of latency.
#
# FILE
# ~/.config/plasma-workspace/env/11-kwin-triple-buffer.sh
#
#   export KWIN_TRIPLE_BUFFER=1
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdeGlYield, "12-gl-yield.sh", kdeBodyGlYield,
    "NVIDIA GL Yield (__GL_YIELD)", "NVIDIA GL yield behaviour",
R"(# NVIDIA GL Yield — __GL_YIELD=USLEEP
# ==================================
#
# WHAT IT DOES
# Controls how the driver waits for the GPU. By default it spin-waits, burning a
# core to shave latency. On a busy desktop that core is often the one the
# compositor wants, and the contention shows up as micro-stutter.
#
# USLEEP makes it sleep instead. The alternative value is NOTHING, which spins
# without even yielding — worse for a desktop, occasionally better in benchmarks.
#
# COST
# Marginally higher latency in GPU-bound work.
#
# FILE
# ~/.config/plasma-workspace/env/12-gl-yield.sh
#
#   export __GL_YIELD=USLEEP
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdeShaderCacheRam, "20-nvidia-shadercache-ram.sh", kdeBodyShaderCacheRam,
    "NVIDIA Shader Cache in RAM (__GL_SHADER_DISK_CACHE_PATH)", "NVIDIA shader cache location",
R"(# NVIDIA Shader Cache in RAM — __GL_SHADER_DISK_CACHE_PATH
# ========================================================
#
# WHAT IT DOES
# Moves the driver's compiled-shader cache from ~/.cache/nvidia (on disk) to
# tmpfs. A cache miss compiles a shader in the middle of a frame, and that frame
# takes far longer than the rest — a visible hitch the first time an effect runs.
# In RAM the lookup never waits on storage.
#
# COST
# XDG_RUNTIME_DIR is cleared at logout, so the cache is rebuilt once per session
# and the first run of each effect can still hitch. Edit the file and point it at
# $HOME/.cache/nvidia to keep it across reboots and still skip the default path.
# It also holds a few MB of RAM.
#
# FILE
# ~/.config/plasma-workspace/env/20-nvidia-shadercache-ram.sh
#
#   export __GL_SHADER_DISK_CACHE=1
#   export __GL_SHADER_DISK_CACHE_PATH="${XDG_RUNTIME_DIR:-/tmp}/nvidia-glcache"
#   export __GL_SHADER_DISK_CACHE_SKIP_CLEANUP=1
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdeQmlCacheRam, "21-qml-cache-ram.sh", kdeBodyQmlCacheRam,
    "QML Disk Cache in RAM (QML_DISK_CACHE_PATH)", "QML disk cache location",
R"(# QML Disk Cache in RAM — QML_DISK_CACHE_PATH
# ===========================================
#
# WHAT IT DOES
# Qt caches compiled QML as .qmlc files under ~/.cache. That cache backs
# essentially everything KWin and Plasma draw. This points it at tmpfs so the
# lookups are memory reads.
#
# WHEN IT HELPS
# Mostly load-time smoothness — opening panels, popups, System Settings pages —
# rather than mid-animation stutter, since QML is read at load rather than per
# frame.
#
# COST
# Cleared at logout, so the first load of each QML component per session is a
# recompile. Costs a few MB of RAM.
#
# FILE
# ~/.config/plasma-workspace/env/21-qml-cache-ram.sh
#
#   export QML_DISK_CACHE_PATH="${XDG_RUNTIME_DIR:-/tmp}/qmlcache"
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdePipelineCacheRam, "22-qsg-pipeline-cache-ram.sh", kdeBodyPipelineCacheRam,
    "Qt Quick Pipeline Cache in RAM (QSG_RHI_PIPELINE_CACHE)", "Qt Quick pipeline cache location",
R"(# Qt Quick Pipeline Cache in RAM — QSG_RHI_PIPELINE_CACHE_LOAD / _SAVE
# ====================================================================
#
# WHAT IT DOES
# Qt Quick's RHI keeps a graphics pipeline cache — the compiled state objects a
# shader needs before it can draw. Building one mid-frame is a hitch. These two
# variables set where it is read from and written to; pointing both at tmpfs
# takes storage out of that path.
#
# COST
# Rebuilt each session, a few MB of RAM.
#
# FILE
# ~/.config/plasma-workspace/env/22-qsg-pipeline-cache-ram.sh
#
#   export QSG_RHI_PIPELINE_CACHE_LOAD="${XDG_RUNTIME_DIR:-/tmp}/qsg-pipeline.cache"
#   export QSG_RHI_PIPELINE_CACHE_SAVE="${XDG_RUNTIME_DIR:-/tmp}/qsg-pipeline.cache"
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdeQtCacheRam, "23-qt-cache-ram.sh", kdeBodyQtCacheRam,
    "All Qt Caches in RAM (QT_CACHE_HOME)", "Qt cache root location",
R"(# All Qt Caches in RAM — QT_CACHE_HOME
# ====================================
#
# WHAT IT DOES
# Relocates the entire Qt cache root for the session, rather than one cache at a
# time. This is the blunt version of the QML and pipeline options above and it
# supersedes both — there is no reason to enable this and those together.
#
# COST
# Everything Qt would have cached is discarded at logout, so the first run of a
# session is colder across the board. Uses more RAM than the targeted options,
# though still a small amount.
#
# FILE
# ~/.config/plasma-workspace/env/23-qt-cache-ram.sh
#
#   export QT_CACHE_HOME="${XDG_RUNTIME_DIR:-/tmp}/qtcache"
#
# Takes effect at the next login.
)")

KDE_TWEAK_SLOTS(kdeDisableQmlCache, "24-qml-disable-disk-cache.sh", kdeBodyDisableQmlDiskCache,
    "Disable QML Disk Cache (QML_DISABLE_DISK_CACHE)", "QML disk cache disabled",
R"(# Disable QML Disk Cache — QML_DISABLE_DISK_CACHE
# ==============================================
#
# WHAT IT DOES
# Stops Qt using a QML cache file at all. QML is recompiled from source every
# run, in memory.
#
# READ THIS BEFORE ENABLING
# This is NOT "use RAM instead of disk". Relocating the cache (the QML Disk Cache
# in RAM option) keeps the cache and makes it faster to reach. This one removes
# the cache, so every load pays a full recompile — it normally makes hitching
# WORSE, and it is here for one specific job: proving whether a stale or corrupt
# cache is the actual problem. If enabling this fixes something, the real fix is
# to delete ~/.cache/qmlcache and turn this back off.
#
# The related switches, same caveat: QSG_RHI_DISABLE_DISK_CACHE and
# QT_DISABLE_SHADER_DISK_CACHE.
#
# FILE
# ~/.config/plasma-workspace/env/24-qml-disable-disk-cache.sh
#
#   export QML_DISABLE_DISK_CACHE=1
#
# Takes effect at the next login.
)")

#undef KDE_TWEAK_SLOTS

// ── Panel floating (not an env var — live Plasma setting) ────────────────────

void MainWindow::on_kdePanelFloatingToggle_clicked() {
    showTweakInstructions("Panel Floating Mode",
R"(# Panel Floating Mode
# ===================
#
# WHAT IT DOES
# A floating panel is detached from the screen edge with a margin and its own
# shadow. On X11 that geometry has a long-standing interaction with the window
# maximize animation: dragging a window to the top edge to maximize it stutters,
# and does not when the panel is docked.
#
# This reads and sets the live value through Plasma's scripting interface rather
# than editing plasma-org.kde.plasma.desktop-appletsrc, because plasmashell
# rewrites that file from memory and would overwrite an edit made underneath it.
#
# APPLY toggles floating for every panel and takes effect immediately — no
# logout. It is purely cosmetic otherwise.
#
# You can also do it by hand: right-click the panel, Enter Edit Mode, and use the
# Floating switch in the panel settings.
)");
}

// Reads the live state via qdbus rather than the config file, for the reason in
// the info text above.
static QString kdePanelFloatingQuery() {
    QProcess proc;
    const QString script =
        "var out=\"\"; var p=panels(); for (var i=0;i<p.length;i++) out += p[i].floating + \" \"; print(out);";
    for (const QString &tool : {QStringLiteral("qdbus6"), QStringLiteral("qdbus")}) {
        proc.start(tool, QStringList()
                             << "org.kde.plasmashell" << "/PlasmaShell"
                             << "org.kde.PlasmaShell.evaluateScript" << script);
        if (!proc.waitForFinished(4000)) continue;
        const QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) return out;
    }
    return QString();
}

void MainWindow::checkkdePanelFloatingState() {
    const QString out = kdePanelFloatingQuery();
    if (out.isEmpty()) {
        updateTweakStatusLabel(ui->kdePanelFloatingStatusLabel, "Unknown", false);
        return;
    }
    const bool floating = out.contains("true");
    // Floating is the state implicated in the stutter, so "not floating" is the
    // tuned state and reads as enabled here.
    updateTweakStatusLabel(ui->kdePanelFloatingStatusLabel,
                           floating ? "Floating" : "Docked", !floating);
}

void MainWindow::on_kdePanelFloatingApplyButton_clicked() {
    const QString cur = kdePanelFloatingQuery();
    if (cur.isEmpty()) {
        QMessageBox::warning(this, tr("Panel Floating Mode"),
                             tr("Could not reach plasmashell over D-Bus.\n\n"
                                "Is this a running Plasma session?"));
        return;
    }
    const bool floating = cur.contains("true");
    const int ret = QMessageBox::question(
        this, tr("Panel Floating Mode"),
        floating ? tr("Panels are currently FLOATING.\n\nDock them to the screen edge?")
                 : tr("Panels are currently DOCKED.\n\nMake them float?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    const QString set = floating ? "false" : "true";
    const QString script =
        QString("var p=panels(); for (var i=0;i<p.length;i++) p[i].floating = %1;").arg(set);
    QProcess proc;
    for (const QString &tool : {QStringLiteral("qdbus6"), QStringLiteral("qdbus")}) {
        proc.start(tool, QStringList()
                             << "org.kde.plasmashell" << "/PlasmaShell"
                             << "org.kde.PlasmaShell.evaluateScript" << script);
        if (proc.waitForFinished(4000)) break;
    }
    QTimer::singleShot(500, this, &MainWindow::refreshTweaksStatus);
}

void MainWindow::on_kdePanelFloatingConfigButton_clicked() {
    openConfigInNano(QDir::homePath() + "/.config/plasma-org.kde.plasma.desktop-appletsrc");
}

void MainWindow::on_kdePanelFloatingBackupButton_clicked() {
    backupConfigFile(QDir::homePath() + "/.config/plasma-org.kde.plasma.desktop-appletsrc",
                     "Plasma panel and desktop layout");
}

// ── Section-wide actions ─────────────────────────────────────────────────────

void MainWindow::on_kdeOpenEnvFolderButton_clicked() {
    QDir().mkpath(kdeEnvDir());
    QProcess::startDetached("xdg-open", QStringList() << kdeEnvDir());
}

void MainWindow::on_kdeDisableAllButton_clicked() {
    const QStringList files = {
        "10-gl-maxframes.sh", "11-kwin-triple-buffer.sh", "12-gl-yield.sh",
        "20-nvidia-shadercache-ram.sh", "21-qml-cache-ram.sh",
        "22-qsg-pipeline-cache-ram.sh", "23-qt-cache-ram.sh",
        "24-qml-disable-disk-cache.sh"
    };
    QStringList present;
    for (const QString &f : files)
        if (kdeEnvEnabled(f)) present << f;

    if (present.isEmpty()) {
        QMessageBox::information(this, tr("Nothing to Remove"),
                                 tr("None of the KDE environment tweaks are enabled."));
        return;
    }
    const int ret = QMessageBox::question(
        this, tr("Remove All KDE Tweaks"),
        tr("Remove these %1 file(s) from %2?\n\n%3")
            .arg(present.size()).arg(kdeEnvDir()).arg(present.join("\n")),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    for (const QString &f : present) kdeEnvRemove(f);
    QMessageBox::information(this, tr("Removed"),
                             tr("Removed %1 file(s). Log out and back in to return to defaults.")
                                 .arg(present.size()));
    refreshTweaksStatus();
}

void MainWindow::refreshKdeTweaksStatus() {
    checkkdeGlMaxFramesState();
    checkkdeTripleBufferState();
    checkkdeGlYieldState();
    checkkdeShaderCacheRamState();
    checkkdeQmlCacheRamState();
    checkkdePipelineCacheRamState();
    checkkdeQtCacheRamState();
    checkkdeDisableQmlCacheState();
    checkkdePanelFloatingState();
    // Known-issue rows (kde_fixes.h)
    checkkdeFixFolderDragState();
    checkkdeFixWidgetsLockedState();
    checkkdeFixConfigCorruptState();
    checkkdeFixPlasmaCacheState();
}
