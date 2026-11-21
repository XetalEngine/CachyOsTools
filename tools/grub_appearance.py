#!/usr/bin/env python3
"""GRUB designs, unprivileged previews, and backed-up appearance transactions.

The desktop prepares artwork as the calling user. The authenticated editor only
publishes a bounded set of assets and a generated presentation block. It never
runs grub-install or regenerates the existing menu.
"""
from __future__ import annotations

import base64
import io
import json
import os
from pathlib import Path
import pwd
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parent))
import grub_editor as grub

ROOT = Path(__file__).resolve().parents[1]
SIZE = (1920, 1080)
GENERATOR = '/etc/grub.d/99_cachyostools_appearance'
BEGIN = '### BEGIN ' + GENERATOR + ' ###'
END = '### END ' + GENERATOR + ' ###'
SIGNATURE = '# CachyOsTools GRUB Appearance v1'
PALETTES = {
    'xetal-dark': ('XETAL Dark', '#27ae60', '#1d1f24', '#f2f4f8'),
    'xetal-red': ('XETAL Red', '#c0392b', '#201a1a', '#fff1ed'),
    'midnight': ('Midnight Blue', '#3498db', '#0f1626', '#e4efff'),
    'matrix': ('Matrix', '#00cc44', '#0a0f0a', '#9dffb0'),
    'solar': ('Solar Light', '#e67e22', '#f5f1e8', '#29231d'),
    'purple': ('Purple Haze', '#9b59b6', '#191221', '#f1e6fa'),
}
NAMES = {key: colors[0] for key, colors in PALETTES.items()}

SLICES = ('nw', 'n', 'ne', 'w', 'c', 'e', 'sw', 's', 'se')
FILES = {'background.png', 'source.png', 'menu.pf2', 'selected.pf2', 'heading.pf2', 'detail.pf2',
         *(f'select_{part}.png' for part in SLICES)}


def presets():
    return [{'id': key, 'name': title, 'options': {
        'preset': key, 'background': '', 'layout': 'left', 'heading': 'CACHYOSTOOLS',
        'subtitle': 'Choose a system to start', 'size': 22, 'dimming': 0, 'contrast': 78,
        'accent': accent, 'text': text, 'base': base, 'hints': True}}
        for key, (title, accent, base, text) in PALETTES.items()]


def normalize(value):
    if not isinstance(value, dict): raise ValueError('Invalid appearance settings.')
    defaults = presets()[0]['options']
    result = {key: value.get(key, default) for key, default in defaults.items()}
    if result['preset'] not in NAMES or result['layout'] not in ('left', 'center', 'right'):
        raise ValueError('Choose a bundled theme and menu position.')
    for key, maximum in (('heading', 64), ('subtitle', 100)):
        text = result[key]
        if not isinstance(text, str) or len(text) > maximum or any(ord(c) < 32 for c in text):
            raise ValueError('Heading and subtitle must be short, single-line text.')
    if type(result['size']) is not int or result['size'] not in (18, 22, 28):
        raise ValueError('Choose a supported menu text size.')
    for key in ('dimming', 'contrast'):
        if type(result[key]) is not int or not 0 <= result[key] <= 95:
            raise ValueError('Picture dimming and menu contrast must be between 0 and 95.')
    for key in ('accent', 'text', 'base'):
        if not isinstance(result[key], str) or not re.fullmatch(r'#[0-9a-fA-F]{6}', result[key]):
            raise ValueError('Choose an RGB color for '+key+'.')
    if type(result['hints']) is not bool: raise ValueError('Invalid navigation hint setting.')
    path = result['background']
    if not isinstance(path, str) or len(path) > 4096 or '\0' in path or (path and not Path(path).is_absolute()):
        raise ValueError('Choose a picture using its full path.')
    return result


def geometry(options):
    # Percentages match theme.txt, including the centered and right-hand layouts.
    left = {'left': 7, 'center': 28, 'right': 50}[options['layout']]
    return {'left': left, 'width': 43, 'top': 36,
            'height': options['size'] + 26, 'spacing': 24, 'border': 8, 'padding': 16}


