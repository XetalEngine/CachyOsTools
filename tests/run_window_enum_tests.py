#!/usr/bin/env python3
"""Run window tests on a disposable X server and, optionally, nested KWin."""
import argparse
import os
from pathlib import Path
import select
import subprocess
import sys
import tempfile
import time


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def session(args):
    if not args.kwin and not args.kwin_x11:
        return subprocess.call([str(args.binary)], env=os.environ)
    log = Path(os.environ['XDG_RUNTIME_DIR']) / 'kwin.log'
    with log.open('w+') as output:
        env = dict(os.environ, QT_QPA_PLATFORM='xcb' if args.kwin_x11 else 'offscreen', QT_QUICK_BACKEND='software',
                   LIBGL_ALWAYS_SOFTWARE='1')
        command = ['kwin_x11'] if args.kwin_x11 else ['kwin_wayland', '--virtual', '--width', '1600', '--height', '900',
                                 '--no-lockscreen', '--no-global-shortcuts', '--no-kactivities',
                                 '--socket', 'window-enum-test']
        kwin = subprocess.Popen(command, env=env, stdout=output, stderr=output)
        try:
            socket = Path(env['XDG_RUNTIME_DIR']) / 'window-enum-test'
            deadline = time.monotonic() + 15
            while not args.kwin_x11 and not socket.exists() and kwin.poll() is None and time.monotonic() < deadline:
                time.sleep(0.1)
            if not args.kwin_x11 and not socket.exists():
                output.seek(0)
                raise RuntimeError(output.read())
            if args.kwin_x11:
                time.sleep(0.5)
                env.update(WINDOW_ENUM_WM_TEST='1')
            else:
                env.update(QT_QPA_PLATFORM='wayland', WAYLAND_DISPLAY=socket.name,
                           XDG_SESSION_TYPE='wayland', WINDOW_ENUM_KWIN_TEST='1')
            result = subprocess.call([str(args.binary)], env=env)
            if result:
                output.seek(0)
                print(output.read()[-10000:], file=sys.stderr)
            return result
        finally:
            stop(kwin)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=lambda p: Path(p).resolve())
    parser.add_argument('--kwin', action='store_true')
    parser.add_argument('--kwin-x11', action='store_true')
    parser.add_argument('--session', action='store_true')
    args = parser.parse_args()
    if args.session:
        return session(args)
    with tempfile.TemporaryDirectory(prefix='window-enum-test-') as tmp:
        root = Path(tmp)
        env = dict(os.environ)
        env.update(QT_NO_XDG_DESKTOP_PORTAL='1', NO_AT_BRIDGE='1', QT_ACCESSIBILITY='0')
        for name, directory in [('XDG_RUNTIME_DIR', 'runtime'), ('XDG_CONFIG_HOME', 'config'),
                                ('XDG_DATA_HOME', 'data'), ('XDG_CACHE_HOME', 'cache')]:
            path = root / directory
            path.mkdir(mode=0o700)
            env[name] = str(path)
        for name in ('WAYLAND_DISPLAY', 'DBUS_SESSION_BUS_ADDRESS', 'DBUS_SESSION_BUS_PID'):
            env.pop(name, None)
        with (root / 'xvfb.log').open('w') as log:
            xvfb = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '1600x900x24',
                                     '-nolisten', 'tcp'], stdout=subprocess.PIPE, stderr=log, text=True, env=env)
            try:
                if not select.select([xvfb.stdout], [], [], 10)[0]:
                    raise RuntimeError('Xvfb did not start.')
                display = xvfb.stdout.readline().strip()
                if not display.isdecimal():
                    raise RuntimeError('Xvfb could not create an isolated display.')
                env.update(DISPLAY=':' + display, WINDOW_ENUM_ISOLATED='1',
                           XDG_SESSION_TYPE='x11', XDG_CURRENT_DESKTOP='window-enum-test',
                           QT_QPA_PLATFORM='xcb')
                command = ['dbus-run-session', '--', sys.executable, __file__, str(args.binary), '--session']
                if args.kwin:
                    command.append('--kwin')
                if args.kwin_x11:
                    command.append('--kwin-x11')
                return subprocess.call(command, env=env)
            finally:
                stop(xvfb)
                xvfb.stdout.close()


if __name__ == '__main__':
    sys.exit(main())
