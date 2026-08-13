#ifndef CACHYOS_SCRIPT_HELPERS_H
#define CACHYOS_SCRIPT_HELPERS_H

#include <QString>

// Shared shell-script plumbing for the panels that run privileged repairs in a
// visible terminal (Pacman Doctor, Sensors & Power, Network Shares, Autostart).

// Wraps a script body in `set -e` plus an ERR trap. Without the trap, a failing
// command under `set -e` exits before runScriptInTerminal's trailing prompt and
// the terminal window slams shut on the error the user needed to read.
inline QString repairScript(const QString &body) {
    return QStringLiteral(
        "set -e\n"
        "trap 'echo; echo \"⚠️  A command failed — the error is printed above.\";"
        " echo \"Press Enter to close.\"; read -r; exit 1' ERR\n") + body;
}

// Single-quotes a value for safe interpolation into a bash script.
// Any embedded single quote is closed, escaped and reopened: it's  ->  'it'\''s'
inline QString shQuote(const QString &s) {
    QString out = s;
    out.replace("'", "'\\''");
    return "'" + out + "'";
}

#endif // CACHYOS_SCRIPT_HELPERS_H
