#!/usr/bin/python3
#
# fztimezone.py
#
# Randal A. Koene, 20240201
#
# Information about and setting of Formalizer time zone offsets. See README.md for more.
#
# NOTE:
# - This script is run as the Formalizer server user (not as www-data).
# - This script is called by:
#   - User on command line.
#   - fztimezone-cgi.py (must be called in 'quiet' mode to return clean JSON).

import os
import sys
import json
userhome = os.getenv('HOME')
sys.path.append(userhome+'/src/formalizer/core/lib')
sys.path.append(userhome+'/src/formalizer/core/include')

import argparse
import subprocess
#from shutil import chown

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
    ('fzserverpq', formalizer_config_dir+'/fzserverpq/config.json', 'timezone_offset_hours', 'str', 4, False),
    ('fzgraphhtml', formalizer_config_dir+'/fzgraphhtml/config.json', 'timezone_offset_hours', 'str', 4, False),
    ('fzloghtml', formalizer_config_dir+'/fzloghtml/config.json', 'timezone_offset_hours', 'str', 4, False),
    ('fzlogtime', formalizer_config_dir+'/fzlogtime/config.json', 'hours_offset', 'int', 4, False),
    ('fzupdate', formalizer_config_dir+'/fzupdate/config.json', 'timezone_offset_hours', 'str', 4, False),
    ('fzuistate', webdata_formalizer_dir+'/fzuistate.json', 'tz_offset_hours', 'int', 0, True),
]

DEBUGFILE='/tmp/fztimezone.debug'
def debug_position(position:str):
    with open(DEBUGFILE, 'w') as f:
        f.write(position)

def print_error(err:str):
    print('ERROR: %s' % err)

def update_signalfile(signalfile:str):
    try:
        tstamp = TimeStamp(ActualTime())
        with open(signalfile, 'w') as f:
            f.write(tstamp)
    except:
        pass

def get_time_zone_info()->str:
    tzinfo_dict = {}
    for fzprog, sourcefile, element, eltype, tabsize, set_permissions in tzsettings_sources:
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
    tzinfo_json = get_time_zone_info()
    print(tzinfo_json)

def update_settings_source(sourcefile:str, element:str, tzhours:int, eltype:str, tabsize:int)->tuple:
    try:
        with open(sourcefile, 'r') as f:
            fzcfg = json.load(f)
    except Exception as e:
        #debug_position('Failed at %s (step 1) with error: %s' % (sourcefile, str(e)))
        return ('ERROR', 'Failed at %s (step 1) with error: %s' % (sourcefile, str(e)))
    try:
        if eltype=='str':
            fzcfg[element] = str(tzhours)
        else:
            fzcfg[element] = tzhours
    except Exception as e:
        #debug_position('Failed at %s (step 2) with error: %s' % (sourcefile, str(e)))
        return ('ERROR', 'Failed at %s (step 2) with error: %s' % (sourcefile, str(e)))
    try:
        with open(sourcefile, 'w') as f:
            if tabsize==0:
                json.dump(fzcfg, f)
            else:
                json.dump(fzcfg, f, indent=tabsize)
    except Exception as e:
        #debug_position('Failed at %s (step 3) with error: %s' % (sourcefile, str(e)))
        return ('ERROR', 'Failed at %s (step 3) with error: %s' % (sourcefile, str(e)))
    return ('OK','')

def ensure_www_data_permissions(filepath:str)->bool:
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        os.remove(filepath)
        with open(filepath, 'w') as f:
            f.write(content)
        os.chmod(filepath, 0o666)
        #chown(filepath, 'www-data', 'www-data')
    except Exception as e:
        print_error('Unable to set www-data group ownership and permissions on %s. Error: %s' % (filepath, str(e)))
        return False
    return True

def set_timezone(tzhours_str:str):
    try:
        tzhours = int(tzhours_str)
    except:
        print_error('Failed to convert tzhours_str to integer.')
        return

    for fzprog, sourcefile, element, eltype, tabsize, set_permissions in tzsettings_sources:
        retstate, errstr = update_settings_source(sourcefile, element, tzhours, eltype, tabsize)
        if retstate == 'ERROR':
            print_error(errstr)
            return
        if set_permissions:
            ensure_www_data_permissions(sourcefile)

def parse_options():
    #print('Arguments received: '+str(sys.argv))
    parser = argparse.ArgumentParser(description='Information about and setting of Formalizer time zone offsets.')
    parser.add_argument('-s', '--show', dest='show_timezone', action="store_true", help='show time zone information')
    parser.add_argument('-z', '--set', dest='set_timezone', action="store", help='set time zone')
    parser.add_argument('-S', '--signal_file', dest='signalfile', action="store", help='specify signal file')
    parser.add_argument('-q', '--quiet', dest='quiet', action='store_true', help='run quietly, pure JSON or error output only')

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
        print_error('Some options have not been implemented yet.')

    #debug_position('pos-1')
    if args.signalfile:
        update_signalfile(args.signalfile)

    #debug_position('pos-2')
    exit(0)
