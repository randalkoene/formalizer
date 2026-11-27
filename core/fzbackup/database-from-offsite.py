#!/usr/bin/python3
#
# database-from-offsite.py
#
# Randal A. Koene, 20261127
#
# Fetch a Formalizer database backup from the encrypted off-site restic
# repository (see fzbackup-offsite.py) into the local backup directory:
# by default the newest one, or the one named with -f (from whichever
# off-site snapshot still holds it).
#
# Replaces database-from-github.py, now that ~/.formalizer/archive is no longer
# kept in Git. Prints "Saved as <filename>" on success (used by replicate.py).
#
# Exit code 0 on success, 1 on failure.

import os
import re
import sys
import shutil
import argparse
import tempfile
import importlib.util
from datetime import datetime

here = os.path.dirname(os.path.realpath(__file__))
spec = importlib.util.spec_from_file_location('fzbackup_offsite', os.path.join(here, 'fzbackup-offsite.py'))
offsite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(offsite)

spec = importlib.util.spec_from_file_location('fzbackup_prune', os.path.join(here, 'fzbackup-prune.py'))
prune = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prune)

BACKUP_NAME_RE = re.compile(r'^formalizer-postgres-backup-\d{12}\.gz$')


def finish(code: int, signalfile: str):
    sys.stdout.flush()
    if signalfile:
        with open(signalfile, 'w') as f:
            f.write(datetime.now().strftime('%Y%m%d%H%M%S'))
    sys.exit(code)


def locate(settings: dict, name: str) -> tuple:
    """Return (snapshot id, path in snapshot) holding the named backup, or the newest
    backup if name is empty. ('', '') if not found."""
    if not name:
        res = offsite.restic(settings, ['ls', 'latest'])
        if res.returncode != 0:
            offsite.explain_failure(settings, 'listing the latest off-site snapshot', res)
            return ('', '')
        paths = sorted(l for l in res.stdout.splitlines()
                       if BACKUP_NAME_RE.match(os.path.basename(l)) and '/archive/postgres/' in l)
        return ('latest', paths[-1]) if paths else ('', '')
    found = offsite.offsite_db_backups(settings).get(name)
    if not found:
        return ('', '')
    return (found['snapshots'][-1], found['path'])


def main():
    parser = argparse.ArgumentParser(
        description='Fetch a Formalizer database backup from the off-site restic repository.')
    parser.add_argument('-f', dest='name', help='backup to fetch (default: the newest)')
    parser.add_argument('-S', dest='signalfile',
                        help='when done, write a time stamp to this file (used by the Backups page)')
    args = parser.parse_args()

    if args.name and not BACKUP_NAME_RE.match(args.name):
        print(f'FAIL: not a backup name: {args.name}')
        finish(1, args.signalfile)

    settings = offsite.load_settings()
    if not os.path.isfile(settings['restic_password_file']):
        print(f'FAIL: no restic password file at {settings["restic_password_file"]}.')
        print('See core/fzbackup/README.md, "Getting backups back on a new machine".')
        finish(1, args.signalfile)

    snapshot, path = locate(settings, args.name)
    if not path:
        print(f'FAIL: {args.name or "no database backup"} not found in the off-site snapshots.')
        finish(1, args.signalfile)
    name = os.path.basename(path)

    destdir = prune.backup_root()
    os.makedirs(destdir, exist_ok=True)
    dest = os.path.join(destdir, name)
    if os.path.isfile(dest):
        print(f'{name} is already in {destdir}.')
        print(f'Saved as {name}')
        finish(0, args.signalfile)

    print(f'Fetching {name} from snapshot {snapshot} of {settings["restic_repository"]} ...')
    with tempfile.TemporaryDirectory() as tmp:
        res = offsite.restic(settings, ['restore', snapshot, '--target', tmp, '--include', path])
        restored = os.path.join(tmp, path.lstrip('/'))
        if res.returncode != 0 or not os.path.isfile(restored):
            offsite.explain_failure(settings, 'restoring from the off-site snapshot', res)
            finish(1, args.signalfile)
        shutil.move(restored, dest)
    print(f'Saved as {name}')
    finish(0, args.signalfile)


if __name__ == '__main__':
    main()