def theme_text(options):
    o = normalize(options); g = geometry(o)
    def quoted(text): return '"'+text.replace('\\', '\\\\').replace('"', '\\"')+'"'
    labels = [(20, 8, o['heading'], 'CachyOsTools Heading Bold 44', o['text']),
              (28, 4, o['subtitle'], 'CachyOsTools Detail Regular 17', o['text'])]
    if o['hints']:
        labels.append((82, 4, '↑ ↓ Select   Enter Start   E Edit   C Console', 'CachyOsTools Detail Regular 17', o['text']))
    text = f'''{SIGNATURE}
title-text: ""
desktop-image: "background.png"
desktop-image-scale-method: "stretch"
desktop-color: "{o['base']}"
message-font: "CachyOsTools Detail Regular 17"
message-color: "{o['text']}"
message-bg-color: "{o['base']}"
terminal-font: "CachyOsTools Detail Regular 17"
# Keep the console aligned with the wallpaper during boot, E and C.
terminal-left: "0"
terminal-top: "0"
terminal-width: "100%"
terminal-height: "100%"
terminal-border: "16"
'''
    for top, height, title, font, color in labels:
        text += f'''+ label {{
  left = {g['left']}%
  top = {top}%
  width = {g['width']}%
  height = {height}%
  text = {quoted(title)}
  font = "{font}"
  color = "{color}"
}}
'''
    selected_color = '#111820' if sum(bytes.fromhex(o['accent'][1:])) > 360 else '#ffffff'
    text += f'''+ boot_menu {{
  left = {g['left']}%
  top = 36%
  width = {g['width']}%
  height = 43%
  item_font = "CachyOsTools Menu Regular {o['size']}"
  selected_item_font = "CachyOsTools Selected Regular {o['size']}"
  item_color = "{o['text']}"
  selected_item_color = "{selected_color}"
  item_height = {g['height']}
  item_padding = 16
  item_spacing = {g['spacing']}
  icon_width = 0
  icon_height = 0
  selected_item_pixmap_style = "select_*.png"
  scrollbar = false
}}
+ label {{
  id = "__timeout__"
  left = {g['left']}%
  top = 89%
  width = {g['width']}%
  height = 4%
  text = "Starting the default system in %d seconds"
  font = "CachyOsTools Detail Regular 17"
  color = "{o['accent']}"
}}
'''
    return text


def source_image(options, expected=''):
    from PIL import Image, ImageOps
    if not options['background']: return Image.new('RGB', SIZE, options['base']), ''
    path = Path(options['background'])
    descriptor = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_CLOEXEC)
    with os.fdopen(descriptor, 'rb') as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > 40*1024*1024:
            raise ValueError('Choose a regular picture smaller than 40 MiB.')
        data = stream.read(40*1024*1024+1)
    fingerprint = grub.digest(data)
    if expected and fingerprint != expected:
        raise ValueError('The background picture changed. Refresh its preview before applying.')
    with Image.open(io.BytesIO(data)) as opened:
        if opened.width*opened.height > 40_000_000: raise ValueError('Choose a picture of at most 40 megapixels.')
        image = ImageOps.exif_transpose(opened).convert('RGB')
        return ImageOps.fit(image, SIZE, Image.Resampling.LANCZOS), fingerprint


def artwork(options, source):
    from PIL import Image, ImageDraw, ImageEnhance
    art = ImageEnhance.Brightness(source).enhance(1-options['dimming']/100).convert('RGBA')
    g = geometry(options); left = round(SIZE[0]*g['left']/100); width = round(SIZE[0]*g['width']/100)
    base = tuple(bytes.fromhex(options['base'][1:])); maximum = round(options['contrast']*2.55)
    strip = Image.new('RGBA', (SIZE[0], 1)); pixels = strip.load()
    for x in range(SIZE[0]):
        distance = max(left-x, x-(left+width), 0)
        pixels[x, 0] = (*base, round(maximum*max(.08, 1-distance/440)))
    art = Image.alpha_composite(art, strip.resize(SIZE))
    draw = ImageDraw.Draw(art); accent = options['accent']
    draw.rounded_rectangle((left-22, 216, left-16, 289), radius=3, fill=accent)
    draw.line((left, 885, left+width, 885), fill=accent, width=2)
    return art.convert('RGB')


