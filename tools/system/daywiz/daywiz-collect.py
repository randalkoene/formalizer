#!/usr/bin/python3
#
# daywiz-collect.py
#
# Randal A. Koene, 20261026
#
# Collect values for DayWiz Wiztable items automatically and fill them in
# with daywiz-set.py.
#
# Each collector returns the value for one Wiztable item. To add one, write
# a function and add it to COLLECTORS.
#
# By default, items that are already filled in are not changed.

import os
import sys
import argparse
import subprocess

def collect_nodebalance()->str:
    # fzserver-info -B prints: <open Nodes> <completed Nodes>
    res = subprocess.run('fzserver-info -q -B -E STDOUT', shell=True, capture_output=True, text=True)
    balance = res.stdout.split()
    if res.returncode != 0 or len(balance) < 1 or not balance[0].isdigit():
        raise RuntimeError(f'fzserver-info -B failed: {res.stdout}{res.stderr}')
    return balance[0]

COLLECTORS = {
    'nodebalance': collect_nodebalance,
}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Collect values and fill them in on the DayWiz page.')
    parser.add_argument('items', nargs='*', help=f"Items to collect (default: all): {', '.join(COLLECTORS)}")
    parser.add_argument('-d', '--date', default='', help='Day as YYYY.MM.DD or YYYY-MM-DD (default: today in DayWiz)')
    parser.add_argument('-f', '--force', action='store_true', help='Overwrite states that are already filled in')
    parser.add_argument('-n', '--dryrun', action='store_true', help='Only show the collected values')
    parser.add_argument('-q', '--quiet', action='store_true', help='Only report errors')
    args = parser.parse_args()

    items = args.items if args.items else list(COLLECTORS)
    for item in items:
        if item not in COLLECTORS:
            print(f'daywiz-collect: no collector for {item}', file=sys.stderr)
            sys.exit(1)

    daywiz_set = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'daywiz-set.py')
    failed = False
    for item in items:
        try:
            value = COLLECTORS[item]()
        except Exception as e:
            print(f'daywiz-collect: {item}: {e}', file=sys.stderr)
            failed = True
            continue
        if args.dryrun:
            print(f'{item}: {value}')
            continue
        cmd = [ sys.executable, daywiz_set, item, value ]
        if args.date:
            cmd += [ '-d', args.date ]
        if args.force:
            cmd.append('-f')
        if args.quiet:
            cmd.append('-q')
        if subprocess.run(cmd).returncode != 0:
            failed = True

    sys.exit(1 if failed else 0)
