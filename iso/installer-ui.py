#!/usr/bin/env python3
"""Portable, keyboard-only presentation for installer.sh.

The shell owns device discovery, validation and all writes. This process only
returns a supplied menu tag, a confirmation, or a read-only checksum result.
Exit 10 means return to Actions; exit 2 asks the shell to use dialog instead.
No graphics protocols, terminal probes, custom fonts or third-party Python libs.
"""
import argparse
import curses
import functools
import locale
import os
from pathlib import Path
import re
import subprocess
import sys
import textwrap
import unicodedata


BACK = 10
UNAVAILABLE = 2
ACTIONS = {
    "wipe": ("Restore drive", "Erase whole disk",
             "Restore the snapshot onto a whole drive. The selected drive will "
             "be erased and recreated with a new ext4 filesystem.",
             "Choose drive", "Review target", "Confirm restore"),
    "verify": ("Check snapshot", "Optional check",
               "Check the snapshot against its SHA-256 checksum. This reads "
               "the installation media and does not change any target disk.",
               "Read snapshot", "Compare checksum", "View result"),
    "free": ("Use free space", "Keep partitions",
             "Create new Linux and EFI partitions in an unused region of a "
             "GPT disk. Existing partitions are kept.",
             "Choose free region", "Review layout", "Confirm restore"),
    "parts": ("Use partitions", "Pick ROOT + EFI",
              "Restore into prepared partitions. Only the selected ROOT and "
              "dedicated EFI partition will be formatted.",
              "Choose ROOT + EFI", "Review layout", "Confirm restore"),
}
ASCII_LOGO = (
    "X   X EEEEE TTTTT  AAA  L      EEEEE N   N  GGGG  III N   N EEEEE",
    " X X  E       T   A   A L      E     NN  N G       I  NN  N E",
    "  X   EEEE    T   AAAAA L      EEEE  N N N G  GG   I  N N N EEEE",
    " X X  E       T   A   A L      E     N  NN G   G   I  N  NN E",
    "X   X EEEEE   T   A   A LLLLL  EEEEE N   N  GGGG  III N   N EEEEE",
)
# The compact wordmark is used when the supplied ASCII art cannot fit in full.
# Full blocks are part of the standard Linux console font; no image renderer
# or unusual Unicode glyphs are needed. Every letter has an explicit gutter.
LOGO_GLYPHS = {
    "X": ("#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"),
    "E": ("#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"),
    "T": ("#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "),
    "A": (" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"),
    "L": ("#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"),
    "N": ("#   #", "##  #", "##  #", "# # #", "#  ##", "#  ##", "#   #"),
    "G": (" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ### "),
    "I": ("#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"),
}
ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def clean(text):
    """Device labels are text, never terminal commands or dialog markup."""
    text = re.sub(r"\\Z[0-9bBrRuUnN]", "", text.replace("\\n", "\n"))
    return "".join(c for c in ANSI.sub("", text)
                   if c == "\n" or not unicodedata.category(c).startswith("C"))


def clip(text, width):
    result, cells = [], 0
    for char in text:
        n = 0 if unicodedata.combining(char) else (2 if unicodedata.east_asian_width(char) in "WF" else 1)
        if cells + n > width:
            break
        result.append(char)
        cells += n
    return "".join(result)


def lines(text, width):
    return [line for paragraph in clean(text).splitlines()
            for line in (textwrap.wrap(paragraph, max(1, width)) or [""])]