def png(image):
    stream = io.BytesIO(); image.save(stream, format='PNG'); return stream.getvalue()


def preview(options, entries=()):
    from PIL import ImageDraw, ImageFont
    options = normalize(options); source, fingerprint = source_image(options)
    art = artwork(options, source); draw = ImageDraw.Draw(art); g = geometry(options)
    fonts = ROOT/'assets/fonts'
    heading = ImageFont.truetype(str(fonts/'Inter-Bold.otf'), 44)
    detail = ImageFont.truetype(str(fonts/'Inter-Medium.otf'), 17)
    menu = ImageFont.truetype(str(fonts/'Inter-Regular.otf'), options['size'])
    selected = ImageFont.truetype(str(fonts/'Inter-SemiBold.otf'), options['size'])
    left = round(SIZE[0]*g['left']/100); width = round(SIZE[0]*g['width']/100)
    def elide(text, face):
        while text and draw.textlength(text, font=face) > width-34: text = text[:-2]+'…'
        return text
    draw.text((left, 218), elide(options['heading'], heading), font=heading, fill=options['text'])
    draw.text((left, 303), elide(options['subtitle'], detail), font=detail, fill=options['text'])
    entries = entries or ['Arch Linux', 'Advanced options for Arch Linux', 'UEFI Firmware Settings']
    selected_color = '#111820' if sum(bytes.fromhex(options['accent'][1:])) > 360 else '#ffffff'
    for index, entry in enumerate(entries[:max(1, 464//(g['height']+g['spacing']))]):
        top = 389+g['padding']+index*(g['height']+g['spacing'])
        if index == 0:
            draw.rounded_rectangle((left+g['padding'], top, left+width-g['padding'], top+g['height']+2*g['border']),
                                   radius=8, fill=options['accent'])
        draw.text((left+g['padding']+g['border']+4, top+g['border']+(g['height']-options['size'])//2), elide(str(entry)[:200], selected if index == 0 else menu),
                  font=selected if index == 0 else menu, fill=selected_color if index == 0 else options['text'])
    if options['hints']:
        draw.text((left, 895), '↑ ↓ Select   Enter Start   E Edit   C Console', font=detail, fill=options['text'])
    draw.text((left, 970), 'Your existing boot countdown is kept', font=detail, fill=options['accent'])
    art.thumbnail((1120, 630))
    return {'image': base64.b64encode(png(art)).decode(), 'background_sha': fingerprint}


def prepare(options, expected=''):
    from PIL import Image, ImageDraw
    options = normalize(options); source, fingerprint = source_image(options, expected)
    files = {'source.png': png(source), 'background.png': png(artwork(options, source))}
    # GRUB adds the top/bottom slices outside item_height. Keep their sum below
    # item_spacing so the highlight never overlaps the following entry.
    selection = Image.new('RGBA', (24, 24)); draw = ImageDraw.Draw(selection)
    draw.rounded_rectangle((0, 0, 23, 23), radius=8, fill=options['accent'])
    cuts = (0, 8, 16, 24)
    for row, vertical in enumerate(('n', '', 's')):
        for col, horizontal in enumerate(('w', '', 'e')):
            files['select_'+(vertical+horizontal or 'c')+'.png'] = png(selection.crop((cuts[col], cuts[row], cuts[col+1], cuts[row+1])))
    with tempfile.TemporaryDirectory(prefix='cachyostools-grub-fonts-') as directory:
        for name, face, font_name, size in (
                ('menu', 'Regular', 'CachyOsTools Menu', options['size']),
                ('selected', 'SemiBold', 'CachyOsTools Selected', options['size']),
                ('heading', 'Bold', 'CachyOsTools Heading', 44), ('detail', 'Medium', 'CachyOsTools Detail', 17)):
            target = Path(directory)/(name+'.pf2')
            grub.command([grub.executable('grub-mkfont', 'grub2-mkfont'), '--name', font_name, '--size', size,
                          '--output', target, ROOT/'assets/fonts'/('Inter-'+face+'.otf')])
            files[name+'.pf2'] = target.read_bytes()

    return {'files': {name: base64.b64encode(data).decode() for name, data in files.items()}, 'background_sha': fingerprint}


def prepared_as_user(options, expected):
    kwargs = {}
    if os.geteuid() == 0:
        value = os.environ.get('PKEXEC_UID', '')
        if not value.isdecimal() or int(value) == 0:
            raise RuntimeError('Open the Appearance tab as your desktop user to apply a design.')
        account = pwd.getpwuid(int(value))
        kwargs.update(user=account.pw_uid, group=account.pw_gid,
                      extra_groups=os.getgrouplist(account.pw_name, account.pw_gid))
    result = subprocess.run(['/usr/bin/python3', '-I', str(Path(__file__).resolve()), 'prepare'],
        input=grub.encoded({'options': options, 'background_sha': expected}), capture_output=True, timeout=90,
        env={'PATH': '/usr/sbin:/usr/bin:/sbin:/bin', 'LANG': 'C.UTF-8'}, **kwargs)
    if result.returncode:
        try: message = json.loads(result.stdout)['error']
        except (ValueError, KeyError): message = 'Could not prepare this design. Check the preview and try again.'
        raise RuntimeError(message)
    if len(result.stdout) > 32*1024*1024: raise ValueError('The prepared theme is too large.')
    return json.loads(result.stdout)


def appearance_block(fs_uuid, relative):
    if not re.fullmatch(r'[a-zA-Z0-9-]{4,80}', fs_uuid) or not re.fullmatch(r'/[a-zA-Z0-9_./@+-]+', relative):
        raise ValueError('This boot filesystem path cannot be represented safely in GRUB.')
    base = '($cachyostools_theme_device)'+relative
    return f'''{BEGIN}
{SIGNATURE}
if search --no-floppy --fs-uuid --set=cachyostools_theme_device {fs_uuid}; then
  if [ -f {base}/theme.txt ]; then
    insmod all_video
    insmod gfxterm
    insmod gfxmenu
    insmod png
    if loadfont {base}/menu.pf2; then
      loadfont {base}/selected.pf2
      loadfont {base}/heading.pf2
      loadfont {base}/detail.pf2
      set gfxmode=1920x1080,1280x720,1024x768,auto
      if terminal_output gfxterm; then
        # gfxmenu and gfxterm have separate backgrounds. Configure both so
        # Enter, the countdown, the editor and boot errors share the artwork.
        insmod gfxterm_background
        background_image -m stretch {base}/background.png
        set color_normal=white/black
        set color_highlight=black/light-gray
        set theme={base}/theme.txt
        export theme
      fi
    fi
  fi
fi
{END}
'''


def replace_appearance(config, block):
    if config.count(BEGIN) != config.count(END) or config.count(BEGIN) > 1:
        raise ValueError('The saved appearance section needs repair before it can be changed.')
    if BEGIN in config:
        pattern = r'(?m)^'+re.escape(BEGIN)+r'\n.*?^'+re.escape(END)+r'\n?'
        config, count = re.subn(pattern, lambda _: block, config, flags=re.S)
        if count != 1: raise ValueError('Could not locate the appearance section safely.')
        return config
    return config + ('' if not config or config.endswith('\n') else '\n') + block


class AppearanceStore(grub.Store):
    def __init__(self, root=Path('/'), checker=None):
        super().__init__(root, checker)
        self.state = root/'var/lib/cachyostools/grub-appearance'
        self.generator = root/GENERATOR.lstrip('/')
        self.assets = self.config.parent/'themes/cachyostools-editor'

    def safe_paths(self):
        super().safe_paths()
        for path in (self.assets, *self.assets.parents):
            if path == self.root: break
            if path.is_symlink(): raise RuntimeError('A theme destination is a symbolic link. Nothing was changed.')
            if path.exists() and self.root == Path('/'):
                info = path.stat()
                if info.st_uid != 0 or info.st_mode & 0o022: raise RuntimeError('Theme destinations must be writable only by root.')

    def status(self):
        data = grub.read_optional(self.state/'entries.json')
        value = json.loads(data) if data else {}
        return {'revision': self.revision(), 'can_undo': (self.state/'last.json').is_file(),
                'active': BEGIN in self.config.read_text(), 'options': value.get('options', {})}

    def unchanged_targets(self):
        return {key: (grub.read_optional(path), stat.S_IMODE(path.stat().st_mode) if path.exists() else 0o600)
                for key, path in self.files().items()}

    def review(self, targets, expected, operation, alive, assets=None):
        alive()
        if expected != self.revision(): raise RuntimeError('GRUB changed. Review fresh backups before applying.')
        record = self.snapshot(targets)
        if expected != self.revision(): raise RuntimeError('GRUB changed while making backups. Review again.')
        backup = Path(record['backup'])
        rows = []
        # Include every overwritten configuration file and the undo record.
        for key, path in self.files().items():
            before, after = record['before'][key], record['after'][key]
            if before == after: continue
            rows.append({'path': str(path), 'backup': str(backup/key) if before else '',
                         'change': 'Create' if before is None else 'Remove' if after is None else 'Replace'})
        rows.append({'path': str(self.state/'last.json'),
                     'backup': str(backup/'undo-history.json') if record['history'] else '',
                     'change': 'Replace' if record['history'] else 'Create'})
        created = [str(assets['path']/name) for name in sorted(assets['files'])] if assets else []
        script = self.recovery_script(record)
        grub.atomic(backup/'restore.sh', script.encode(), 0o700)
        command = 'sudo sh -- '+grub.shlex.quote(str(backup/'restore.sh'))
        live_command = 'sudo sh -- '+grub.shlex.quote('/mnt'+str(backup/'restore.sh'))+' /mnt'
        instructions = ('Recovery Instructions:\n'
            'If Linux starts, open a terminal and run:\n'+command+'\n\n'
            'If Linux cannot start, boot a Linux live USB. Mount your installed Linux root at /mnt, '
            'and mount any separate /boot filesystem at /mnt/boot (plus other boot submounts in their usual locations). '
            'Then run:\n'+live_command+'\n\n'
            'The recovery script verifies the backups, restores the files and permissions, and removes only files '
            'that this change would create. Then restart. Do not run grub-mkconfig after restoring; '
            'the saved boot menu is already complete. New theme assets can remain on disk.')
        report = ('Take a picture of the screen. Here are your backups.\n\n'
                  'Backup folder: '+str(backup)+'\n\nFiles that will change:\n')
        for row in rows:
            report += row['change']+': '+row['path']+'\nBackup: '+(row['backup'] or 'New file — no previous copy; recovery removes it.')+'\n\n'
        if created: report += 'New theme files (existing designs are kept):\n'+'\n'.join(created)+'\n\n'
        report += instructions+'\n'
        grub.atomic(backup/'RECOVERY.txt', report.encode(), 0o600)
        return {'targets': targets, 'expected': expected, 'record': record, 'assets': assets,
                'operation': operation, 'token': uuid.uuid4().hex,
                'review': {'files': rows, 'created': created, 'backup': str(backup),
                           'instructions': instructions, 'command': command, 'report': report,
                           'recovery_file': str(backup/'RECOVERY.txt')}}

    def recovery_script(self, record):
        # Self-contained POSIX recovery, usable from the installed system or a
        # live USB. Verify EVERY backup before the first restoration write.
        q = grub.shlex.quote
        script = ['#!/bin/sh', 'set -eu', 'prefix=${1:-}',
                  'case "$prefix" in ""|/*) ;; *) echo "Use an absolute mount path." >&2; exit 1;; esac',
                  'prefix=${prefix%/}',
                  'backup_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)']
        restore = []
        files = {**self.files(), 'history': self.state/'last.json'}
        # Restore the complete boot menu last, just like the normal transaction.
        for key in ('generator', 'entries', 'history', 'config'):
            before = ({'sha': record['history'], 'mode': 0o600} if record['history'] else None) if key == 'history' else record['before'][key]
            name = 'undo-history.json' if key == 'history' else key
            target = '"$prefix"'+q(str(files[key]))
            if before:
                source = '"$backup_dir/"'+q(name)
                script.append("printf '%s  %s\\n' "+q(before['sha'])+' '+source+' | sha256sum -c -')
                restore.append('install -m '+format(before['mode'], '04o')+' -- '+source+' '+target)
            else: restore.append('rm -f -- '+target)
        script += restore + ['rm -f -- "$prefix"'+q(str(self.state/'pending.json')), 'sync',
                             "printf '%s\\n' 'GRUB files restored. You can restart now.'"]
        return '\n'.join(script)+'\n'

    def plan_apply(self, options, expected, menu_revision, background_sha='', alive=lambda: None, prepare_fn=prepared_as_user):
        options = normalize(options); config = self.config.read_text()
        if (expected and expected != self.revision()) or menu_revision != grub.digest(config.encode()):
            raise RuntimeError('The boot menu changed. Reload it before applying your design.')
        expected = self.revision()
        old = grub.read_optional(self.generator)
        if old and SIGNATURE.encode() not in old: raise RuntimeError('Another tool owns the appearance generator. Nothing was changed.')
        if not grub.menu_entries(config) and not re.search(r'\bblscfg\b', config):
            raise RuntimeError('This is a forwarding or empty GRUB configuration. Open the installed boot menu before changing its appearance.')
        self.check(config)
        prepared = prepare_fn(options, background_sha)
        if set(prepared.get('files', {})) != FILES: raise ValueError('The prepared theme is incomplete.')
        files = {name: base64.b64decode(data, validate=True) for name, data in prepared['files'].items()}
        if sum(map(len, files.values())) > 24*1024*1024: raise ValueError('The theme is too large.')
        for name, data in files.items():
            if not data.startswith(b'\x89PNG\r\n\x1a\n' if name.endswith('.png') else b'FILE\0\0\0\x04PFF2'):
                raise ValueError('Invalid theme asset: '+name)
        alive(); self.safe_paths()
        target = self.assets/('design-'+uuid.uuid4().hex)
        files['theme.txt'] = theme_text(options).encode()
        saved = {**options, 'background': str(target/'source.png') if options['background'] else ''}
        files['appearance.json'] = grub.encoded(saved)
        # Resolve the already-existing boot directory. No /boot or /etc file is
        # created during review; assets remain in this authenticated process.
        probe = next(parent for parent in target.parents if parent.is_dir())
        fs_uuid = grub.command([grub.executable('grub-probe', 'grub2-probe'), '--target=fs_uuid', probe]).decode().strip()
        relative = grub.command([grub.executable('grub-mkrelpath', 'grub2-mkrelpath'), probe]).decode().strip()
        relative = relative.rstrip('/')+'/'+str(target.relative_to(probe))
        block = appearance_block(fs_uuid, relative)
        revised = replace_appearance(config, block); self.check(revised)
        targets = {'config': (revised.encode(), stat.S_IMODE(self.config.stat().st_mode)),
                   'generator': (('#!/bin/sh\nexec tail -n +3 "$0"\n'+block).encode(), 0o755),
                   'entries': (grub.encoded({'version': 1, 'options': saved}), 0o600)}
        return self.review(targets, expected, 'apply', alive, {'path': target, 'files': files, 'probe': probe, 'device': probe.stat().st_dev})

    def plan_system(self, expected, menu_revision, alive=lambda: None):
        config = self.config.read_text()
        if (expected and expected != self.revision()) or menu_revision != grub.digest(config.encode()):
            raise RuntimeError('The boot menu changed. Reload before restoring its appearance.')
        old = grub.read_optional(self.generator)
        if old and SIGNATURE.encode() not in old: raise RuntimeError('Another tool owns the appearance generator.')
        revised = replace_appearance(config, ''); self.check(revised)
        targets = self.unchanged_targets(); targets['config'] = (revised.encode(), targets['config'][1])
        targets['generator'] = (None, 0o755); targets['entries'] = (None, 0o600)
        return self.review(targets, self.revision(), 'system', alive)

    def plan_undo(self, expected, alive=lambda: None):
        if expected and expected != self.revision(): raise RuntimeError('GRUB changed. Review fresh backups before undoing.')
        record = json.loads((self.state/'last.json').read_text())
        backup = Path(record['backup'])
        if record['config'] != str(self.config) or not backup.resolve().is_relative_to((self.state/'backups').resolve()):
            raise RuntimeError('The recovery record belongs to another installation.')
        targets = {}
        for key, path in self.files().items():
            before, after = record['before'][key], record['after'][key]
            if grub.digest(grub.read_optional(path)) != (after['sha'] if after else None):
                raise RuntimeError('Another tool updated GRUB after this change. Reload before undoing.')
            data = (backup/key).read_bytes() if before else None
            if before and grub.digest(data) != before['sha']: raise RuntimeError('A backup failed its integrity check.')
            targets[key] = (data, before['mode'] if before else 0o600)
        self.check(targets['config'][0].decode())
        return self.review(targets, self.revision(), 'undo', alive)

    def commit_plan(self, plan, alive=lambda: None):
        alive(); self.safe_paths()
        if plan['expected'] != self.revision(): raise RuntimeError('GRUB changed after review. Review fresh backups before applying.')
        assets = plan['assets']; target = assets['path'] if assets else None
        created = False
        try:
            if target:
                if assets['probe'].stat().st_dev != assets['device']:
                    raise RuntimeError('The boot filesystem changed. Review fresh backups before applying.')
                self.assets.mkdir(parents=True, exist_ok=True, mode=0o755)
                target.mkdir(mode=0o755); created = True
                if target.stat().st_dev != assets['device']:
                    raise RuntimeError('The boot filesystem changed. Review fresh backups before applying.')
                for name, data in assets['files'].items(): grub.atomic(target/name, data, 0o644)
            return self.publish(plan['targets'], plan['expected'], alive, record=plan['record'])
        except BaseException:
            if created and target.name not in self.config.read_text(errors='replace') and target.name not in (grub.read_optional(self.generator) or b'').decode(errors='replace'):
                shutil.rmtree(target)
            raise


def catalog():
    result = {'presets': presets(), 'pictures': [], 'active': False, 'options': {}, 'can_undo': False}

    try:
        store = AppearanceStore()
        try: result.update(store.status())
        except PermissionError:
            text = store.config.read_text()
            result['active'] = BEGIN in text
            if result['active']:
                # Read only our immutable, public design metadata, never an
                # arbitrary path extracted from another GRUB theme.
                block = text.split(BEGIN, 1)[1].split(END, 1)[0]
                match = re.search(r'/cachyostools-editor/(design-[0-9a-f]{32})/theme.txt', block)
                if match:
                    result['options'] = normalize(json.loads((store.assets/match[1]/'appearance.json').read_text()))
    except (OSError, ValueError, RuntimeError): pass
    return result


def main():
    try:
        action = sys.argv[1] if len(sys.argv) == 2 else ''
        if action == 'catalog': result = catalog()
        elif action in ('preview', 'prepare'):
            request = json.loads(sys.stdin.read(65537))
            if action == 'preview': result = preview(request['options'], request.get('entries', [])[:32])
            else: result = prepare(request['options'], request.get('background_sha', ''))
        else: raise ValueError('Open the Appearance tab in GRUB Boot Editor.')
        print(json.dumps(result)); return 0
    except Exception as error:
        print(json.dumps({'error': str(error)})); return 1


if __name__ == '__main__': raise SystemExit(main())
