#!/usr/bin/python3
#
# database-from-offsite.py
#
# Randal A. Koene, 20261127
#
# Fetch the newest Formalizer database backup from the encrypted off-site
# restic repository (see fzbackup-offsite.py) into the local backup directory.
# Replaces database-from-github.py, now that ~/.formalizer/archive is no longer
# kept in Git. Prints "Saved as <filename>" on success (used by replicate.py).
#
# Exit code 0 on success, 1 on failure.

import os
import re
import sys
import shutil
import tempfile
import importlib.util

here = os.path.dirname(os.path.realpath(__file__))
spec = importlib.util.spec_from_file_location('fzbackup_offsite', os.path.join(here, 'fzbackup-offsite.py'))
offsite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(offsite)

spec = importlib.util.spec_from_file_location('fzbackup_prune', os.path.join(here, 'fzbackup-prune.py'))
prune = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prune)

BACKUP_PATH_RE = re.compile(r'/archive/postgres/formalizer-postgres-backup-\d{12}\.gz$')


def main():
    settings = offsite.load_settings()
    if not os.path.isfile(settings['restic_password_file']):
        print(f'FAIL: no restic password file at {settings["restic_password_file"]}.')
        print('See core/fzbackup/README.md, "Getting backups back on a new machine".')
        sys.exit(1)

    res = offsite.restic(settings, ['ls', 'latest'])
    if res.returncode != 0:
        offsite.explain_failure(settings, 'listing the latest off-site snapshot', res)
        sys.exit(1)
    backups = sorted(l for l in res.stdout.splitlines() if BACKUP_PATH_RE.search(l))
    if not backups:
        print('FAIL: the latest off-site snapshot contains no database backups.')
        sys.exit(1)
    newest = backups[-1]
    name = os.path.basename(newest)

    destdir = prune.backup_root()
    os.makedirs(destdir, exist_ok=True)
    dest = os.path.join(destdir, name)
    if os.path.isfile(dest):
        print(f'{name} is already in {destdir}.')
        print(f'Saved as {name}')
        return

    print(f'Fetching {name} from {settings["restic_repository"]} ...')
    with tempfile.TemporaryDirectory() as tmp:
        res = offsite.restic(settings, ['restore', 'latest', '--target', tmp, '--include', newest])
        restored = os.path.join(tmp, newest.lstrip('/'))
        if res.returncode != 0 or not os.path.isfile(restored):
            offsite.explain_failure(settings, 'restoring from the off-site snapshot', res)
            sys.exit(1)
        shutil.move(restored, dest)
    print(f'Saved as {name}')


if __name__ == '__main__':
    main()
