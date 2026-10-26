#!/usr/bin/python3
#
# daywiz-set.py
#
# Randal A. Koene, 20261026
#
# Set the state of a DayWiz Wiztable item for a day, e.g. the number of
# open Nodes on the 'nodebalance' line.
#
# This calls daywiz.py through the web server (cmd=set), so that the update
# runs as the same user and along the same path as updates made on the
# DayWiz page. (Run directly as a regular user, daywiz.py cannot write its
# temporary save files in /var/www/webdata/formalizer.)
#
# By default, an item that is already filled in is not changed. Use -f to
# overwrite it.
#
# Exit codes: 0 when set or skipped, 1 on error.

import sys
import json
import argparse
from urllib.parse import urlencode
from urllib.request import urlopen

DEFAULT_URL = 'http://localhost/cgi-bin/daywiz.py'

def daywiz_set(item:str, value:str, date:str='', overwrite=False, url:str=DEFAULT_URL)->dict:
    fields = { 'cmd': 'set', 'item': item, 'value': value, }
    if date:
        fields['date'] = date
    if overwrite:
        fields['overwrite'] = 'true'
    try:
        with urlopen(url, data=urlencode(fields).encode(), timeout=60) as response:
            body = response.read().decode()
    except Exception as e:
        return { 'item': item, 'value': value, 'status': 'error', 'message': 'Request failed: '+str(e) }
    try:
        return json.loads(body)
    except:
        return { 'item': item, 'value': value, 'status': 'error', 'message': 'Unexpected response:\n'+body }

def report(result:dict, quiet=False):
    if result['status'] == 'error':
        print(f"daywiz-set: {result.get('item','')}: error: {result['message']}", file=sys.stderr)
    elif not quiet:
        if result['status'] == 'set':
            print(f"{result['date']} {result['item']}: set to {result['value']} (was '{result['old']}')")
        else:
            print(f"{result['date']} {result['item']}: skipped, already '{result['old']}'")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Set the state of a DayWiz Wiztable item for a day.')
    parser.add_argument('item', help='Wiztable item ID, e.g. nodebalance, emailunread, weight')
    parser.add_argument('value', help="New state. For checkbox items use 'true' or 'false'.")
    parser.add_argument('-d', '--date', default='', help='Day as YYYY.MM.DD or YYYY-MM-DD (default: today in DayWiz)')
    parser.add_argument('-f', '--force', action='store_true', help='Overwrite a state that is already filled in')
    parser.add_argument('-u', '--url', default=DEFAULT_URL, help=f'daywiz.py URL (default: {DEFAULT_URL})')
    parser.add_argument('-q', '--quiet', action='store_true', help='Only report errors')
    args = parser.parse_args()

    result = daywiz_set(args.item, args.value, args.date, args.force, args.url)
    report(result, args.quiet)
    sys.exit(1 if result['status'] == 'error' else 0)
