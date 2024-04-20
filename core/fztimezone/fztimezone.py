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
    ('fzserverpq', formalizer_config_dir+'/fzserverpq/config.json', 'timezone_offset_hours', 'str', 4),
    ('fzgraphhtml', formalizer_config_dir+'/fzgraphhtml/config.json', 'timezone_offset_hours', 'str', 4),
    ('fzloghtml', formalizer_config_dir+'/fzloghtml/config.json', 'timezone_offset_hours', 'str', 4),
    ('fzlogtime', formalizer_config_dir+'/fzlogtime/config.json', 'hours_offset', 'int', 4),
    ('fzupdate', formalizer_config_dir+'/fzupdate/config.json', 'timezone_offset_hours', 'str', 4),
    ('fzuistate', webdata_formalizer_dir+'/fzuistate.json', 'tz_offset_hours', 'int', 0),
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
    for fzprog, sourcefile, element, eltype, tabsize in tzsettings_sources:
        try:
            with open(sourcefile, 'r') as f:
                fzcfg = json.load(f)
                tzdata = fzcfg[element]
                tzinfo_dict[fzprog] = tzdata
        except:
            pass
        # except Exception as e:
        #     print('Exception: '+str(e))
    return json.dumps(tzinfo_dict)

def show_timezone():
    tzinfo = get_time_zone_info()
    print(tzinfo)

def update_settings_source(sourcefile:str, element:str, tzhours:int, eltype:str, tabsize:int):
    try:
        with open(sourcefile, 'r') as f:
            fzcfg = json.load(f)
        if eltype=='str':
            fzcfg[element] = str(tzhours)
        else:
            fzcfg[element] = tzhours
        with open(sourcefile, 'w') as f:
            if tabsize==0:
                json.dump(fzcfg, f)
            else:
                json.dump(fzcfg, f, indent=tabsize)
    except:
        pass

def set_timezone(tzhours_str:str):
    try:
        tzhours = int(tzhours_str)
    except:
        return

    for fzprog, sourcefile, element, eltype, tabsize in tzsettings_sources:
        update_settings_source(sourcefile, element, tzhours, eltype, tabsize)


def parse_options():
    #print('Arguments received: '+str(sys.argv))
    parser = argparse.ArgumentParser(description='Information about and setting of Formalizer time zone offsets.')
    parser.add_argument('-s', '--show', dest='show_timezone', action="store_true", help='show time zone information')
    parser.add_argument('-z', '--set', dest='set_timezone', action="store", help='set time zone')
    parser.add_argument('-S', '--signal_file', dest='signalfile', action="store", help='specify signal file')
    parser.add_argument('-q', '--quiet', dest='quiet', action='store_true', help='run quietly')

    args = parser.parse_args()

    global flow_control

    if args.show_timezone:
        flow_control = 'show_timezone'

    if args.set_timezone:
        flow_control = 'set_timezone'

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
    if flow_control == 'set_timezone':
        set_timezone(args.set_timezone)
    elif flow_control == 'unknown':
        print('Some options have not been implemented yet.\n')

    if args.signalfile:
        update_signalfile(args.signalfile)

    exit(0)
