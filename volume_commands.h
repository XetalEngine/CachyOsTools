#pragma once
#include <QStringList>

// CachyOsTools volume actions pass arguments directly to udisksctl.
// Authentication is handled by the session's polkit agent.
namespace CachyVolumes {
inline QStringList mount(const QString &device, bool force = false) {
  QStringList args{"mount", "-b", device};
  if (force)
    args << "-o" << "force";
  return args;
}
inline QStringList unmount(const QString &device) {
  return {"unmount", "-b", device};
}
} // namespace CachyVolumes
