#!/usr/bin/python3
#
# fztimezone.py
#
# Randal A. Koene, 20240201
#
# Information about and setting of Formalizer time zone offsets. See README.md for more.

import os
import sys
import json
userhome = os.getenv('HOME')
sys.path.append(userhome+'/src/formalizer/core/lib')
sys.path.append(userhome+'/src/formalizer/core/include')

import argparse
import subprocess

from fzmodbase import *
import coreversion
from TimeStamp import TimeStamp, ActualTime

version = "0.1.0-0.1"

config = {
    'verbose' : False,
    'sourceroot': '~/src/formalizer',
    'quiet': False,
}

global flow_control
flow_control = 'unknown'

formalizer_config_dir = userhome+'/.formalizer/config'
webdata_formalizer_dir = '/var/www/webdata/formalizer'
tzsettings_sources = [
    ('fzserverpq', formalizer_config_dir+'/fzserverpq/config.json', 'timezone_offset_hours'),
    ('fzgraphhtml', formalizer_config_dir+'/fzgraphhtml/config.json', 'timezone_offset_hours'),
    ('fzloghtml', formalizer_config_dir+'/fzloghtml/config.json', 'timezone_offset_hours'),
    ('fzlogtime', formalizer_config_dir+'/fzlogtime/config.json', 'hours_offset'),
    ('fzupdate', formalizer_config_dir+'/fzupdate/config.json', 'timezone_offset_hours'),
    ('fzuistate', webdata_formalizer_dir+'/fzuistate.json', 'tz_offset_hours'),
]

def update_signalfile(signalfile:str):
    try:
        tstamp = TimeStamp(ActualTime())
        with open(signalfile, 'w') as f:
            f.write(tstamp)
    except:
        pass

def get_time_zone_info()->str:
    tzinfo_dict = {}
    for fzprog, sourcefile, element in tzsettings_sources:
        try:
            with open(sourcefile, 'r') as f:
                fzcfg = json.load(f)
                tzdata = fzcfg[element]
                tzinfo_dict[fzprog] = tzdata
        except:
            pass
    return json.dumps(tzinfo_dict)

def show_timezone():
    tzinfo = get_time_zone_info()
    print(tzinfo)

def parse_options():
    #print('Arguments received: '+str(sys.argv))
    parser = argparse.ArgumentParser(description='Information about and setting of Formalizer time zone offsets.')
    parser.add_argument('-s', '--show', dest='show_timezone', action="store_true", help='show time zone information')
    parser.add_argument('-S', '--signal_file', dest='signalfile', action="store", help='specify signal file')
    parser.add_argument('-q', '--quiet', dest='quiet', action='store_true', help='run quietly')

    args = parser.parse_args()

    global flow_control

    if args.show_timezone:
        flow_control = 'show_timezone'

    if args.quiet:
        config['quiet'] = True

    return args


if __name__ == '__main__':

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:Info:TimeZone v{version} (core v{core_version})"

    args=parse_options()

    if not config['quiet']:
        print(server_long_id+"\n")

    if flow_control == 'show_timezone':
        show_timezone()
    elif flow_control == 'unknown':
        print('Some options have not been implemented yet.\n')

    if args.signalfile:
        update_signalfile(args.signalfile)

    exit(0)