@functools.lru_cache(maxsize=8)
def logo(width, height, utf8):
    artwork = ascii_logo()
    if artwork and len(artwork) <= height and max(map(len, artwork)) <= width:
        # Keep the original characters and spacing, including the descender.
        # The wordmark fades from red through yellow to green; its tail is green.
        shades = (9, 9, 1, 3, 11, 11, 10, 2, 2, 2, 2)
        return [[(c, shades[min(i, len(shades) - 1)], False) for c in row]
                for i, row in enumerate(artwork)]
    if utf8 and width >= 68 and height >= 7:
        shades = (9, 1, 3, 11, 2, 10, 2)
        rows = ["    ".join(" ".join(LOGO_GLYPHS[c][row] for c in word)
                            for word in ("XETAL", "ENGINE")) for row in range(7)]
        scale = 2 if width >= len(rows[0]) * 2 else 1
        return [[("\u2588" if c == "#" else " ", shades[i], False)
                 for c in row for _ in range(scale)] for i, row in enumerate(rows)]
    if width >= 65 and height >= 5:
        return [[(c, (1, 1, 3, 2, 2)[i], True) for c in row] for i, row in enumerate(ASCII_LOGO)]
    return [[(c, 2, True) for c in "XETAL ENGINE"[:width]]]


@functools.lru_cache(maxsize=1)
def ascii_logo():
    try:
        return Path(__file__).with_name("installer-logo.txt").read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeError):
        return []


def print_logo(width, height, colors):
    """Use the same branding for the shell's installation progress screen."""
    utf8 = "UTF-8" in locale.getencoding().upper().replace("UTF8", "UTF-8")
    art = logo(max(1, width), max(1, height), utf8)
    for row in art:
        if not row:
            print()
            continue
        shade = row[0][1]
        text = "".join(c for c, _, _ in row)
        if colors >= 8:
            code = f"1;{30 + shade % 8}" if shade >= 8 or row[0][2] else str(30 + shade)
            text = f"\x1b[{code}m{text}\x1b[0m"
        print(text)


class Palette:
    def __init__(self):
        self.colors = curses.COLORS if curses.has_colors() else 0
        self.pairs = {}
        self.bg = 233 if self.colors >= 256 else 0
        self.panel = 235 if self.colors >= 256 else 0
        self.raised = 237 if self.colors >= 256 else 0
        self.fg = 252 if self.colors >= 256 else 7
        self.muted = 245 if self.colors >= 256 else 7
        self.accent = 84 if self.colors >= 256 else 2
        self.warning = 215 if self.colors >= 256 else 3

    def attr(self, fg=None, bg=None, bold=False, reverse=False):
        attr = curses.A_BOLD if bold else 0
        if reverse:
            attr |= curses.A_REVERSE
        if not self.colors:
            return attr
        fg = self.fg if fg is None else fg
        bg = self.bg if bg is None else bg
        if fg >= self.colors:
            fg %= 8
            attr |= curses.A_BOLD
        key = (fg, bg)
        if key not in self.pairs:
            number = len(self.pairs) + 1
            if number >= min(curses.COLOR_PAIRS, 256):
                return attr
            curses.init_pair(number, fg, bg)
            self.pairs[key] = number
        return attr | curses.color_pair(self.pairs[key])


