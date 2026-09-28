"""Validate compositor bridge discovery and action handling without a desktop."""
from pathlib import Path
import shutil
import subprocess
import unittest


class BridgeTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('node'), 'Node.js is needed for bridge tests')
    def test_compositor_bridges(self):
        result = subprocess.run(['node', str(Path(__file__).with_name('window_enum_bridges.js'))],
                                capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
