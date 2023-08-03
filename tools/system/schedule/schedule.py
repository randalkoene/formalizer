#!/usr/bin/env python3
# schedule.py
# Randal A. Koene, 20230626

# TODO:
# - Allocate by hour.

import os
from json import loads
import numpy as np

def get_schedule_data(options:dict)->dict:

    os.system('fzgraphhtml -I -r -o /dev/shm/fzschedule.json -E STDOUT -D %d -F json -q -e' % options['num_days'])

    with open('/dev/shm/fzschedule.json','r') as f:
        schedjsontxt = f.read()

    schedjsontxt = '[\n'+schedjsontxt[:-2]+'\n]'

    scheduledata = loads(schedjsontxt)

    #print(str(scheduledata))
    return scheduledata

def convert_to_data_by_day(scheduledata:dict)->dict:

    day = ''
    days = {}
    day_entries = []
    for entry in scheduledata:
        td = entry['targetdate']
        entry_day = td[:8]
        if entry_day == day:
            day_entries.append(entry)
        else:
            if len(day_entries)>0:
                days[day] = day_entries
            day_entries = []
            day = entry_day
    if len(day_entries)>0:
        days[day]=day_entries

    print('Found %d days.' % len(days))
    for day_date, day_entries in days.items():
        print('Date: %s - %d entries.' % (str(day_date), len(day_entries)))
    return days

def initialize_days_map(days:dict)->np.ndarray:
    total_minutes = len(days.keys())*24*60
    print('Number of minutes in daysmap: %d' % total_minutes)
    return np.zeros((total_minutes,))

def map_exact_target_date_entries(days:dict, daysmap:np.ndarray)->np.ndarray:
    day_dates = list(days.keys())
    exact_consumed = 0
    for day_n in range(len(days)):
        day = days[day_dates[day_n]]
        for entry in day:
            if entry['tdprop'][3:8]=='exact':
                node_id = float(entry['node_id'])
                num_minutes = int(float(entry['req_hrs'])*60.0)
                tdtime_hr = int(entry['targetdate'][8:10])
                tdtime_min = int(entry['targetdate'][10:12])
                td_index = (day_n*60*24) + tdtime_hr*60 + tdtime_min
                start_index = td_index - num_minutes
                if start_index<0: start_index = 0
                exact_consumed += (td_index - start_index)
                daysmap[start_index:td_index] = node_id
    print('Minutes consumed by exact target date Nodes: %d' % exact_consumed)
    return daysmap

def map_fixed_target_date_entries(days:dict, daysmap:np.ndarray)->np.ndarray:
    # For each fixed target date entry, find the latest available minute
    # and start filling back from there.
    day_dates = list(days.keys())
    fixed_consumed = 0
    for day_n in range(len(days)):
        day = days[day_dates[day_n]]
        for entry in day:
            if entry['tdprop'][3:8]=='fixed':
                node_id = float(entry['node_id'])
                num_minutes = int(float(entry['req_hrs'])*60.0)
                tdtime_hr = int(entry['targetdate'][8:10])
                tdtime_min = int(entry['targetdate'][10:12])
                latest_td_index = (day_n*60*24) + tdtime_hr*60 + tdtime_min
                # NOTE: The following is dumb and inefficient...
                i = latest_td_index
                while i>=0 and num_minutes>0:
                    if daysmap[i]==0:
                        daysmap[i]=node_id
                        num_minutes -= 1
                        fixed_consumed += 1
                    i -= 1
    print('Minutes consumed by fixed target date Nodes: %d' % fixed_consumed)
    return daysmap

def map_variable_target_date_entries(days:dict, daysmap:np.ndarray)->np.ndarray:


# Step 7: Print the resulting schedule.

HELP='''
schedule.py -d <num_days>

Options:
  -d    Number of days to schedule (default: 1).

'''

def parse_command_line()->dict:
    from sys import argv

    num_days = 1

    cmdline = argv.copy()
    scriptpath = cmdline.pop(0)
    while len(cmdline) > 0:
        arg = cmdline.pop(0)
        if arg == '-h':
            print(HELP)
            exit(0)
        elif arg== '-d':
            num_days = int(cmdline.pop(0))

    return {
        "num_days": num_days,
    }

if __name__ == '__main__':
    options = parse_command_line()
    schedule_data = get_schedule_data(options)
    days = convert_to_data_by_day(schedule_data)
    daysmap = initialize_days_map(days)
    daysmap = map_exact_target_date_entries(days, daysmap)
    daysmap = map_fixed_target_date_entries(days, daysmap)
