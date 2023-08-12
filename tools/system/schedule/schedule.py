#!/usr/bin/env python3
# schedule.py
# Randal A. Koene, 20230626

# TODO:
# - Find variable target date Nodes beyond day range until available minutes are filled.
# - Print map.

import os
import datetime
from json import loads
import numpy as np

thisdatetime = datetime.datetime.now()
thisdate=thisdatetime.strftime('%Y%m%d')
thishour=thisdatetime.hour
thisminute=thisdatetime.minute
thisminutes=60*thishour + thisminute
thisday_datetime = datetime.datetime.strptime(thisdate, '%Y%m%d')

tdprop_by_node = {}

def get_schedule_data(options:dict)->dict:

    os.system('fzgraphhtml -I -r -o /dev/shm/fzschedule.json -E STDOUT -D %d -F json -q -e' % options['num_days'])

    with open('/dev/shm/fzschedule.json','r') as f:
        schedjsontxt = f.read()

    schedjsontxt = '[\n'+schedjsontxt[:-2]+'\n]'

    scheduledata = loads(schedjsontxt)

    #print(str(scheduledata))
    return scheduledata

def get_node_description(node_id:str)->str:
    os.system('fzgraphhtml -n %s -o /dev/shm/fznode_desc.json -E STDOUT -F desc -q' % node_id)
    with open('/dev/shm/fznode_desc.json','r') as f:
        node_desc = f.read()
    return node_desc[0:60]

def convert_to_data_by_day(scheduledata:dict)->dict:

    day = ''
    days = {}
    day_entries = []
    for entry in scheduledata:
        td = entry['targetdate']
        entry_day = td[:8]
        if entry_day != day:
            if len(day_entries)>0:
                days[day] = day_entries
            day_entries = []
            day = entry_day
        if entry['tdprop'][0]=='<': # Removing <b></b>.
            entry['tdprop'] = entry['tdprop'][3:-4]
        day_entries.append(entry)
    if len(day_entries)>0:
        days[day]=day_entries

    print('Found %d days.' % len(days))
    for day_date, day_entries in days.items():
        print('Date: %s - %d entries.' % (str(day_date), len(day_entries)))
    return days

def initialize_days_map(days:dict)->tuple:
    total_minutes = len(days.keys())*24*60
    print('Number of minutes in daysmap: %d' % total_minutes)
    daysmap = np.zeros((total_minutes,))
    thisday_int = int(thisdate)
    firstday_int = int(list(days.keys())[0])
    passed_days = firstday_int - thisday_int
    passed_minutes = 0
    if passed_days > 0: passed_minutes = 60*24*passed_days
    passed_minutes += thisminutes
    if passed_minutes > 0:
        print('Passed minutes: %d' % passed_minutes)
        daysmap[0:passed_minutes] = -1
    return daysmap, total_minutes, passed_minutes

def is_tdprop(entry: dict, tdprop:str)->bool:
    return entry['tdprop'][0:len(tdprop)]==tdprop

def is_exact(entry: dict)->bool:
    return is_tdprop(entry, 'exact')

def is_fixed(entry: dict)->bool:
    return is_tdprop(entry, 'fixed')

def is_variable(entry: dict)->bool:
    return is_tdprop(entry, 'variable')

def map_exact_target_date_entries(days:dict, daysmap:np.ndarray)->tuple:
    day_dates = list(days.keys())
    exact_consumed = 0
    for day_n in range(len(days)):
        day = days[day_dates[day_n]]
        for entry in day:
            #print(entry['node_id'])
            if is_exact(entry):
                node_id = float(entry['node_id'])
                num_minutes = int(float(entry['req_hrs'])*60.0)
                tdtime_hr = int(entry['targetdate'][8:10])
                tdtime_min = int(entry['targetdate'][10:12])
                td_index = (day_n*60*24) + tdtime_hr*60 + tdtime_min
                start_index = td_index - num_minutes
                if start_index<0: start_index = 0
                #print('EXACT %s: %d - %d' % (node_id, start_index, td_index))
                exact_consumed += (td_index - start_index)
                tdprop_by_node[node_id] = 'exact'
                daysmap[start_index:td_index] = node_id
    print('Minutes consumed by exact target date Nodes: %d' % exact_consumed)
    return daysmap, exact_consumed

