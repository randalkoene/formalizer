#!/usr/bin/python3
#
# fzbackup-verify.py
#
# Randal A. Koene, 20261127
#
# Check that a Formalizer database backup (a gzipped pg_dump, as made by
# fzbackup) can really be restored, without touching the live database:
#
#   1. The .gz file is intact (gzip -t), and its SHA-256 is shown.
#   2. It is restored into a separate scratch database.
#   3. Each table in the scratch database is compared with the live
#      database by row count.
#   4. The scratch database is dropped (unless --keep).
#
# Row counts can differ when the Formalizer changed after the backup was
# made (new Log entries, Node edits). Those differences are reported but
# only a failed restore counts as a failure. Use --fresh to make a new
# backup first, so that the comparison is against a current backup.
#
# Exit codes: 0 = restored and all tables match, 1 = the backup failed a
# check, 2 = restored, but some row counts differ from the live database.
#
# Plan B package 3 (Node 20261102153502.3).

import os
import re
import sys
import glob
import json
import hashlib
import argparse
import subprocess
from datetime import datetime

userhome = os.getenv('HOME') or os.path.expanduser('~')  # CGI scripts may run without HOME
fzbackupconfig = userhome + '/.formalizer/config/fzbackup.py/config.json'

SCRATCH_PREFIX = 'fzverify_'

BACKUP_NAME_RE = re.compile(r'^formalizer-postgres-backup-\d{12}\.gz$')

# Table name and exact row count for every table outside the system schemas.
COUNT_QUERY = r"""
SELECT table_schema || '.' || table_name,
       (xpath('/row/c/text()',
              query_to_xml(format('SELECT count(*) AS c FROM %I.%I', table_schema, table_name),
                           false, true, '')))[1]::text
FROM information_schema.tables
WHERE table_type = 'BASE TABLE'
  AND table_schema NOT IN ('pg_catalog', 'information_schema')
ORDER BY 1;
"""


def backup_root() -> str:
    try:
        with open(fzbackupconfig) as f:
            return json.load(f)['backuproot']
    except (OSError, KeyError, ValueError):
        return userhome + '/.formalizer/archive/postgres/'


def newest_backup(directory: str) -> str:
    files = sorted(glob.glob(os.path.join(directory, 'formalizer-postgres-backup-*.gz')))
    return files[-1] if files else ''


def run(cmd: list, **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, **kwargs)


def sha256_of(path: str) -> str:
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def table_counts(dbname: str) -> dict:
    res = run(['psql', '-d', dbname, '-At', '-F', '\t', '-c', COUNT_QUERY])
    if res.returncode != 0:
        raise RuntimeError(f'Counting rows in {dbname} failed:\n{res.stderr}')
    counts = {}
    for line in res.stdout.splitlines():
        table, _, count = line.partition('\t')
        counts[table] = int(count)
    return counts


def drop_scratch(scratch: str):
    run(['dropdb', '--if-exists', scratch])


def restore_into_scratch(backup: str, scratch: str) -> bool:
    drop_scratch(scratch)
    res = run(['createdb', '-T', 'template0', scratch])
    if res.returncode != 0:
        print(f'FAIL: could not create the scratch database {scratch}:\n{res.stderr}')
        return False
    gunzip = subprocess.Popen(['gzip', '-d', '-c', backup], stdout=subprocess.PIPE)
    res = subprocess.run(['psql', '-q', '-d', scratch, '--set', 'ON_ERROR_STOP=on',
                          '--single-transaction', '-o', '/dev/null'],
                         stdin=gunzip.stdout, capture_output=True, text=True)
    gunzip.stdout.close()
    gunzip.wait()
    if gunzip.returncode != 0 or res.returncode != 0:
        print(f'FAIL: restoring {backup} into {scratch} failed:\n{res.stderr}')
        return False
    return True


def compare(live: dict, restored: dict) -> int:
    """Print a table comparison and return the number of tables that differ."""
    differ = 0
    width = max((len(t) for t in set(live) | set(restored)), default=10)
    print(f'  {"table":<{width}}  {"backup":>10}  {"live":>10}')
    for table in sorted(set(live) | set(restored)):
        b = restored.get(table)
        l = live.get(table)
        if b is None:
            note = '  only in live database'
        elif l is None:
            note = '  only in backup'
        elif b != l:
            note = f'  differs by {l - b:+d}'
        else:
            note = ''
        if note:
            differ += 1
        bs = '-' if b is None else str(b)
        ls = '-' if l is None else str(l)
        print(f'  {table:<{width}}  {bs:>10}  {ls:>10}{note}')
    return differ


