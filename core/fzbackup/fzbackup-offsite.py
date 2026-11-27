#!/usr/bin/python3
#
# fzbackup-offsite.py
#
# Randal A. Koene, 20261127
#
# Make an encrypted off-site copy of ~/.formalizer/archive (database backups
# and other archived data) with restic, and thin out old off-site snapshots.
#
# Called daily from fzdaily_do.source (cron) and by the "Ensure backup of the
# Formalizer database" button in DayWiz (through fzbackup-mirror-to-github.sh).
#
# Settings come from ~/.formalizer/config/fzbackup.py/config.json, with these
# defaults:
#
#   "restic_repository":    "rclone:gdrive:formalizer-restic"
#   "restic_password_file": "~/.config/restic/formalizer.pass"
#   "offsite_source":       "~/.formalizer/archive"
#
# Setup is described in core/fzbackup/README.md ("Setting up encrypted
# off-site backups"). Exit code 0 on success, 1 on failure.
#
# Plan B package 4 (Node 20261102153503.1).

import os
import sys
import json
import argparse
import subprocess
from datetime import datetime

userhome = os.getenv('HOME')
fzbackupconfig = userhome + '/.formalizer/config/fzbackup.py/config.json'

DEFAULTS = {
    'restic_repository': 'rclone:gdrive:formalizer-restic',
    'restic_password_file': '~/.config/restic/formalizer.pass',
    'offsite_source': '~/.formalizer/archive',
}

# Same rule as fzbackup-prune.py: the 4 most recent snapshots, the newest per day
# for 14 days, per week for 13 weeks, per month for 100 years (restic 0.12 has no
# "unlimited" value).
FORGET_POLICY = ['--keep-last', '4', '--keep-daily', '14', '--keep-weekly', '13',
                 '--keep-monthly', '1200']

TAG = 'formalizer-archive'

# Signs in restic or rclone output that the Google sign-in needs renewing.
AUTH_SIGNS = ['invalid_grant', 'token expired', 'oauth2', 'unauthorized_client',
              'Token has been expired or revoked']


def load_settings() -> dict:
    settings = dict(DEFAULTS)
    try:
        with open(fzbackupconfig) as f:
            settings.update({k: v for k, v in json.load(f).items() if k in DEFAULTS})
    except (OSError, ValueError):
        pass
    for key in ('restic_password_file', 'offsite_source'):
        settings[key] = os.path.expanduser(settings[key])
    return settings


def restic(settings: dict, args: list) -> subprocess.CompletedProcess:
    env = dict(os.environ)
    env['RESTIC_REPOSITORY'] = settings['restic_repository']
    env['RESTIC_PASSWORD_FILE'] = settings['restic_password_file']
    return subprocess.run(['restic'] + args, env=env, capture_output=True, text=True)


def explain_failure(settings: dict, what: str, res: subprocess.CompletedProcess):
    print(f'FAIL: {what} (exit code {res.returncode}).')
    output = (res.stderr or '') + (res.stdout or '')
    print(output.strip()[-2000:])
    if any(sign.lower() in output.lower() for sign in AUTH_SIGNS):
        remote = settings['restic_repository'].split(':')[1] if settings['restic_repository'].startswith('rclone:') else ''
        print('\nThe Google Drive sign-in seems to have expired. Renew it with:\n'
              f'  rclone config reconnect {remote}:\n'
              'and then run fzbackup-offsite.py again.')


def check_setup(settings: dict) -> bool:
    ok = True
    if not os.path.isfile(settings['restic_password_file']):
        print(f'FAIL: no restic password file at {settings["restic_password_file"]}.')
        ok = False
    if not os.path.isdir(settings['offsite_source']):
        print(f'FAIL: nothing to back up, {settings["offsite_source"]} does not exist.')
        ok = False
    if not ok:
        print('See core/fzbackup/README.md, "Setting up encrypted off-site backups".')
    return ok


def main():
    parser = argparse.ArgumentParser(
        description='Make an encrypted off-site copy of ~/.formalizer/archive with restic.')
    parser.add_argument('--no-forget', action='store_true',
                        help='do not thin out old off-site snapshots this time')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='only print problems (for cron)')
    args = parser.parse_args()

    settings = load_settings()
    if not args.quiet:
        print(f'Off-site backup of {settings["offsite_source"]} to {settings["restic_repository"]}')
    if not check_setup(settings):
        sys.exit(1)

    res = restic(settings, ['backup', '--tag', TAG, settings['offsite_source']])
    if res.returncode != 0:
        explain_failure(settings, 'restic backup', res)
        sys.exit(1)
    if not args.quiet:
        # The last lines of restic's report: files, data added, snapshot ID.
        print('\n'.join(l for l in res.stdout.splitlines()
                        if l.startswith(('Files:', 'Added to the repo', 'snapshot'))))

    if not args.no_forget:
        res = restic(settings, ['forget', '--tag', TAG, '--prune'] + FORGET_POLICY)
        if res.returncode != 0:
            explain_failure(settings, 'restic forget/prune', res)
            sys.exit(1)
        if not args.quiet:
            removed = [l for l in res.stdout.splitlines() if l.startswith('remove ')]
            print(f'Thinned out old snapshots: {removed[0] if removed else "nothing to remove"}')

    if not args.quiet:
        print(f'Off-site backup done at {datetime.now():%Y-%m-%d %H:%M}.')


if __name__ == '__main__':
    main()