class InstallerUI:
    def __init__(self, screen, args):
        self.screen, self.args = screen, args
        self.palette = Palette()
        self.utf8 = "UTF-8" in locale.getencoding().upper().replace("UTF8", "UTF-8")
        self.actions = ["wipe", "verify"] + (["free", "parts"] if args.uefi else [])
        self.items = list(zip(args.items[::2], args.items[1::2]))
        if args.kind == "home":
            self.actions = [tag for tag, _ in self.items if tag in ACTIONS]
        self.mode = args.mode if args.mode in self.actions else self.actions[0]
        self.index = self.actions.index(self.mode) if args.kind == "home" else 0
        self.button = 0  # Every confirmation starts on Back, never on approval.
        self.scroll = 0
        self.max_scroll = 0
        self.page = 1
        self.result = ""

    def put(self, y, x, text, attr=0, width=None):
        rows, cols = self.screen.getmaxyx()
        if not 0 <= y < rows or not 0 <= x < cols - 1:
            return
        text = clean(text).replace("\n", " ")
        if not self.utf8:
            text = text.encode("ascii", "replace").decode("ascii")
        text = clip(text, min(cols - x - 1, width if width is not None else cols))
        try:
            self.screen.addstr(y, x, text, attr)
        except curses.error:
            # A resize can occur between measuring and drawing a cell.
            pass

    def fill(self, y, x, h, w, bg):
        for row in range(y, y + max(0, h)):
            self.put(row, x, " " * max(0, w), self.palette.attr(bg=bg))

    def paragraph(self, y, x, text, width, limit, attr):
        wrapped = lines(text, width)
        for i, line in enumerate(wrapped[:max(0, limit)]):
            self.put(y + i, x, line, attr, width)
        return min(len(wrapped), max(0, limit))

    def draw(self):
        p = self.palette
        rows, cols = self.screen.getmaxyx()
        self.screen.bkgd(" ", p.attr())
        self.screen.erase()
        if cols < 40 or rows < 20:
            self.put(1, 1, "XETAL ENGINE", p.attr(p.accent, bold=True))
            self.put(3, 1, "Resize to at least 40 x 20.")
            self.put(5, 1, "Esc: back / exit")
            self.screen.refresh()
            return False
        width = min(cols - 4, 170)
        x = (cols - width) // 2
        tall = rows >= 30 and width >= 76
        if tall:
            # Reserve at least 14 rows for controls. Never crop or reflow the
            # supplied artwork to make room for the caption or a shorter screen.
            artwork = logo(width, rows - 23, self.utf8)
            for y, row in enumerate(artwork, 1):
                for column, (char, color, bold) in enumerate(row):
                    self.put(y, x + column, char, p.attr(color, bold=bold))
            top = len(artwork) + 2
            if max(map(len, artwork)) + 34 <= width:
                self.put(2, x + width - 30, "SYSTEM INSTALLER", p.attr(p.accent, bold=True))
                self.put(4, x + width - 30, "Your system. A new beginning.", p.attr(p.muted))
        else:
            self.put(1, x, "XETAL ENGINE", p.attr(p.accent, bold=True))
            self.put(1, x + 16, "/ SYSTEM INSTALLER", p.attr(p.muted))
            top = 3
        firmware = "UEFI" if self.args.uefi else "BIOS"
        status = f"{firmware}  /  Snapshot: {self.args.snapshot}"
        if width < 64:
            status = f"{firmware} / {self.args.snapshot}"
        badge = "DUMMY PREVIEW" if self.args.dummy else "SYSTEM RESTORE"
        self.put(top, x, status, p.attr(p.muted), width - len(badge) - 2)
        self.put(top, x + width - len(badge), badge, p.attr(p.warning if self.args.dummy else p.accent, bold=True))
        self.put(top + 1, x, "-" * width, p.attr(p.accent))
        y, height = top + 3, rows - top - 7
        self.split = width >= 76
        self.content = (y, x, height, width)
        if self.split:
            left = 26 if width >= 104 else 22
            self.fill(y, x, height, left, p.panel)
            self.draw_actions(y, x, height, left)
            if p.colors < 256:
                for row in range(y + 1, y + height - 1):
                    self.put(row, x + left, "|", p.attr(p.muted))
            self.content = (y, x + left + 2, height, width - left - 2)
        elif self.args.kind == "home":
            self.fill(y, x, height, width, p.panel)
            self.draw_actions(y, x, height, width)
        cy, cx, ch, cw = self.content
        if self.split or self.args.kind != "home":
            self.fill(cy, cx, ch, cw, p.panel)
            if self.args.kind == "home":
                self.draw_overview(cy, cx, ch, cw)
            elif self.args.kind == "menu":
                self.draw_menu(cy, cx, ch, cw)
            else:
                self.draw_text(cy, cx, ch, cw)
        if self.args.kind == "home":
            hint = "Up/Down  Choose action    Enter/Right  Open    Esc  Shell"
        elif self.args.kind == "menu":
            hint = "Up/Down  Choose    Enter  Select    Left/Tab/Esc  Actions"
        elif self.args.kind == "confirm":
            hint = "Left/Right/Tab  Buttons    Enter  Select    Up/Down  Scroll    Esc  Back"
        elif self.args.kind == "verify":
            hint = "Checking snapshot...    Esc  Cancel check"
        else:
            hint = "Enter  Continue    Up/Down  Scroll"
        if width < 70:
            hint = {"home": "Up/Down: choose  Enter: open  Esc: shell",
                    "menu": "Up/Down: choose  Enter: select  Esc: back",
                    "confirm": "Tab: button  Enter: select  Esc: back"}.get(self.args.kind, "Enter: continue  Up/Down: scroll")
        self.put(rows - 2, x, hint, p.attr(p.muted), width)
        self.screen.refresh()
        return True

    def draw_actions(self, y, x, height, width):
        p = self.palette
        attr = p.attr(p.muted, p.panel)
        self.put(y + 1, x + 2, "01 / ACTIONS", attr)
        step = 3 if height >= 18 else (2 if height >= 12 else 1)
        for i, tag in enumerate(self.actions):
            row = y + 3 + i * step
            if row >= y + height - 1:
                break
            selected = tag == self.mode
            active = selected and self.args.kind == "home"
            fg, bg = (0, p.accent) if active and p.colors else (p.accent if selected else p.fg, p.panel)
            a = p.attr(fg, bg, bold=selected, reverse=active and not p.colors)
            self.put(row, x + 1, " " * (width - 2), a)
            self.put(row, x + 2, ("> " if selected else "  ") + ACTIONS[tag][0], a, width - 4)
            if step == 3:
                self.put(row + 1, x + 4, ACTIONS[tag][1], attr, width - 5)
        if height >= 23:
            self.put(y + height - 4, x + 2, "KEYBOARD CONTROL", attr, width - 4)
            hint = "Enter to open  >" if self.args.kind == "home" else "<  Esc to actions"
            self.put(y + height - 2, x + 2, hint, p.attr(p.accent, p.panel), width - 4)

    def draw_overview(self, y, x, height, width):
        p = self.palette
        action = ACTIONS[self.mode]
        title = "VERIFY INSTALLATION MEDIA" if self.mode == "verify" else "02 / CONFIGURE & REVIEW"
        self.put(y + 1, x + 2, title, p.attr(p.accent, p.panel), width - 4)
        self.put(y + 3, x + 2, action[0], p.attr(bg=p.panel, bold=True), width - 4)
        used = self.paragraph(y + 5, x + 2, action[2], min(width - 4, 76), max(1, height - 10), p.attr(bg=p.panel))
        start = y + 6 + used
        if height >= 20:
            for i, step in enumerate(action[3:]):
                self.put(start + i * 2, x + 3, f"0{i + 1}  {step}", p.attr(p.muted, p.panel), width - 6)
        note = "Preview mode: disk changes are simulated." if self.args.dummy else "Review the target before confirming changes."
        self.put(y + height - 3, x + 2, note, p.attr(p.warning, p.panel), width - 4)
        if self.mode in ("free", "parts"):
            note = "Partition modes by devvz (PR #7)"
        else:
            note = "Snapshot verification is optional."
        self.put(y + height - 2, x + 2, note, p.attr(p.muted, p.panel), width - 4)

    def draw_menu(self, y, x, height, width):
        p = self.palette
        self.put(y + 1, x + 2, "02 / " + clean(self.args.title).strip().upper(), p.attr(p.accent, p.panel), width - 4)
        used = self.paragraph(y + 3, x + 2, self.args.text, width - 4, min(3, max(1, height - 7)), p.attr(p.muted, p.panel))
        start = y + 4 + used
        step = 3 if height >= 20 else 2
        self.page = max(1, (y + height - start - 2) // step)
        first = max(0, self.index - self.page + 1)
        for i in range(first, min(len(self.items), first + self.page)):
            tag, description = self.items[i]
            # Keep capacity and model visible even when filesystem labels are
            # long. The confirmation screen shows the complete metadata.
            if " | " in description:
                labels, model = description.rsplit(" | ", 1)
                description = f"{model}  /  {labels}"
            row = start + (i - first) * step
            selected = i == self.index
            fg, bg = (0, p.accent) if selected and p.colors else (p.fg, p.raised if selected else p.panel)
            a = p.attr(fg, bg, bold=selected, reverse=selected and not p.colors)
            self.put(row, x + 1, " " * (width - 2), a)
            self.put(row, x + 2, ("> " if selected else "  ") + tag, a, width - 4)
            self.put(row + 1, x + 4, description, p.attr(p.fg if selected else p.muted, p.panel), width - 6)
        self.put(y + height - 1, x + 2, f"{self.index + 1} / {len(self.items)}   Enter to review selection", p.attr(p.muted, p.panel), width - 4)

    def draw_text(self, y, x, height, width):
        p = self.palette
        warning = self.args.kind == "confirm"
        self.put(y + 1, x + 2, clean(self.args.title).strip().upper(), p.attr(p.warning if warning else p.accent, p.panel, bold=True), width - 4)
        text = lines(self.args.text, width - 4)
        self.page = max(1, height - 7)
        self.max_scroll = max(0, len(text) - self.page)
        self.scroll = min(self.scroll, self.max_scroll)
        for i, line in enumerate(text[self.scroll:self.scroll + self.page]):
            self.put(y + 3 + i, x + 2, line, p.attr(bg=p.panel), width - 4)
        if self.max_scroll:
            self.put(y + height - 4, x + 2, f"Text {self.scroll + 1}-{min(len(text), self.scroll + self.page)} / {len(text)}   Up/Down to scroll", p.attr(p.muted, p.panel), width - 4)
        if self.args.kind == "verify":
            return
        buttons = ["Back", self.args.accept] if warning else ["Continue"]
        bx = x + 2
        for i, label in enumerate(buttons):
            active = i == self.button
            fg, bg = (0, p.accent) if active and p.colors else (p.fg, p.raised)
            text = f"[ {label} ]"
            self.put(y + height - 2, bx, text, p.attr(fg, bg, bold=active, reverse=active and not p.colors), width - (bx - x) - 1)
            bx += len(text) + 3

    def handle(self, key, usable=True):
        if key in (27, ord("q")):
            if self.args.kind == "message":
                return 0
            return 1 if self.args.kind == "home" else BACK
        if not usable:
            return None
        up, down = key in (curses.KEY_UP, ord("k")), key in (curses.KEY_DOWN, ord("j"))
        enter = key in (10, 13, curses.KEY_ENTER)
        if self.args.kind in ("home", "menu"):
            count = len(self.actions) if self.args.kind == "home" else len(self.items)
            if up or down:
                self.index = (self.index + (1 if down else -1)) % count
            elif key in (curses.KEY_NPAGE, curses.KEY_PPAGE):
                self.index = min(count - 1, max(0, self.index + (self.page if key == curses.KEY_NPAGE else -self.page)))
            elif key in (curses.KEY_HOME, curses.KEY_END):
                self.index = 0 if key == curses.KEY_HOME else count - 1
            elif self.args.kind == "menu" and key in (curses.KEY_LEFT, 9, curses.KEY_BTAB):
                return BACK
            elif enter or (self.args.kind == "home" and key in (curses.KEY_RIGHT, 9)):
                self.result = self.actions[self.index] if self.args.kind == "home" else self.items[self.index][0]
                return 0
            if self.args.kind == "home":
                self.mode = self.actions[self.index]
        else:
            if up or down:
                self.scroll = min(self.max_scroll, max(0, self.scroll + (1 if down else -1)))
            elif key in (curses.KEY_NPAGE, curses.KEY_PPAGE):
                self.scroll = min(self.max_scroll, max(0, self.scroll + (self.page if key == curses.KEY_NPAGE else -self.page)))
            elif self.args.kind == "confirm" and key in (9, curses.KEY_BTAB, curses.KEY_LEFT, curses.KEY_RIGHT):
                self.button = 1 - self.button
            elif enter:
                return 0 if self.args.kind == "message" or self.button == 1 else BACK
        return None

    def verify(self):
        self.args.title = "Checking snapshot"
        self.args.text = "Reading the complete snapshot and comparing its SHA-256 checksum.\n\nThis may take several minutes. No disks are being changed."
        self.draw()
        process = None
        outcome = "failed"
        try:
            clone = Path(self.args.clone_dir)
            if (clone / "snapshot.sha256").is_file() and (clone / "rootfs-snapshot.tar.zst").is_file():
                process = subprocess.Popen(["sha256sum", "--check", "--strict", "snapshot.sha256"], cwd=clone,
                                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                self.screen.timeout(150)
                while process.poll() is None:
                    key = self.screen.getch()
                    if key in (27, ord("q")):
                        outcome = "cancelled"
                        break
                    if key == curses.KEY_RESIZE:
                        self.draw()
                else:
                    outcome = "passed" if process.returncode == 0 else "failed"
        except OSError:
            pass
        finally:
            if process is not None and process.poll() is None:
                process.terminate()
                process.wait()
            self.screen.timeout(-1)
        self.args.kind = "message"
        self.args.title = "Snapshot check " + outcome
        self.args.text = {
            "passed": "Snapshot integrity check passed.\n\nThe snapshot matches its checksum. Return to Actions to choose an installation mode.",
            "failed": "The snapshot could not be verified.\n\nInstallation is blocked until a check passes. Recreate the ISO or installation media if the problem persists.",
            "cancelled": "The check was cancelled. The previous integrity status is unchanged.\n\nNo disks were changed.",
        }[outcome]
        if outcome != "cancelled":
            self.args.snapshot = outcome
        self.result = outcome

    def run(self):
        self.screen.keypad(True)
        try:
            curses.curs_set(0)
        except curses.error:
            pass
        curses.set_escdelay(40)
        if self.args.kind == "confirm":
            curses.flushinp()  # Do not carry Enter/typeahead across warnings.
        if self.args.kind == "verify":
            self.verify()
        while True:
            usable = self.draw()
            status = self.handle(self.screen.getch(), usable)
            if status is not None:
                return status


def arguments(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=("home", "menu", "confirm", "message", "verify", "logo"))
    parser.add_argument("--mode", choices=tuple(ACTIONS), default="wipe")
    parser.add_argument("--uefi", action="store_true")
    parser.add_argument("--dummy", action="store_true")
    parser.add_argument("--snapshot", default="not checked")
    parser.add_argument("--clone-dir", default=str(Path(__file__).resolve().parent))
    parser.add_argument("--title", default="")
    parser.add_argument("--text", default="")
    parser.add_argument("--accept", default="Continue")
    parser.add_argument("--width", type=int, default=110)
    parser.add_argument("--height", type=int, default=11)
    parser.add_argument("--colors", type=int, default=0)
    parser.add_argument("items", nargs="*")
    args = parser.parse_args(argv)
    if args.kind in ("home", "menu") and (not args.items or len(args.items) % 2):
        parser.error("menus need tag/description pairs")
    if args.kind == "home" and not any(tag in ACTIONS for tag in args.items[::2]):
        parser.error("no supported actions")
    return args


def main(argv=None):
    args = arguments(argv)
    if args.kind == "logo":
        print_logo(args.width, args.height, args.colors)
        return 0
    if not os.isatty(0) or os.environ.get("TERM", "dumb") == "dumb":
        return UNAVAILABLE
    try:
        locale.setlocale(locale.LC_ALL, "")
        tty = os.open(os.ttyname(0), os.O_RDWR | os.O_NOCTTY)
    except (OSError, locale.Error):
        return UNAVAILABLE
    result_fd = os.dup(1)
    try:
        # stdout is normally a Bash command substitution. Draw only on the TTY;
        # send the selected tag, and nothing else, back through the original fd.
        sys.stdout.flush()
        os.dup2(tty, 1)
        ui = None

        def run(screen):
            nonlocal ui
            ui = InstallerUI(screen, args)
            return ui.run()

        status = curses.wrapper(run)
        if status == 0 and ui.result:
            os.write(result_fd, (ui.result + "\n").encode("utf-8"))
        return status
    except KeyboardInterrupt:
        return 130
    except (curses.error, OSError, ValueError):
        return UNAVAILABLE
    finally:
        os.dup2(result_fd, 1)
        os.close(result_fd)
        os.close(tty)


if __name__ == "__main__":
    sys.exit(main())