def map_fixed_target_date_entries(days:dict, daysmap:np.ndarray)->tuple:
    # For each fixed target date entry, find the latest available minute
    # and start filling back from there.
    day_dates = list(days.keys())
    fixed_consumed = 0
    for day_n in range(len(days)):
        day = days[day_dates[day_n]]
        for entry in day:
            #print(entry['tdprop'])
            if is_fixed(entry):
                node_id = float(entry['node_id'])
                num_minutes = int(float(entry['req_hrs'])*60.0)
                tdtime_hr = int(entry['targetdate'][8:10])
                tdtime_min = int(entry['targetdate'][10:12])
                latest_td_index = (day_n*60*24) + tdtime_hr*60 + tdtime_min
                # NOTE: The following is dumb and inefficient...
                tdprop_by_node[node_id] = 'fixed'
                i = latest_td_index
                while i>=0 and num_minutes>0:
                    if daysmap[i]==0:
                        daysmap[i]=node_id
                        num_minutes -= 1
                        fixed_consumed += 1
                    i -= 1
    print('Minutes consumed by fixed target date Nodes: %d' % fixed_consumed)
    return daysmap, fixed_consumed

def map_variable_target_date_entries(days:dict, daysmap:np.ndarray, start_at=0)->tuple:
    day_dates = list(days.keys())
    variable_consumed = 0
    for day_n in range(start_at, len(days)):
        day = days[day_dates[day_n]]
        for entry in day:
            if is_variable(entry):
                node_id = float(entry['node_id'])
                num_minutes = int(float(entry['req_hrs'])*60.0)
                tdtime_hr = int(entry['targetdate'][8:10])
                tdtime_min = int(entry['targetdate'][10:12])
                # NOTE: The following is dumb and inefficient...
                tdprop_by_node[node_id] = 'variable'
                i = 0
                while i<len(daysmap) and num_minutes>0:
                    if daysmap[i]==0:
                        daysmap[i]=node_id
                        num_minutes -= 1
                        variable_consumed += 1
                    i += 1
    print('Minutes consumed by variable target date Nodes: %d' % variable_consumed)
    return daysmap, variable_consumed

def get_and_map_more_variable_target_date_entries(days:dict, daysmap:np.ndarray, remaining_minutes:int)->tuple:
    num_mapped_days = len(days)
    num_days = 15
    while True:
        print('Getting more variable target date Nodes in %d days.' % num_days)
        options = {
            "num_days": num_days,
        }
        more_schedule_data = get_schedule_data(options)
        more_days = convert_to_data_by_day(more_schedule_data)
        more_day_dates = list(more_days.keys())
        more_minutes = 0
        for day_n in range(num_mapped_days, len(more_days)):
            day = more_days[more_day_dates[day_n]]
            for entry in day:
                if is_variable(entry):
                    num_minutes = int(float(entry['req_hrs'])*60.0)
                    more_minutes += num_minutes
                    if more_minutes >= remaining_minutes:
                        return map_variable_target_date_entries(more_days, daysmap, start_at=len(days))
        num_days += 15
        if num_days > 150:
            print('WARNING: Unable to fill all remaining minutes with variable target date Nodes within 150 days.')
            return map_variable_target_date_entries(more_days, daysmap, start_at=len(days))

def get_interval(start_minute:int, end_minute:int, days:dict)->tuple:
    start_delta = datetime.timedelta(minutes=start_minute)
    start_time = thisday_datetime + start_delta
    end_delta = datetime.timedelta(minutes=end_minute)
    end_time = thisday_datetime + end_delta
    return start_time, end_time, (end_minute-start_minute)