def make_fresh_backup() -> str:
    print('Making a fresh backup with fzbackup...')
    res = run(['fzbackup'])
    print(res.stdout.strip())
    if res.returncode != 0:
        print(res.stderr)
        return ''
    return newest_backup(backup_root())


def verify(backup: str, dbname: str, scratch: str, keep: bool) -> int:
    print(f'Backup:  {backup}')
    if not os.path.isfile(backup):
        print('FAIL: file not found.')
        return 1
    print(f'Size:    {os.path.getsize(backup):,} bytes')
    print(f'Made:    {datetime.fromtimestamp(os.path.getmtime(backup)):%Y-%m-%d %H:%M}')
    print(f'SHA-256: {sha256_of(backup)}')

    if run(['gzip', '-t', backup]).returncode != 0:
        print('FAIL: gzip integrity test failed.')
        return 1
    print('OK:   gzip integrity test.')

    print(f'Restoring into scratch database {scratch} (the live database is not touched)...')
    if not restore_into_scratch(backup, scratch):
        drop_scratch(scratch)
        return 1
    print('OK:   restore completed without errors.')

    try:
        restored = table_counts(scratch)
        live = table_counts(dbname)
    except RuntimeError as e:
        print(f'FAIL: {e}')
        if not keep:
            drop_scratch(scratch)
        return 1

    if not keep:
        drop_scratch(scratch)
    else:
        print(f'The scratch database {scratch} was kept. Remove it with: dropdb {scratch}')

    if not restored:
        print('FAIL: the restored database contains no tables.')
        return 1
    print(f'Row counts, backup versus live database "{dbname}":')
    differ = compare(live, restored)
    total = sum(restored.values())
    print(f'{len(restored)} tables, {total:,} rows restored.')
    if differ:
        print(f'NOTE: {differ} table(s) differ from the live database. This is expected '
              'if the Formalizer changed after the backup was made.')
        return 2
    print('PASS: every table matches the live database.')
    return 0


def main():
    parser = argparse.ArgumentParser(
        description='Verify a Formalizer database backup by restoring it into a scratch '
                    'database and comparing it with the live database.')
    parser.add_argument('backup', nargs='?',
                        help='backup .gz file, or a directory to take the newest one from '
                             f'(default: newest in {backup_root()})')
    parser.add_argument('--fresh', action='store_true',
                        help='make a new backup with fzbackup first and verify that one')
    parser.add_argument('-d', '--database', default='formalizer',
                        help='live database to compare with (default: formalizer)')
    parser.add_argument('--keep', action='store_true',
                        help='keep the scratch database for inspection')
    parser.add_argument('-f', dest='name',
                        help='name of a backup in the backup directory (used by the Backups page)')
    parser.add_argument('-S', dest='signalfile',
                        help='when done, write a time stamp to this file (used by the Backups page)')
    args = parser.parse_args()

    if args.name:
        if not BACKUP_NAME_RE.match(args.name):
            print(f'FAIL: not a backup name: {args.name}')
            finish(1, args.signalfile)
        backup = os.path.join(backup_root(), args.name)
    elif args.fresh:
        backup = make_fresh_backup()
    else:
        backup = args.backup or backup_root()
        if os.path.isdir(backup):
            backup = newest_backup(backup)
    if not backup:
        print('FAIL: no backup file found.')
        finish(1, args.signalfile)

    scratch = SCRATCH_PREFIX + datetime.now().strftime('%Y%m%d%H%M%S')
    if scratch == args.database:
        print('FAIL: the scratch database name equals the live database name.')
        finish(1, args.signalfile)
    finish(verify(backup, args.database, scratch, args.keep), args.signalfile)


def finish(code: int, signalfile: str):
    sys.stdout.flush()
    if signalfile:
        with open(signalfile, 'w') as f:
            f.write(datetime.now().strftime('%Y%m%d%H%M%S'))
    sys.exit(code)


if __name__ == '__main__':
    main()
