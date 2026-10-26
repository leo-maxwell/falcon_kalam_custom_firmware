#!/usr/bin/env python3
"""POLICY-DOCS checker. Offline, stdlib only. Exit status 1 on any failure.

1. docs/ contains exactly the category documents listed in docs/policy.md §4.
2. Every `docs/<name>.md` mentioned anywhere in the repo's own files exists.
3. Every repo path written in backticks in README.md / docs/*.md exists
   (paths starting with a top-level project directory; glob/ellipsis forms skipped).
4. docs/firmware.md states the current ship and dev SHA-256 and size from artifacts/.
5. Device status (hashes of flashed images) appears only in docs/devnotes.md,
   and the current release hash only in docs/firmware.md (plus devnotes).
6. Retired names (falcon_hs_r2, "Falcon HS R2", ...) appear nowhere except the
   naming history in docs/policy.md (POLICY-NAMING).
7. The release number (VIA_FIRMWARE_VERSION, I12) equals the one in
   docs/firmware.md, and was bumped if the image hashes changed since HEAD.
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / 'docs'
TOP = ('originals/', 'artifacts/', 'tools/', 'scripts/', 'keyboards/', 'userspace/', 'via/', 'bootloader/', 'docs/')
OWN = ('README.md', 'docs', 'scripts', 'tools', 'keyboards', 'userspace', 'via')
failures = []


def fail(msg):
    failures.append(msg)
    print('FAIL ' + msg)


def ok(msg):
    print('ok   ' + msg)


# 1. category documents
policy = (DOCS / 'policy.md').read_text()
section = policy[policy.index('## 4. Documentation'):policy.index('## 5.')]
listed = set(re.findall(r'`docs/([a-z-]+\.md)`', section))
present = {p.name for p in DOCS.glob('*.md')}
if listed == present:
    ok('docs/ = the %d categories of policy.md §4' % len(listed))
else:
    fail('docs/ mismatch: missing %s, unlisted %s' % (sorted(listed - present), sorted(present - listed)))

# 2. references to docs
files = [ROOT / 'README.md'] + [p for d in OWN[1:] for p in (ROOT / d).rglob('*') if p.is_file()
                                and p.suffix in ('.md', '.py', '.sh', '.c', '.h', '.mk', '.json')]
bad = sorted({(str(f.relative_to(ROOT)), m) for f in files for m in re.findall(r'docs/([a-z-]+\.md)', f.read_text(errors='ignore'))
              if not (DOCS / m).exists()})
for f, m in bad:
    fail('%s references missing docs/%s' % (f, m))
if not bad:
    ok('all docs/*.md references resolve')

# 3. repo paths in docs
missing = []
for f in [ROOT / 'README.md'] + sorted(DOCS.glob('*.md')):
    for path in re.findall(r'`([^`\s]+)`', f.read_text()):
        if not path.startswith(TOP) or any(ch in path for ch in '*{}<>|…') or path.endswith('…'):
            continue
        if not (ROOT / path.rstrip('/')).exists():
            missing.append('%s: %s' % (f.name, path))
for m in missing:
    fail('path does not exist: ' + m)
if not missing:
    ok('all backticked repo paths in the docs exist')

# 4. release identity: both L2 variants (POLICY-VARIANTS)
fw = (DOCS / 'firmware.md').read_text()
hashes = []
for variant in ('ship', 'dev'):
    sha_file = ROOT / 'artifacts' / ('falcon_kalam_%s.bin.sha256' % variant)
    if not sha_file.exists():
        fail('%s missing (run scripts/build.sh)' % sha_file.relative_to(ROOT))
        continue
    sha = sha_file.read_text().split()[0]
    size = (ROOT / 'artifacts' / ('falcon_kalam_%s.bin' % variant)).stat().st_size - 16
    hashes.append(sha)
    if sha in fw:
        ok('firmware.md %s hash = artifacts (%s…)' % (variant, sha[:8]))
    else:
        fail('firmware.md does not contain the current %s hash %s' % (variant, sha))
    if '{:,}'.format(size) in fw:
        ok('firmware.md %s size = %s B' % (variant, '{:,}'.format(size)))
    else:
        fail('firmware.md does not state the current %s payload size {:,} B'.format(size) % variant)

# 5. status lives in devnotes, release identity in firmware.md
for doc in DOCS.glob('*.md'):
    if doc.name in ('firmware.md', 'devnotes.md'):
        continue
    text = doc.read_text()
    for sha in hashes:
        if sha in text or sha[:8] in text:
            fail('%s repeats a current image hash (belongs in firmware.md/devnotes.md)' % doc.name)

# 6. POLICY-NAMING: retired names may appear only in the naming history (docs/policy.md).
RETIRED = ('falcon_hs_r2', 'Falcon HS R2', 'hhkb_via', 'keymaps/falcon/', 'keymaps/falcon_dev')
hits = sorted({'%s: %s' % (f.relative_to(ROOT), name) for f in files + sorted(DOCS.glob('*.md'))
               for name in RETIRED if name in f.read_text(errors='ignore') and f not in (DOCS / 'policy.md', Path(__file__).resolve())})
for h in hits:
    fail('retired name outside the policy naming history: ' + h)
if not hits:
    ok('no retired names (POLICY-NAMING)')

# 7. release number (I12)
REL_RE = re.compile(r'#define VIA_FIRMWARE_VERSION (\d+)')
CFG = 'userspace/users/falcon_kalam/config.h'
release = REL_RE.search((ROOT / CFG).read_text())
if not release:
    fail('%s defines no VIA_FIRMWARE_VERSION' % CFG)
else:
    release = int(release.group(1))
    if re.search(r'\*\*Release %d\*\*' % release, fw):
        ok('firmware.md release = %d' % release)
    else:
        fail('firmware.md does not state **Release %d** (VIA_FIRMWARE_VERSION)' % release)

    def at_head(path):
        r = subprocess.run(['git', '-C', str(ROOT), 'show', 'HEAD:' + path], capture_output=True, text=True)
        return r.stdout if r.returncode == 0 else ''

    head_rel = REL_RE.search(at_head(CFG))
    head_fw = at_head('docs/firmware.md')
    if head_rel and hashes and not all(h in head_fw for h in hashes) and int(head_rel.group(1)) == release:
        fail('the images changed since HEAD but VIA_FIRMWARE_VERSION is still %d: bump it' % release)
    elif head_rel:
        ok('release number consistent with HEAD (%s -> %d)' % (head_rel.group(1), release))

print('RESULT: %s' % ('PASS' if not failures else 'FAIL (%d)' % len(failures)))
sys.exit(1 if failures else 0)
