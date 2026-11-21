#!/usr/bin/env python3
"""Private, pipe-owned disk index for the Control Panel (normally run via sudo).

Only this helper is elevated, never Qt. Filenames remain bytes internally; the
UI refers to opaque index IDs, not shell commands. The disposable SQLite index
keeps million-file scans out of GUI memory. EOF cancels work and removes it.
"""
import json
import os
import re
import sqlite3
import stat
import sys
import tempfile
import threading
import time
from contextlib import closing, suppress

VIRTUAL = {b"proc", b"sysfs", b"devtmpfs", b"devpts", b"cgroup", b"cgroup2",
           b"securityfs", b"debugfs", b"tracefs", b"pstore", b"configfs",
           b"fusectl", b"mqueue", b"hugetlbfs", b"autofs", b"binfmt_misc"}
PROTECTED = {b"/", b"/boot", b"/bin", b"/sbin", b"/lib", b"/lib64", b"/etc",
             b"/usr", b"/var", b"/home", b"/root", b"/opt", b"/srv", b"/mnt", b"/media"}
OPEN_DIR = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC


def display(value):
    return value.decode("utf-8", "replace") if isinstance(value, bytes) else value


def beneath(path, root):
    return path == root or path.startswith(root.rstrip(b"/") + b"/")


def mount_points():
    result = {}
    with open("/proc/self/mountinfo", "rb") as source:
        for line in source:
            left, right = line.rstrip().split(b" - ", 1)
            unescape = lambda value: re.sub(rb"\\([0-7]{3})", lambda m: bytes([int(m[1], 8)]), value)
            result[unescape(left.split()[4])] = right.split()[0]
    return result


def open_directory(path):
    """Walk every component relative to an open fd; never traverse a symlink."""
    if not path.startswith(b"/") or b".." in path.split(b"/"):
        raise ValueError("Choose an absolute folder path without '..'.")
    fd = os.open(b"/", OPEN_DIR)
    try:
        for component in path.split(b"/"):
            if component in (b"", b"."):
                continue
            child = os.open(component, OPEN_DIR, dir_fd=fd)
            os.close(fd)
            fd = child
        return fd
    except BaseException:
        os.close(fd)
        raise


class Cancelled(Exception):
    pass


