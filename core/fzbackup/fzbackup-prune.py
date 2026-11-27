#!/usr/bin/python3
#
# fzbackup-prune.py
#
# Randal A. Koene, 20261127
#
# Thin out Formalizer database backups (formalizer-postgres-backup-YYYYmmddHHMM.gz)
# by a retention rule, so that this no longer has to be done by hand:
#
#   - daily:   the newest backup of each day, for the last 14 days
#   - weekly:  the newest backup of each ISO week, for the last 92 days (3 months)
#   - monthly: the newest backup of each month, kept forever
#
#   - last:    the 4 most recent backups (--keep-last), whatever their dates
#
# The newest backup is always kept. A backup is deleted only if none of these
# rules keeps it, so every month always keeps one backup.
#
# Without --apply this only shows what would be kept and deleted.
#
# Plan B package 3 (Node 20261102153502.3), retention from Node 20210119080736.1.

import os
import re
import sys
import json
import argparse
from datetime import datetime, timedelta

userhome = os.getenv('HOME') or os.path.expanduser('~')  # CGI scripts may run without HOME
fzbackupconfig = userhome + '/.formalizer/config/fzbackup.py/config.json'

BACKUP_RE = re.compile(r'^formalizer-postgres-backup-(\d{12})\.gz$')


def backup_root() -> str:
    try:
        with open(fzbackupconfig) as f:
            return json.load(f)['backuproot']
    except (OSError, KeyError, ValueError):
        return userhome + '/.formalizer/archive/postgres/'


def find_backups(directory: str) -> list:
    """Return (timestamp, filename) for every backup in directory, oldest first."""
    backups = []
    for name in os.listdir(directory):
        m = BACKUP_RE.match(name)
        if m:
            backups.append((datetime.strptime(m.group(1), '%Y%m%d%H%M'), name))
    return sorted(backups)


def decide(backups: list, now: datetime, daily_days: int, weekly_days: int, keep_last: int = 4) -> dict:
    """Return {filename: reason it is kept} for the backups to keep."""
    keep = {}
    for _, name in backups[-keep_last:] if keep_last > 0 else []:
        keep[name] = 'last'

    def keep_newest_per(bucket_of, since, reason):
        newest = {}
        for t, name in backups:
            if since is None or t >= since:
                newest[bucket_of(t)] = name  # backups are sorted, so the last one wins
        for name in newest.values():
            keep.setdefault(name, reason)

    if backups:
        keep[backups[-1][1]] = 'newest'  # also when keep_last is 0
    keep_newest_per(lambda t: t.date(), now - timedelta(days=daily_days), 'daily')
    keep_newest_per(lambda t: t.isocalendar()[:2], now - timedelta(days=weekly_days), 'weekly')
    keep_newest_per(lambda t: (t.year, t.month), None, 'monthly')
    return keep


def main():
    parser = argparse.ArgumentParser(
        description='Thin out Formalizer database backups: newest per day for 14 days, '
                    'per week for 3 months, per month forever.')
    parser.add_argument('directory', nargs='?', default=backup_root(),
                        help=f'backup directory (default: {backup_root()})')
    parser.add_argument('--apply', action='store_true',
                        help='really delete (default: only show what would be deleted)')
    parser.add_argument('--daily-days', type=int, default=14)
    parser.add_argument('--weekly-days', type=int, default=92)
    parser.add_argument('--keep-last', type=int, default=4,
                        help='also keep this many of the most recent backups (default: 4), '
                             'in case something goes wrong shortly after a backup')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='only print deletions and errors (for cron)')
    args = parser.parse_args()

    backups = find_backups(args.directory)
    if not backups:
        print(f'No backups found in {args.directory}.')
        sys.exit(1)

    keep = decide(backups, datetime.now(), args.daily_days, args.weekly_days, args.keep_last)
    delete = [name for _, name in backups if name not in keep]

    if not args.quiet:
        for _, name in backups:
            print(f'  {"keep  " + keep[name] if name in keep else "DELETE":<14} {name}')
        print(f'{len(backups)} backups: keep {len(keep)}, delete {len(delete)}.')

    if not delete:
        return
    if not args.apply:
        if not args.quiet:
            print('Nothing was deleted. Use --apply to delete.')
        return
    for name in delete:
        os.remove(os.path.join(args.directory, name))
        print(f'Deleted {name}')


if __name__ == '__main__':
    main()
