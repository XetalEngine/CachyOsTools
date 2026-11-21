"""Isolated disk fixtures only: never scan or remove personal files."""
import json
import os
from pathlib import Path
import select
import sqlite3
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from disk_scan import Cancelled, DiskIndex, browse, open_directory


class DiskScanTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        self.index = DiskIndex()
        self.addCleanup(self.index.close)

    def file(self, relative, size):
        path = self.path / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"x" * size)
        return path

    def scan(self):
        result = self.index.scan(str(self.path))
        self.db = self.index.connect()
        self.addCleanup(self.db.close)
        return result

    def ident(self, path):
        return self.db.execute("SELECT id FROM nodes WHERE path=?", (os.fsencode(path),)).fetchone()[0]

    def assert_matches_fresh_scan(self):
        fresh = DiskIndex()
        self.addCleanup(fresh.close)
        fresh.scan(str(self.path))
        other = fresh.connect()
        self.addCleanup(other.close)
        query = """SELECT path,size,allocated,files,dirs,direct_size,direct_allocated,
            direct_files,own_allocated,mtime FROM nodes ORDER BY path"""
        self.assertEqual([tuple(row) for row in self.db.execute(query)],
                         [tuple(row) for row in other.execute(query)])
        self.assertEqual(self.index.count, fresh.count)

    def test_folders_largest_first_files_bucket_always_last(self):
        self.file("small/a", 20)
        self.file("big/b", 10000)
        self.file("largest-direct-file", 50000)
        self.file("tiny", 1)
        result = self.scan()
        self.assertEqual(result["root"]["size"], 60021)
        nodes = self.index.children(self.db, 1, "size")["nodes"]
        self.assertEqual([n["name"] for n in nodes], ["big", "small", "Files"])
        files = self.index.children(self.db, -1, "size")["nodes"]
        self.assertEqual([n["name"] for n in files], ["largest-direct-file", "tiny"])
        self.assertEqual(nodes[-1]["size"], 50001)

    def test_sparse_files_and_hard_links_are_not_overcounted(self):
        sparse = self.file("sparse", 1)
        os.truncate(sparse, 1 << 30)
        source = self.file("source", 9000)
        os.link(source, self.path / "hard-link")
        result = self.scan()["root"]
        self.assertEqual(result["size"], (1 << 30) + 9000)
        self.assertLess(result["allocated"], 1 << 20)
        linked = self.db.execute("SELECT status FROM nodes WHERE status LIKE 'Hard link%'").fetchall()
        self.assertEqual(len(linked), 1)

    def test_symlinks_are_not_followed_and_unlink_keeps_target(self):
        target = self.file("target/keep", 32)
        link = self.path / "link"
        link.symlink_to(target.parent)
        result = self.scan()
        self.assertEqual(result["root"]["files"], 2)
        self.index.remove(self.db, self.ident(link))
        self.assertTrue(target.exists())
        self.assertFalse(link.exists())

    def test_unusual_and_non_utf8_names_survive_scan_and_id_based_delete(self):
        path = self.file("quote'\n☃", 17)
        raw = os.fsencode(self.path) + b"/bad-\xff"
        with open(raw, "wb") as output:
            output.write(b"literal bytes")
        self.scan()
        self.assertEqual(self.index.node(self.db, self.ident(path))["path"], str(path))
        ident = self.ident(raw)
        self.assertFalse(self.index.node(self.db, ident)["utf8"])
        self.index.remove(self.db, ident)
        self.assertFalse(os.path.exists(raw))
        self.assertTrue(path.exists())

    def test_root_and_files_group_cannot_be_deleted(self):
        path = self.file("keep", 8)
        self.scan()
        for ident in (1, -1, 0, -99):
            with self.subTest(ident=ident), self.assertRaises(ValueError):
                self.index.remove(self.db, ident)
        self.assertTrue(path.exists())

    def test_changed_file_refuses_delete(self):
        path = self.file("changed", 8)
        self.scan()
        path.write_text("edited after scan")
        with self.assertRaisesRegex(ValueError, "changed since"):
            self.index.remove(self.db, self.ident(path))
        self.assertEqual(path.read_text(), "edited after scan")

    def test_symlink_replacement_of_parent_cannot_redirect_deletion(self):
        path = self.file("original/keep", 12)
        other = self.file("other/keep", 12)
        self.scan()
        ident = self.ident(path)
        path.parent.rename(self.path / "moved")
        (self.path / "original").symlink_to(other.parent)
        with self.assertRaises(OSError):
            self.index.remove(self.db, ident)
        self.assertTrue(other.exists())
        self.assertTrue((self.path / "moved/keep").exists())

    def test_directory_removal_is_scoped_to_selected_subtree(self):
        gone = self.file("remove/nested/file", 50)
        keep = self.file("keep/file", 17)
        self.scan()
        self.index.remove(self.db, self.ident(gone.parents[1]))
        self.assertFalse(gone.parents[1].exists())
        self.assertTrue(keep.exists())
        self.assertTrue(self.index.ready)
        self.assert_matches_fresh_scan()

    def test_removal_reuses_scan_and_preserves_unaffected_ids(self):
        gone = self.file("remove/nested/file", 50)
        keep = self.file("keep/file", 17)
        before = self.scan()
        kept_id = self.ident(keep)
        # The incremental update must not walk an unrelated branch again.
        self.file("keep/created-after-scan", 999)
        with patch("disk_scan.os.scandir", side_effect=AssertionError("unexpected scan")):
            result = self.index.remove(self.db, self.ident(gone.parents[1]))
            self.index.map(self.db, 1, "allocated")
        self.assertEqual(result["root"]["size"], 17)
        self.assertEqual(result["entries"], before["entries"] - 3)
        self.assertEqual(self.ident(keep), kept_id)
        self.assertEqual(self.index.node(self.db, kept_id)["size"], 17)
        self.assertTrue((keep.parent / "created-after-scan").exists())

    def test_repeated_deletions_update_files_groups_and_parent_identity(self):
        first = self.file("parent/first", 99)
        last = self.file("parent/last", 15)
        self.file("keep", 9)
        self.scan()
        parent_id = self.ident(first.parent)
        self.index.remove(self.db, self.ident(first), view=-parent_id)
        self.assertEqual(self.index.node(self.db, -parent_id)["files"], 1)
        self.assert_matches_fresh_scan()
        result = self.index.remove(self.db, self.ident(last), view=-parent_id)
        self.assertEqual(result["view"], parent_id)  # Empty Files group goes back to its folder.
        self.assertFalse(self.index.node(self.db, parent_id)["children"])
        self.assert_matches_fresh_scan()
        result = self.index.remove(self.db, parent_id, view=parent_id)
        self.assertEqual(result["view"], 1)
        self.assert_matches_fresh_scan()

    def test_deleting_ancestor_of_current_view_returns_surviving_parent(self):
        nested = self.file("parent/remove/deep/file", 10)
        self.file("parent/keep", 11)
        self.scan()
        parent_id = self.ident(nested.parents[2])
        result = self.index.remove(self.db, self.ident(nested.parents[1]), view=-self.ident(nested.parent))
        self.assertEqual(result["view"], parent_id)
        self.assert_matches_fresh_scan()

    def test_hard_link_bytes_transfer_until_last_alias_is_removed(self):
        source = self.file("first/owner", 9000)
        (self.path / "second").mkdir()
        os.link(source, self.path / "first/alias")
        os.link(source, self.path / "second/survivor")
        self.scan()
        # Select whichever branch holds the counted link, regardless of readdir order.
        owner = self.db.execute("SELECT parent FROM nodes WHERE linked=1 AND size>0").fetchone()[0]
        result = self.index.remove(self.db, owner)
        self.assertEqual(result["root"]["size"], 9000)
        self.assert_matches_fresh_scan()
        while survivor := self.db.execute("SELECT id FROM nodes WHERE linked=1 AND size>0").fetchone():
            self.index.remove(self.db, survivor[0])
            self.assert_matches_fresh_scan()
        self.assertEqual(self.index.node(self.db, 1)["size"], 0)

    def test_removing_uncounted_hard_link_keeps_counted_bytes(self):
        source = self.file("source", 9000)
        os.link(source, self.path / "alias")
        self.scan()
        alias = self.db.execute("SELECT id FROM nodes WHERE linked=1 AND size=0").fetchone()[0]
        result = self.index.remove(self.db, alias)
        self.assertEqual(result["root"]["size"], 9000)
        self.assertEqual(result["root"]["files"], 1)
        self.assert_matches_fresh_scan()

    def test_partial_deletion_invalidates_index_instead_of_subtracting_entire_branch(self):
        first = self.file("remove/one", 11)
        last = self.file("remove/two", 22)
        before = self.scan()
        unlink = os.unlink
        listdir = os.listdir

        def fail_second(name, **kwargs):
            if name == b"two":
                raise PermissionError("fixture deletion failure")
            return unlink(name, **kwargs)

        with patch("disk_scan.os.unlink", side_effect=fail_second), \
                patch("disk_scan.os.listdir", side_effect=lambda fd: sorted(listdir(fd))):
            with self.assertRaises(PermissionError):
                self.index.remove(self.db, self.ident(first.parent))
        self.assertFalse(first.exists())
        self.assertTrue(last.exists())
        self.assertFalse(self.index.ready)
        self.assertEqual(self.index.node(self.db, 1), before["root"])
        refreshed = self.index.scan(str(self.path))
        self.assertEqual(refreshed["root"]["size"], 22)
        self.assertEqual(refreshed["root"]["files"], 1)

    def test_disappearing_child_is_already_removed(self):
        for operation in ("stat", "unlink"):
            with self.subTest(operation=operation):
                temporary = self.file("selected/gnupg/S.gpg-agent.browser", 0)
                self.file("selected/keep-deleting", 25)
                outside = self.file("outside/keep", 11)
                self.scan()
                original = getattr(os, operation)
                unlink = os.unlink
                disappeared = False

                def disappear(name, *args, **kwargs):
                    nonlocal disappeared
                    if name == b"S.gpg-agent.browser" and not disappeared:
                        disappeared = True
                        unlink(name, dir_fd=kwargs["dir_fd"])
                    return original(name, *args, **kwargs)

                with patch("disk_scan.os." + operation, side_effect=disappear):
                    result = self.index.remove(self.db, self.ident(temporary.parents[1]))
                self.assertTrue(disappeared)
                self.assertFalse(temporary.parents[1].exists())
                self.assertTrue(outside.exists())
                self.assertTrue(result["deleted"])
                self.assertTrue(self.index.ready)
                self.assert_matches_fresh_scan()

    def test_disappearing_directory_is_already_removed(self):
        for operation in ("open", "listdir", "stat", "rmdir"):
            with self.subTest(operation=operation):
                gone = self.path / "selected/ephemeral"
                gone.mkdir(parents=True)
                self.file("selected/keep-deleting", 25)
                self.file("outside/keep", 11)
                self.scan()
                original = getattr(os, operation)
                rmdir = os.rmdir
                inode = gone.stat().st_ino
                disappeared = False
                stat_calls = 0

                def disappear(name, *args, **kwargs):
                    nonlocal disappeared, stat_calls
                    match = (os.fstat(name).st_ino == inode if operation == "listdir"
                             else name == b"ephemeral")
                    if match and operation == "stat":
                        stat_calls += 1
                        match = stat_calls == 2  # After visiting the opened directory.
                    if match and not disappeared:
                        disappeared = True
                        rmdir(gone)
                        if operation == "listdir":
                            raise FileNotFoundError("Directory removed after open")
                    return original(name, *args, **kwargs)

                with patch("disk_scan.os." + operation, side_effect=disappear):
                    self.index.remove(self.db, self.ident(gone.parent))
                self.assertTrue(disappeared)
                self.assertFalse(gone.parent.exists())
                self.assertTrue(self.index.ready)
                self.assert_matches_fresh_scan()

    def test_replaced_child_is_not_treated_as_missing(self):
        temporary = self.file("selected/temporary", 12)
        self.file("selected/keep", 25)
        self.scan()
        original = os.listdir
        replaced = False

        def replace(fd):
            nonlocal replaced
            entries = original(fd)
            if not replaced:
                replaced = True
                temporary.rename(self.path / "saved-original")
                temporary.write_text("a replacement that must survive")
            return entries

        with patch("disk_scan.os.listdir", side_effect=replace):
            with self.assertRaisesRegex(ValueError, "changed since"):
                self.index.remove(self.db, self.ident(temporary.parent))
        self.assertEqual(temporary.read_text(), "a replacement that must survive")
        self.assertTrue((self.path / "saved-original").exists())
        self.assertFalse(self.index.ready)

    def test_new_child_blocks_recursive_removal(self):
        old = self.file("selected/old", 5)
        self.scan()
        new = self.file("selected/new", 7)
        with self.assertRaisesRegex(ValueError, "changed since"):
            self.index.remove(self.db, self.ident(old.parent))
        self.assertTrue(old.exists())
        self.assertTrue(new.exists())

    def test_mounts_are_skipped_including_same_device_bind_mounts(self):
        child = self.file("mounted/private", 45)
        self.file("normal/data", 50)
        with patch("disk_scan.mount_points", return_value={os.fsencode(child.parent): b"ext4"}):
            result = self.scan()
            self.assertEqual(result["root"]["size"], 50)
            with self.assertRaises(ValueError):
                self.index.validate_target(self.db, self.ident(child.parent))
        self.assertEqual(self.db.execute("SELECT COUNT(*) FROM nodes WHERE name=?", (b"private",)).fetchone()[0], 0)

    def test_optional_cross_mount_scan_still_skips_kernel_filesystems(self):
        disk = self.file("disk/data", 50)
        virtual = self.file("virtual/never", 70)
        with patch("disk_scan.mount_points", return_value={os.fsencode(disk.parent): b"ext4", os.fsencode(virtual.parent): b"proc"}):
            result = self.index.scan(str(self.path), cross=True)
        self.assertEqual(result["root"]["size"], 50)

    def test_new_mount_inside_selected_folder_blocks_removal(self):
        file = self.file("folder/mounted/keep", 70)
        self.scan()
        with patch("disk_scan.mount_points", return_value={os.fsencode(file.parent): b"ext4"}):
            with self.assertRaisesRegex(ValueError, "mount points"):
                self.index.remove(self.db, self.ident(file.parents[1]))
        self.assertTrue(file.exists())

    def test_pagination_keeps_files_bucket_after_all_folders(self):
        for number in range(255):
            (self.path / f"folder-{number:03}").mkdir()
        self.file("direct", 200)
        self.scan()
        first = self.index.children(self.db, 1)
        self.assertTrue(first["more"])
        self.assertTrue(all(n["kind"] == "dir" for n in first["nodes"]))
        last = self.index.children(self.db, 1, offset=250)
        self.assertFalse(last["more"])
        self.assertEqual(last["nodes"][-1]["kind"], "files")

    def test_treemap_budget_preserves_total_and_breadcrumbs(self):
        for number in range(90):
            self.file(f"folder-{number}/file", number + 1)
        self.scan()
        value = self.index.map(self.db, 1, "size")
        self.assertEqual(sum(child["size"] for child in value["tiles"]), value["size"])
        ident = self.ident(self.path / "folder-2")
        crumbs = self.index.ancestors(self.db, -ident)
        self.assertEqual([n["name"] for n in crumbs][1:], ["folder-2", "Files"])

    def test_browse_shows_only_directories_and_never_follows_links(self):
        self.file("directory/file", 10)
        self.file("plain", 20)
        (self.path / "link").symlink_to(self.path / "directory")
        self.assertEqual([n["name"] for n in browse(str(self.path))["folders"]], ["directory"])
        with self.assertRaises(OSError):
            open_directory(os.fsencode(self.path / "link"))

    def test_cancellation_invalidates_partial_scan(self):
        self.file("one", 4)
        original = self.index.check_cancel
        calls = 0
        def cancel_early():
            nonlocal calls
            calls += 1
            if calls == 2:
                self.index.cancel.set()
            original()
        with patch.object(self.index, "check_cancel", cancel_early):
            with self.assertRaises(Cancelled):
                self.index.scan(str(self.path))
        self.assertFalse(self.index.ready)

    def test_pipe_protocol_and_eof_cleanup(self):
        self.file("folder/file", 20)
        proc = subprocess.Popen([sys.executable, "-I", str(Path(__file__).resolve().parents[1] / "tools" / "disk_scan.py")],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(lambda: proc.kill() if proc.poll() is None else None)
        requests = [{"op": "scan", "request": 1, "path": str(self.path)},
                    {"op": "children", "request": 2, "node": 1},
                    {"op": "view", "request": 3, "node": 1}]
        def exchange(request):
            proc.stdin.write(json.dumps(request).encode() + b"\n")
            proc.stdin.flush()
            while True:
                self.assertTrue(select.select([proc.stdout], [], [], 5)[0])
                response = json.loads(proc.stdout.readline())
                if "event" not in response:
                    break
                self.assertEqual(request["op"], "scan", "Deletion must not start another scan")
            self.assertEqual(response["request"], request["request"])
            self.assertTrue(response["ok"], response)
            return response

        for request in requests:
            response = exchange(request)
            if request["op"] == "children":
                folder_id = response["nodes"][0]["id"]
        removed = exchange({"op": "delete", "request": 4, "node": folder_id, "view": folder_id})
        self.assertEqual(removed["root"]["files"], 0)
        self.assertEqual(removed["entries"], 1)
        self.assertEqual(removed["view"], 1)
        self.assertEqual(exchange({"op": "children", "request": 5, "node": 1})["nodes"], [])
        self.assertEqual(exchange({"op": "view", "request": 6, "node": 1})["map"]["size"], 0)
        proc.stdin.close()
        self.assertEqual(proc.wait(timeout=5), 0)
        self.assertEqual(proc.stderr.read(), b"")
        proc.stdout.close()
        proc.stderr.close()


if __name__ == "__main__":
    unittest.main()
