"""Render the real app with isolated settings and unavailable system commands.

The command directory deliberately omits shells, privilege helpers, desktop
launchers and service tools. No install, mount, boot edit or live window action
is part of this presentation test.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='cachyostools-ui-') as temporary:
    root = Path(temporary)
    for folder in ('bin', 'config', 'data', 'cache', 'runtime'):
        (root/folder).mkdir(mode=0o700)
    (root/'bin/python3').symlink_to(sys.executable)
    env = dict(os.environ, PATH=str(root/'bin'), QT_QPA_PLATFORM='offscreen', QT_QPA_PLATFORMTHEME='',
               XDG_CONFIG_HOME=str(root/'config'), XDG_DATA_HOME=str(root/'data'),
               XDG_CACHE_HOME=str(root/'cache'), XDG_RUNTIME_DIR=str(root/'runtime'),
               DBUS_SESSION_BUS_ADDRESS='unix:path='+str(root/'no-session-bus'),
               CACHY_SMOKE_ISOLATED='1')
    for key in ('DISPLAY','WAYLAND_DISPLAY','HYPRLAND_INSTANCE_SIGNATURE'):
        env.pop(key, None)
    raise SystemExit(subprocess.run(sys.argv[1:], env=env, timeout=90).returncode)
