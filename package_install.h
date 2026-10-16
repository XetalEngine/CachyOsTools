#ifndef CACHYOS_PACKAGE_INSTALL_H
#define CACHYOS_PACKAGE_INSTALL_H

#include <QCoreApplication>
#include "script_helpers.h"

class PackageInstall {
    Q_DECLARE_TR_FUNCTIONS(PackageInstall)
public:
    static QString script(const QString &installer, const QString &installerName,
                          const QString &packageName, bool skipReview);
};

inline QString PackageInstall::script(const QString &installer, const QString &installerName,
                                       const QString &packageName, bool skipReview) {
    QString command;
    if (installer == "pacman") {
        command = "sudo pacman -Sy && sudo pacman -S -- " + shQuote(packageName);
    } else {
        command = shQuote(installer) + " -S";
        if (skipReview && installer == "yay") {
            command += " --answerdiff None --answeredit None";
        } else if (skipReview && installer == "paru") {
            command += " --skipreview";
        }
        command += " -- " + shQuote(packageName);
    }

    QString instructions = tr("CachyOS Tools: installing %1 using %2").arg(packageName, installerName)
        + "\n\n" + tr("Terminal instructions:") + "\n"
        + tr("  At [Y/n], Enter selects Yes. At [y/N], Enter selects No.") + "\n"
        + tr("  Enter your sudo password when asked; typed characters are hidden.") + "\n"
        + tr("  Check the package list before confirming installation. Ctrl+C cancels.") + "\n";

    if (installer == "yay" || installer == "paru") {
        if (skipReview) {
            instructions += "\n" + tr("Build-file review is skipped for this install. Other prompts still need your input.") + "\n";
        } else {
            instructions += "\n" + tr("Build-file review:") + "\n";
            if (installer == "yay") {
                instructions += tr("  At 'Diffs to show?' or the PKGBUILD edit menu, type N then Enter to skip.") + "\n";
            } else {
                instructions += tr("  At 'Proceed to review?', Enter opens the review; n cancels the installation.") + "\n"
                    + tr("  At 'Accept changes?', type y then Enter if you accept the build files.") + "\n";
            }
            instructions += tr("  In the default less viewer, Space scrolls and q closes the review.") + "\n"
                + tr("  To skip review entirely, cancel and select 'Skip build-file review' when installing again.") + "\n";
        }
    }

    instructions += "\n" + tr("Command: %1").arg(command) + "\n";
    return "printf '%s\\n' " + shQuote(instructions) + "\n"
        + command + "\n"
        + "package_install_status=$?\n"
          "printf '\\n'\n"
          "if [ \"$package_install_status\" -eq 0 ]; then\n"
          "  printf '%s\\n' " + shQuote(tr("Installation completed successfully.")) + "\n"
          "else\n"
          "  printf '%s\\n' " + shQuote(tr("Installation did not complete. Check the output above.")) + "\n"
          "fi\n"
          "read -r -p " + shQuote(tr("Press Enter to close...")) + "\n"
          "exit \"$package_install_status\"\n";
}

#endif // CACHYOS_PACKAGE_INSTALL_H