class DiskIndex:
    def __init__(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="cachyostools-disk-")
        self.filename = self.temporary.name + "/index.sqlite"
        self.cancel = threading.Event()
        self.ready = False
        self.root = b"/"
        self.mounts = {}
        self.count = self.errors = 0
        self.samples = []

    def close(self):
        self.temporary.cleanup()

    def connect(self):
        db = sqlite3.connect(self.filename)
        db.row_factory = sqlite3.Row
        db.execute("PRAGMA cache_size=-8192")
        return db

    def check_cancel(self):
        if self.cancel.is_set():
            raise Cancelled("Scan stopped. Choose Scan to build a fresh map.")

    def scan(self, path, cross=False, progress=lambda *_: None):
        self.ready = False
        self.root = os.path.normpath(os.fsencode(path))
        self.mounts = mount_points()
        self.count = self.errors = 0
        self.samples = []
        self.cancel.clear()
        if any(beneath(self.root, p) for p, fs in self.mounts.items() if fs in VIRTUAL):
            raise ValueError("Choose a storage filesystem, not a virtual kernel filesystem.")
        root_fd = open_directory(self.root)
        started = time.monotonic()
        last_progress = started
        seen_links = set()
        ancestors = set()
        db = self.connect()
        db.set_progress_handler(lambda: int(self.cancel.is_set()), 2000)
        try:
            db.executescript("""
                PRAGMA journal_mode=OFF;
                PRAGMA synchronous=OFF;
                DROP TABLE IF EXISTS nodes;
                CREATE TABLE nodes (
                    id INTEGER PRIMARY KEY, parent INTEGER, name BLOB, path BLOB,
                    kind TEXT, size INTEGER, allocated INTEGER, files INTEGER,
                    dirs INTEGER, dev TEXT, ino TEXT, mtime TEXT, mode INTEGER,
                    status TEXT, direct_size INTEGER DEFAULT 0,
                    direct_allocated INTEGER DEFAULT 0, direct_files INTEGER DEFAULT 0,
                    own_allocated INTEGER DEFAULT 0, linked INTEGER DEFAULT 0);
            """)

            def problem(path, error):
                self.errors += 1
                if len(self.samples) < 6:
                    self.samples.append(display(path) + ": " + str(error))

            def visit(path, parent, entry_stat, fd=None, note="", depth=0):
                nonlocal last_progress
                self.check_cancel()
                isdir = stat.S_ISDIR(entry_stat.st_mode)
                kind = "dir" if isdir else "link" if stat.S_ISLNK(entry_stat.st_mode) else "file"
                size = 0 if isdir else entry_stat.st_size
                allocated = entry_stat.st_blocks * 512
                identity = (entry_stat.st_dev, entry_stat.st_ino)
                if not isdir and not stat.S_ISREG(entry_stat.st_mode) and kind != "link":
                    size = allocated = 0
                    note = "Special file"
                linked = stat.S_ISREG(entry_stat.st_mode) and entry_stat.st_nlink > 1
                if linked:
                    if identity in seen_links:
                        size = allocated = 0
                        note = "Hard link · space counted once"
                    seen_links.add(identity)
                row = db.execute("""INSERT INTO nodes
                    (parent,name,path,kind,size,allocated,files,dirs,dev,ino,mtime,mode,status,own_allocated,linked)
                    VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
                    (parent, os.path.basename(path) or b"/", path, kind, size, allocated,
                     0 if isdir else 1, 0, str(entry_stat.st_dev), str(entry_stat.st_ino),
                     str(entry_stat.st_mtime_ns), entry_stat.st_mode, note,
                     allocated if isdir else 0, linked))
                ident = row.lastrowid
                self.count += 1
                now = time.monotonic()
                if now - last_progress > .25:
                    progress({"entries": self.count, "path": display(path), "errors": self.errors})
                    last_progress = now
                if not isdir or fd is None:
                    return ident, size, allocated, 0 if isdir else 1, 0
                direct_size = direct_allocated = direct_files = files = dirs = 0
                ancestors.add(identity)
                try:
                    with os.scandir(fd) as entries:
                        for entry in entries:
                            self.check_cancel()
                            name = os.fsencode(entry.name)
                            child = os.path.join(path, name)
                            if beneath(child, os.fsencode(self.temporary.name)):
                                continue
                            child_fd = None
                            try:
                                info = os.stat(name, dir_fd=fd, follow_symlinks=False)
                                directory = stat.S_ISDIR(info.st_mode)
                                status = ""
                                if directory:
                                    if child in self.mounts and (not cross or self.mounts[child] in VIRTUAL):
                                        status = "Mounted filesystem · skipped"
                                    elif (info.st_dev, info.st_ino) in ancestors:
                                        status = "Directory loop · skipped"
                                    elif depth >= 240:
                                        status = "Directory nesting limit · skipped"
                                        problem(child, status)
                                    else:
                                        try:
                                            child_fd = os.open(name, OPEN_DIR, dir_fd=fd)
                                            actual = os.fstat(child_fd)
                                            if (actual.st_dev, actual.st_ino) != (info.st_dev, info.st_ino):
                                                raise OSError("Folder changed during scan")
                                        except OSError as error:
                                            if child_fd is not None:
                                                os.close(child_fd)
                                                child_fd = None
                                            status = str(error)
                                            problem(child, error)
                                child_id, cs, ca, cf, cd = visit(child, ident, info, child_fd, status, depth + 1)
                                # A mount belongs to its own filesystem, not its parent's usage.
                                if status.startswith("Mounted filesystem"):
                                    ca = 0
                                    db.execute("UPDATE nodes SET allocated=0,own_allocated=0 WHERE id=?", (child_id,))
                                size += cs
                                allocated += ca
                                files += cf
                                dirs += cd + int(directory)
                                if not directory:
                                    direct_size += cs
                                    direct_allocated += ca
                                    direct_files += 1
                            except OSError as error:
                                problem(child, error)
                            finally:
                                if child_fd is not None:
                                    os.close(child_fd)
                finally:
                    ancestors.remove(identity)
                db.execute("""UPDATE nodes SET size=?,allocated=?,files=?,dirs=?,
                    direct_size=?,direct_allocated=?,direct_files=? WHERE id=?""",
                    (size, allocated, files, dirs, direct_size, direct_allocated, direct_files, ident))
                return ident, size, allocated, files, dirs

            visit(self.root, None, os.fstat(root_fd), root_fd)
            self.check_cancel()
            progress({"entries": self.count, "path": "Sorting the disk index…", "errors": self.errors})
            db.executescript("""
                CREATE INDEX parent_size ON nodes(parent,kind,size DESC);
                CREATE INDEX parent_allocated ON nodes(parent,kind,allocated DESC);
                CREATE UNIQUE INDEX node_path ON nodes(path);
                CREATE INDEX linked_inode ON nodes(dev,ino) WHERE linked=1;
            """)
            db.commit()
            self.ready = True
            return {"root": self.node(db, 1), "entries": self.count, "errors": self.errors,
                    "samples": self.samples, "seconds": round(time.monotonic() - started, 2)}
        finally:
            db.close()
            os.close(root_fd)

    def node(self, db, ident):
        row = db.execute("SELECT * FROM nodes WHERE id=?", (abs(ident),)).fetchone()
        if row is None:
            raise ValueError("This item is no longer in the scan. Scan again.")
        group = ident < 0
        return {"id": ident, "parent": row["id"] if group else
                (-row["parent"] if row["kind"] != "dir" else row["parent"]),
                "name": "Files" if group else display(row["name"]), "path": display(row["path"]),
                "kind": "files" if group else row["kind"],
                "size": row["direct_size"] if group else row["size"],
                "allocated": row["direct_allocated"] if group else row["allocated"],
                "files": row["direct_files"] if group else row["files"],
                "dirs": 0 if group else row["dirs"], "status": row["status"],
                "children": (row["direct_files"] > 0) if group else
                    bool(row["kind"] == "dir" and (row["dirs"] or row["direct_files"])),
                "utf8": row["path"].decode("utf-8", "replace").encode("utf-8") == row["path"]}

    def children(self, db, ident, metric="allocated", offset=0, limit=250):
        metric = "size" if metric == "size" else "allocated"
        operator = "!=" if ident < 0 else "="
        rows = db.execute(f"SELECT id FROM nodes WHERE parent=? AND kind {operator} 'dir' "
                          f"ORDER BY {metric} DESC,name LIMIT ? OFFSET ?",
                          (abs(ident), limit + 1, max(0, offset))).fetchall()
        more = len(rows) > limit
        result = [self.node(db, row[0]) for row in rows[:limit]]
        if ident > 0 and not more:
            group = self.node(db, -ident)
            if group["files"]:
                result.append(group)
        return {"nodes": result, "more": more, "offset": offset + min(limit, len(rows))}

    def ancestors(self, db, ident):
        result = []
        while ident is not None:
            node = self.node(db, ident)
            result.insert(0, node)
            ident = node["parent"]
        return result

    def map(self, db, ident, metric, depth=0, budget=None):
        if budget is None:
            budget = [1100]
        node = self.node(db, ident)
        if depth >= 3 or not node["children"] or budget[0] <= 0:
            return node
        result = self.children(db, ident, metric, limit=60)
        if ident > 0 and result["more"]:
            direct_files = self.node(db, -ident)
            if direct_files["files"]:
                result["nodes"].append(direct_files)
        children = []
        for child in result["nodes"]:
            if budget[0] <= 0:
                break
            budget[0] -= 1
            children.append(self.map(db, child["id"], metric, depth + 1, budget))
        used = sum(item[metric] for item in children)
        rest = max(0, node[metric] - used)
        # Includes tiny children omitted by the drawing budget and directory
        # metadata. The exact, unabridged listing remains available in the tree.
        if rest:
            children.append({"id": ident, "name": "More / folder metadata", "kind": "remainder",
                             "path": node["path"], "size": rest, "allocated": rest})
        node["tiles"] = children
        return node

    def validate_target(self, db, ident):
        if not self.ready or ident <= 1:
            raise ValueError("Choose an individual file or a subfolder, not the scan root or Files group.")
        row = db.execute("SELECT * FROM nodes WHERE id=?", (ident,)).fetchone()
        if row is None:
            raise ValueError("Item not found. Scan again.")
        path = row["path"]
        mounts = mount_points()
        if (not beneath(path, self.root) or path == self.root or path in PROTECTED
                or any(beneath(path, p) for p in (b"/dev", b"/proc", b"/sys", b"/run"))
                or any(beneath(m, path) for m in mounts)
                or beneath(path, os.fsencode(self.temporary.name))):
            raise ValueError("Scan roots, mount points and essential system folders cannot be removed here.")
        parent = open_directory(os.path.dirname(path))
        try:
            self.verify_identity(row, os.stat(os.path.basename(path), dir_fd=parent, follow_symlinks=False))
        finally:
            os.close(parent)
        return row

    @staticmethod
    def verify_identity(row, current):
        if (str(current.st_dev), str(current.st_ino), current.st_mode, str(current.st_mtime_ns)) != (
                row["dev"], row["ino"], row["mode"], row["mtime"]):
            raise ValueError("This item changed since the scan. Scan again before removing it.")

    @staticmethod
    def adjust_totals(db, parent, size=0, allocated=0, files=0, dirs=0):
        """Only touch the ancestors of the changed branch, not their siblings."""
        db.execute("""WITH RECURSIVE ancestors(id) AS (
            SELECT ? UNION ALL
            SELECT nodes.parent FROM nodes JOIN ancestors ON nodes.id=ancestors.id
                WHERE nodes.parent IS NOT NULL)
            UPDATE nodes SET size=size+?,allocated=allocated+?,files=files+?,dirs=dirs+?
                WHERE id IN ancestors""", (parent, size, allocated, files, dirs))

    def forget_removed(self, db, row, parent_info, view):
        """Remove a successfully deleted branch from the existing scan."""
        parent = db.execute("SELECT * FROM nodes WHERE id=?", (row["parent"],)).fetchone()
        if (parent["dev"], parent["ino"], parent["mode"]) != (
                str(parent_info.st_dev), str(parent_info.st_ino), parent_info.st_mode):
            raise ValueError("Parent folder changed during deletion. Scan again.")
        with db:
            db.execute("CREATE TEMP TABLE removed_nodes(id INTEGER PRIMARY KEY)")
            try:
                db.execute("""WITH RECURSIVE branch(id) AS (
                    SELECT ? UNION ALL
                    SELECT nodes.id FROM nodes JOIN branch ON nodes.parent=branch.id)
                    INSERT INTO removed_nodes SELECT id FROM branch""", (row["id"],))
                # If the counted hard link was removed, its bytes still belong
                # to a surviving alias in the scan. Transfer them before dropping
                # the branch; never count that inode twice or pretend it is free.
                owners = db.execute("""SELECT * FROM nodes WHERE id IN removed_nodes
                    AND linked=1 AND (size>0 OR allocated>0)""")
                for owner in owners:
                    survivor = db.execute("""SELECT id,parent FROM nodes
                        WHERE linked=1 AND dev=? AND ino=? AND id NOT IN removed_nodes
                        ORDER BY id LIMIT 1""", (owner["dev"], owner["ino"])).fetchone()
                    if survivor is None:
                        continue
                    db.execute("UPDATE nodes SET size=?,allocated=?,status='' WHERE id=?",
                               (owner["size"], owner["allocated"], survivor["id"]))
                    db.execute("""UPDATE nodes SET direct_size=direct_size+?,
                        direct_allocated=direct_allocated+? WHERE id=?""",
                               (owner["size"], owner["allocated"], survivor["parent"]))
                    self.adjust_totals(db, survivor["parent"], owner["size"], owner["allocated"])

                self.adjust_totals(db, row["parent"], -row["size"], -row["allocated"],
                                   -row["files"], -row["dirs"] - int(row["kind"] == "dir"))
                if row["kind"] != "dir":
                    db.execute("""UPDATE nodes SET direct_size=direct_size-?,
                        direct_allocated=direct_allocated-?,direct_files=direct_files-1 WHERE id=?""",
                               (row["size"], row["allocated"], row["parent"]))
                db.execute("DELETE FROM nodes WHERE id IN removed_nodes")

                # Deleting an entry changes its parent's mtime and may shrink
                # directory metadata. Refresh just that open directory, so the
                # user can subsequently delete the parent without a new scan.
                allocated = parent_info.st_blocks * 512
                self.adjust_totals(db, row["parent"], allocated=allocated - parent["own_allocated"])
                db.execute("UPDATE nodes SET mtime=?,own_allocated=? WHERE id=?",
                           (str(parent_info.st_mtime_ns), allocated, row["parent"]))
            finally:
                db.execute("DROP TABLE removed_nodes")
        root = self.node(db, 1)
        self.count = root["files"] + root["dirs"] + 1
        viewed = db.execute("SELECT direct_files FROM nodes WHERE id=?", (abs(view),)).fetchone()
        if viewed is None:
            view = row["parent"]
        elif view < 0 and not viewed["direct_files"]:
            view = abs(view)
        return {"deleted": True, "root": root, "entries": self.count, "view": view}

    def remove(self, db, ident, view=1):
        row = self.validate_target(db, ident)
        mounts = mount_points()
        parent = open_directory(os.path.dirname(row["path"]))

        def remove_at(parent_fd, path, expected):
            self.check_cancel()
            name = os.path.basename(path)
            # Temporary files (including GPG sockets) can disappear while we
            # walk the selected branch. Ignore only absence at each filesystem
            # operation; a broad catch could skip the rest of a live directory.
            try:
                current = os.stat(name, dir_fd=parent_fd, follow_symlinks=False)
            except FileNotFoundError:
                return
            self.verify_identity(expected, current)
            if path in mounts:
                raise ValueError("A mounted filesystem cannot be deleted.")
            if not stat.S_ISDIR(current.st_mode):
                with suppress(FileNotFoundError):
                    os.unlink(name, dir_fd=parent_fd)
                return
            try:
                fd = os.open(name, OPEN_DIR, dir_fd=parent_fd)
            except FileNotFoundError:
                return
            try:
                self.verify_identity(expected, os.fstat(fd))
                try:
                    children = os.listdir(fd)
                except FileNotFoundError:
                    # An opened directory can be unlinked too. Still check its
                    # name below so a replacement is never accepted as deleted.
                    children = []
                for child in children:
                    child_path = os.path.join(path, os.fsencode(child))
                    snapshot = db.execute("SELECT * FROM nodes WHERE path=?", (child_path,)).fetchone()
                    if snapshot is None:
                        raise ValueError("New files appeared in this folder. Scan again.")
                    remove_at(fd, child_path, snapshot)
                # Verify the directory entry still names the opened inode.
                try:
                    current_name = os.stat(name, dir_fd=parent_fd, follow_symlinks=False)
                except FileNotFoundError:
                    return
                if (current_name.st_dev, current_name.st_ino) != (current.st_dev, current.st_ino):
                    raise ValueError("Folder changed during deletion. Scan again.")
                with suppress(FileNotFoundError):
                    os.rmdir(name, dir_fd=parent_fd)
            finally:
                os.close(fd)

        try:
            remove_at(parent, row["path"], row)
            return self.forget_removed(db, row, os.fstat(parent), view)
        except BaseException:
            # A partial deletion or an index failure must not leave a usable,
            # misleading snapshot. The UI rescans only on this failure path.
            self.ready = False
            raise
        finally:
            os.close(parent)


def browse(path):
    raw = os.path.normpath(os.fsencode(path))
    fd = open_directory(raw)
    try:
        folders = []
        with os.scandir(fd) as entries:
            for entry in entries:
                if entry.is_dir(follow_symlinks=False):
                    folders.append({"name": display(os.fsencode(entry.name)),
                                    "path": display(os.path.join(raw, os.fsencode(entry.name)))})
        return {"folders": sorted(folders, key=lambda f: f["name"].casefold()), "path": display(raw)}
    finally:
        os.close(fd)


def serve():
    index = DiskIndex()
    output_lock = threading.Lock()
    working = threading.Event()
    worker = None

    def send(value):
        with output_lock:
            try:
                sys.stdout.write(json.dumps(value, ensure_ascii=True, separators=(",", ":")) + "\n")
                sys.stdout.flush()
            except BrokenPipeError:
                index.cancel.set()

    def work(request):
        try:
            operation = request["op"]
            if operation == "scan":
                result = index.scan(request["path"], request.get("cross", False),
                                    lambda p: send({"event": "progress", **p}))
            elif operation == "browse":
                result = browse(request["path"])
            else:
                if not index.ready:
                    raise ValueError("Run a complete scan first.")
                with closing(index.connect()) as db:
                    ident = int(request.get("node", 1))
                    metric = "size" if request.get("metric") == "size" else "allocated"
                    if operation == "children":
                        result = index.children(db, ident, metric, int(request.get("offset", 0)))
                    elif operation == "view":
                        result = {"map": index.map(db, ident, metric),
                                  "crumbs": index.ancestors(db, ident)}
                    elif operation == "trail":
                        result = {"crumbs": index.ancestors(db, ident)}
                    elif operation == "validate":
                        index.validate_target(db, ident)
                        result = {"node": index.node(db, ident)}
                    elif operation == "delete":
                        result = index.remove(db, ident, int(request.get("view", 1)))
                    else:
                        raise ValueError("Unknown disk operation.")
            working.clear()
            send({"request": request["request"], "ok": True, **result})
        except (OSError, ValueError, sqlite3.Error, Cancelled) as error:
            working.clear()
            send({"request": request.get("request"), "ok": False, "error": str(error),
                  "cancelled": isinstance(error, Cancelled) or index.cancel.is_set()})

    try:
        for line in sys.stdin:
            try:
                request = json.loads(line)
                if request.get("op") == "cancel":
                    index.cancel.set()
                    continue
                # Queries are small; only scanning/deletion needs a worker so
                # cancellation and GUI/Hyprland shutdown can interrupt it.
                if working.is_set():
                    send({"request": request.get("request"), "ok": False, "error": "A disk operation is still running."})
                    continue
                if request.get("op") in ("scan", "delete"):
                    if worker:
                        worker.join()
                    working.set()
                    worker = threading.Thread(target=work, args=(request,))
                    worker.start()
                else:
                    work(request)
            except (ValueError, TypeError) as error:
                send({"ok": False, "error": str(error)})
    finally:
        index.cancel.set()
        if worker:
            worker.join()
        index.close()


if __name__ == "__main__":
    serve()
