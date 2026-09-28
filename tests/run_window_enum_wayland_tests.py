#!/usr/bin/env python3
"""Exercise the actual Wayland client against private protocol-only servers."""
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import tempfile
import time

from run_window_enum_tests import stop


def read(process):
    if not select.select([process.stdout], [], [], 5)[0]:
        raise AssertionError('Timed out waiting for the Wayland fixture.')
    line = process.stdout.readline()
    if not line:
        raise AssertionError('Wayland process stopped: ' + process.stderr.read())
    return line.strip()


def request(process, value):
    process.stdin.write(json.dumps(value) + '\n')
    process.stdin.flush()
    return json.loads(read(process))


def wait_for(process, predicate):
    deadline = time.monotonic() + 3
    while time.monotonic() < deadline:
        snapshot = request(process, {'action': 'list'})
        if predicate(snapshot):
            return
        time.sleep(0.03)
    raise AssertionError(snapshot)


def scenario(binary, fixture, protocol):
    with tempfile.TemporaryDirectory(prefix='window-enum-wayland-') as tmp:
        env = dict(os.environ, XDG_RUNTIME_DIR=tmp)
        server = subprocess.Popen([str(fixture), protocol], env=env, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, text=True)
        try:
            env['WAYLAND_DISPLAY'] = read(server)
            helper = subprocess.Popen([str(binary), '--window-enum-wayland'], env=env,
                                      stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, text=True)
            try:
                first = json.loads(read(helper))
                assert len(first['windows']) == 1, first
                row = first['windows'][0]
                assert row['title'] == 'Wayland fixture Ω', row
                assert row['appId'] == 'org.xetal.Fixture', row
                for unavailable in ('x', 'width', 'pid', 'executable', 'above'):
                    assert unavailable not in row, 'Do not invent unavailable metadata: ' + unavailable
                assert request(helper, {'action': 'close', 'id': 'stale-connection-id'}).get('error')
                assert request(helper, {'action': 'geometry', 'id': row['id'],
                                        'x': 0, 'y': 0, 'width': 100, 'height': 100}).get('error')
                if protocol == 'wlr':
                    for field in ('minimized', 'maximized', 'fullscreen'):
                        assert request(helper, {'action': field, 'id': row['id'], 'value': True}).get('ok')
                        wait_for(helper, lambda snapshot: snapshot['windows'][0][field] is True)
                        assert request(helper, {'action': field, 'id': row['id'], 'value': False}).get('ok')
                        wait_for(helper, lambda snapshot: snapshot['windows'][0][field] is False)
                    assert request(helper, {'action': 'close', 'id': row['id']}).get('ok')
                    wait_for(helper, lambda snapshot: not snapshot['windows'])
                    assert request(helper, {'action': 'close', 'id': row['id']}).get('error')
                else:
                    assert row['nativeId'] == 'test-window-1'
                    assert row['capabilities'] == []
                    assert request(helper, {'action': 'close', 'id': row['id']}).get('error')
            finally:
                stop(helper)
                helper.stdin.close()
                helper.stdout.close()
                helper.stderr.close()
        finally:
            stop(server)
            server.stdout.close()
            server.stderr.close()
    print('PASS:', protocol, 'enumeration, identity, missing metadata and supported controls')


if __name__ == '__main__':
    binary, fixture = map(lambda p: Path(p).resolve(), sys.argv[1:])
    for protocol in ('wlr', 'ext'):
        scenario(binary, fixture, protocol)