def print_entry(start_minute:int, end_minute:int, days:dict, node_id:str):
    interval_start, interval_end, interval_minutes = get_interval(start_minute, end_minute, days)
    node_desc = get_node_description(node_id).replace('\n',' ')
    print('%s - %s (%s mins): [%s, %s] %s' % (interval_start.strftime('%x %X'), interval_end.strftime('%x %X'), str(interval_minutes), node_id, tdprop_by_node[float(node_id)], node_desc))

def print_map(daysmap:np.ndarray, passed_minutes:int, days:dict):
    node_id = 0
    start_minute = 0
    for minute in range(passed_minutes, len(daysmap)):
        if daysmap[minute] != node_id:
            if node_id != 0:
                print_entry(start_minute, minute, days, node_id)
            node_id = daysmap[minute]
            start_minute = minute
    print_entry(start_minute, len(daysmap), days, node_id)

ENTRY_HTML_TEMPLATE = '''<tr><td>%s</td><td>%d mins</td><td>%s</td><td>%s</td><td>%s</td></tr>
'''

MAP_HTML_TEMPLATE = '''<html>
<head>
<title>Formalizer Calendar Schedule</title>
</head>
<body>
<table>
%s
</table>
</body>
</html>
'''

def print_entry_html(start_minute:int, end_minute:int, days:dict, node_id:float)->str:
    interval_start, interval_end, interval_minutes = get_interval(start_minute, end_minute, days)
    node_desc = get_node_description(node_id).replace('\n',' ')
    return ENTRY_HTML_TEMPLATE % (interval_start.strftime('%x %X'), interval_minutes, node_id, tdprop_by_node[float(node_id)], node_desc)

def print_map_html(daysmap:np.ndarray, passed_minutes:int, days:dict):
    node_id = 0
    start_minute = 0
    rows = ''
    for minute in range(passed_minutes, len(daysmap)):
        if daysmap[minute] != node_id:
            if node_id != 0:
                rows += print_entry_html(start_minute, minute, days, node_id)
            node_id = daysmap[minute]
            start_minute = minute
    rows += print_entry_html(start_minute, len(daysmap), days, node_id)
    map_html = MAP_HTML_TEMPLATE % rows
    with open('/var/www/html/fzcalsched.html','w') as f:
        f.write(map_html)

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

STRATEGY_1='''
Strategy 1:
1. Exact target date Nodes are scheduled into their appointed time-interval.
   Where they overlap, Nodes processed later (in temporal order) override
   Nodes processed earlier.
2. Fixed target date Nodes are scheduled into the latest minutes available
   before their target date.
3. Remaining available time is filled from earliest to latest minute by
   allocating minutes to variable target date Nodes. The variable target
   date Nodes are fully allocated. There is no Node-switching while a
   Node has not been completed. There is no shuffling of Nodes. And
   prioritization is done entirely based on target date and Node ID order,
   i.e. the order in which the variable target date Nodes appear in the
   Schedule.

Problems / to fix:
- Inherited target date Nodes are not properly dealt with.
- Variable target date Nodes are not found on later days as needed.
  (Probably keep finding more until all minutes are used up.)
'''

STRATEGY_DESCRIPTION={
    1: STRATEGY_1,
}

def print_strategy(strategy):
    print(STRATEGY_DESCRIPTION[strategy])

if __name__ == '__main__':
    strategy = 1
    options = parse_command_line()
    print_strategy(strategy)
    schedule_data = get_schedule_data(options)
    days = convert_to_data_by_day(schedule_data)
    daysmap, total_minutes, passed_minutes = initialize_days_map(days)
    daysmap, exact_consumed = map_exact_target_date_entries(days, daysmap)
    daysmap, fixed_consumed = map_fixed_target_date_entries(days, daysmap)
    daysmap, variable_consumed = map_variable_target_date_entries(days, daysmap)
    remaining_minutes = total_minutes - exact_consumed - fixed_consumed - variable_consumed - passed_minutes
    print('Remaining minutes to fill with variable target date entries: %d' % remaining_minutes)
    daysmap, more_variable_consumed = get_and_map_more_variable_target_date_entries(days, daysmap, remaining_minutes)
    print_map_html(daysmap, passed_minutes, days)
