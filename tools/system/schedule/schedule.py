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
from subprocess import Popen, PIPE

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"
webschedulefile = webdata_path+'/fzschedule.html'
defaultschedulefile = '/dev/shm/fzschedule.html'
csvschedulefile = '/fzschedule.csv'

thisdatetime = datetime.datetime.now()
thisdate=thisdatetime.strftime('%Y%m%d')
thishour=thisdatetime.hour
thisminute=thisdatetime.minute
thisminutes=60*thishour + thisminute
thisday_datetime = datetime.datetime.strptime(thisdate, '%Y%m%d')

# We need this so that permissions of a /dev/shm/fzschedule*.json file
# that happens to exist do not obstruct this run.
#timestamp=thisdatetime.strftime('%Y%m%d%H%M%S')
#shm_fzschedule='/dev/shm/fzschedule-%s.json' % timestamp
#shm_fznodedesc='/dev/shm/fznode_desc-%s.json' % timestamp

tdprop_by_node = {}

def try_command_call(thecmd, printhere = True)->tuple:
    try:
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        error = child_stderr.read()
        child_stderr.close()
        if printhere:
            print(result)
            print(error)
            return ('','')
        else:
            return (result, error)

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ('','')

def get_schedule_data(options:dict)->dict:

    print('Fetching Schedule data for %d days.' % options['num_days'])

    if options['web']: thecmd = './'
    else: thecmd = ''
    thecmd += 'fzgraphhtml -I -r -o STDOUT -E STDOUT -N all -D %d -F json -q -e' % options['num_days']
    schedjsontxt, schederr = try_command_call(thecmd, printhere=False)

    if schederr != '':
        print(schederr)

    schedjsontxt = '[\n'+schedjsontxt[:-2]+'\n]'

    scheduledata = loads(schedjsontxt)

    #print(str(scheduledata))
    return scheduledata

def get_node_description(node_id:str, options:dict)->str:
    if options['web']: thecmd = './'
    else: thecmd = ''
    thecmd += 'fzgraphhtml -n %s -o STDOUT -E STDOUT -F desc -x 100 -q' % node_id
    node_desc, node_err = try_command_call(thecmd, printhere=False)
    if node_err != '':
        print(node_err)
    return node_desc

def remove_file(path:str):
    try:
        os.remove(path)
    except Exception as e:
        print('File clean-up exception: '+str(e))

#def clean_up_temporary_files():
#    remove_file(shm_fzschedule)
#    remove_file(shm_fznodedesc)

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

def min_block_available_backwards(daysmap:np.ndarray, i:int, min_block_size:int)->bool:
    while i>=0:
        if daysmap[i] != 0: return False
        min_block_size -= 1
        if min_block_size < 1: return True
        i -= 1
    return False

def set_block_to_node_backwards(daysmap:np.ndarray, i:int, block_size:int, node_id:float)->int:
    while block_size > 0:
        daysmap[i]=node_id
        i -= 1
        block_size -= 1
    return i

def min_block_available_forwards(daysmap:np.ndarray, i:int, min_block_size:int)->bool:
    while i<len(daysmap):
        if daysmap[i] != 0: return False
        min_block_size -= 1
        if min_block_size < 1: return True
        i += 1
    return False

def set_block_to_node_forwards(daysmap:np.ndarray, i:int, block_size:int, node_id:float)->int:
    while block_size > 0:
        daysmap[i]=node_id
        i += 1
        block_size -= 1
    return i

def map_fixed_target_date_entries(days:dict, daysmap:np.ndarray, options:dict)->tuple:
    # For each fixed target date entry, find the latest available minute
    # and start filling back from there.
    min_block_size = options['min_block_size']
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
                    next_grab = min(num_minutes, min_block_size)
                    if min_block_available_backwards(daysmap, i, next_grab): #daysmap[i]==0:
                        i = set_block_to_node_backwards(daysmap, i, next_grab, node_id)
                        #daysmap[i]=node_id
                        num_minutes -= next_grab
                        fixed_consumed += next_grab
                    else:
                        i -= 1
    print('Minutes consumed by fixed target date Nodes: %d' % fixed_consumed)
    return daysmap, fixed_consumed

def map_variable_target_date_entries(days:dict, daysmap:np.ndarray, options:dict, start_at=0)->tuple:
    min_block_size = options['min_block_size']
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
                    next_grab = min(num_minutes, min_block_size)
                    if min_block_available_forwards(daysmap, i, next_grab): #daysmap[i]==0:
                        i = set_block_to_node_forwards(daysmap, i, next_grab, node_id)
                        #daysmap[i]=node_id
                        num_minutes -= 1
                        variable_consumed += 1
                    else:
                        i += 1
    print('Minutes consumed by variable target date Nodes: %d' % variable_consumed)
    return daysmap, variable_consumed

def get_and_map_more_variable_target_date_entries(days:dict, daysmap:np.ndarray, remaining_minutes:int, options:dict)->tuple:
    num_mapped_days = len(days)
    num_days = 15
    while True:
        print('Getting more variable target date Nodes in %d days.' % num_days)
        options["num_days"] = num_days
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
                        print('Additional variable target date Node minutes found: %d' % more_minutes)
                        return map_variable_target_date_entries(more_days, daysmap, options=options, start_at=len(days))
        num_days += 15
        if num_days > 150:
            print('WARNING: Unable to fill all remaining minutes with variable target date Nodes within 150 days.')
            print('Additional variable target date Node minutes found: %d' % more_minutes)
            return map_variable_target_date_entries(more_days, daysmap, options=options, start_at=len(days))

def get_interval(start_minute:int, end_minute:int, days:dict)->tuple:
    start_delta = datetime.timedelta(minutes=start_minute)
    start_time = thisday_datetime + start_delta
    end_delta = datetime.timedelta(minutes=end_minute)
    end_time = thisday_datetime + end_delta
    return start_time, end_time, (end_minute-start_minute)

def print_entry(start_minute:int, end_minute:int, days:dict, node_id:str, options:dict):
    interval_start, interval_end, interval_minutes = get_interval(start_minute, end_minute, days)
    node_desc = get_node_description(node_id, options).replace('\n',' ')
    print('%s - %s (%s mins): [%s, %s] %s' % (interval_start.strftime('%x %H:%M'), interval_end.strftime('%x %H:%M'), str(interval_minutes), node_id, tdprop_by_node[float(node_id)], node_desc))

def print_map(daysmap:np.ndarray, passed_minutes:int, days:dict, options:dict):
    node_id = 0
    start_minute = 0
    for minute in range(passed_minutes, len(daysmap)):
        if daysmap[minute] != node_id:
            if node_id != 0:
                print_entry(start_minute, minute, days, node_id, options)
            node_id = daysmap[minute]
            start_minute = minute
    print_entry(start_minute, len(daysmap), days, node_id)

DAY_QUARTER_COLOR = {
    0: '000000',
    1: '007f00',
    2: '0000ff',
    3: 'ff00ff',
}

ENTRY_CSV_TEMPLATE = '''%s,%s,%s,%s
'''

MAP_CSV_TEMPLATE = '''start,minutes,tdprop,id
%s
'''

ENTRY_HTML_TEMPLATE = '''<tr>
<td style="color:#%s;">%s</td><td>%s%d mins</td>
<td><a class="nnl" href="http://127.0.0.1:8090/fz/graph/namedlists/_select?id=%s">[select]</a></td>
<td class="id_cell" onclick="window.open('/cgi-bin/fzgraphhtml-cgi.py?edit=%s','_blank');">%s</td>
<td>%s</td><td><a class="node" href="/cgi-bin/fzlink.py?id=%s">%s</a></td></tr>
'''

MAP_HTML_TEMPLATE = '''<html>
<head>
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fz-cards.css">
<link rel="stylesheet" href="/bluetable.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Formalizer: Proposed Schedule</title>
</head>
<body>
<h2>Formalizer: Proposed Schedule</h2>
<button id="darkmode" class="button button2" onclick="switch_light_or_dark();">Light / Dark</button>
<table class="blueTable">
<tbody>
%s
</tbody>
</table>
<hr>
<p>[<a href="/index.html">fz: Top</a>]</p>

<script type="text/javascript" src="/fzuistate.js"></script>
</body>
</html>
'''

def print_entry_csv(start_minute:int, end_minute:int, days:dict, node_id:float, options:dict)->str:
    interval_start, interval_end, interval_minutes = get_interval(start_minute, end_minute, days)
    #node_desc = get_node_description(node_id, options).replace('\n',' ')
    #print('Printing entry for Node '+str(node_id))
    return ENTRY_CSV_TEMPLATE % (
        interval_start.strftime('%x %H:%M'),
        interval_minutes,
        tdprop_by_node[float(node_id)],
        node_id,)
        #node_desc)

def print_entry_html(start_minute:int, end_minute:int, days:dict, node_id:float, options:dict)->str:
    interval_start, interval_end, interval_minutes = get_interval(start_minute, end_minute, days)
    interval_hrs = interval_minutes // 60
    if interval_hrs<1: interval_hrs_str = ''
    elif interval_hrs==1: interval_hrs_str = '%d hr ' % interval_hrs
    else: interval_hrs_str = '%d hrs ' % interval_hrs
    interval_mins = interval_minutes % 60
    dayquarter = interval_start.hour // 6
    node_desc = get_node_description(node_id, options).replace('\n',' ')
    return ENTRY_HTML_TEMPLATE % (
        DAY_QUARTER_COLOR[dayquarter],
        interval_start.strftime('%x %H:%M'),
        interval_hrs_str,
        interval_mins,
        node_id,
        node_id,
        node_id,
        tdprop_by_node[float(node_id)],
        node_id,
        node_desc)

def print_map_csv(daysmap:np.ndarray, passed_minutes:int, days:dict, options:dict):
    node_id = 0
    start_minute = 0
    rows = ''
    for minute in range(passed_minutes, len(daysmap)):
        if daysmap[minute] != node_id:
            if node_id != 0:
                rows += print_entry_csv(start_minute, minute, days, node_id, options)
            node_id = daysmap[minute]
            start_minute = minute
    if node_id != 0:
        rows += print_entry_csv(start_minute, len(daysmap), days, node_id, options)
    map_csv = MAP_CSV_TEMPLATE % rows
    #print(map_csv)
    try:
        print('Writing days map to %s.' % options['outfile'])
        with open(options['outfile'],'w') as f:
            f.write(map_csv)
        print('Days map written to %s.' % options['outfile'])
    except Exception as e:
        print('Unable to write days map to file. Exception: '+str(e))

def print_map_html(daysmap:np.ndarray, passed_minutes:int, days:dict, options:dict):
    node_id = 0
    start_minute = 0
    rows = ''
    for minute in range(passed_minutes, len(daysmap)):
        if daysmap[minute] != node_id:
            if node_id != 0:
                rows += print_entry_html(start_minute, minute, days, node_id, options)
            node_id = daysmap[minute]
            start_minute = minute
    rows += print_entry_html(start_minute, len(daysmap), days, node_id, options)
    map_html = MAP_HTML_TEMPLATE % rows
    try:
        print('Writing days map to %s.' % options['outfile'])
        with open(options['outfile'],'w') as f:
            f.write(map_html)
        print('Days map written to %s.' % options['outfile'])
    except Exception as e:
        print('Unable to write days map to file. Exception: '+str(e))

HELP='''
schedule.py -d <num_days> <-w|-c|-W> [-s <min-minutes>]

Options:
  -d    Number of days to schedule (default: 1).
  -w    Output to web schedule file.
  -c    Output to .csv format.
  -W    Output to .csv for use in web output.
  -s    Minimum block size in minutes (default: 1).

The resulting schedule is written to /tmp%s if -c,
to %s%s if -W,
to %s if -w,
otherwise to %s.

'''

def parse_command_line()->dict:
    from sys import argv

    num_days = 1
    outfile = defaultschedulefile
    web = False
    csv = False
    min_block_size = 1

    cmdline = argv.copy()
    scriptpath = cmdline.pop(0)
    while len(cmdline) > 0:
        arg = cmdline.pop(0)
        if arg == '-h':
            print(HELP % (csvschedulefile, webdata_path, csvschedulefile, webschedulefile, defaultschedulefile))
            exit(0)
        elif arg== '-d':
            num_days = int(cmdline.pop(0))
        elif arg== '-w':
            outfile = webschedulefile
            web = True
        elif arg== '-c':
            outfile = '/tmp'+csvschedulefile
            csv = True
        elif arg== '-W':
            outfile = webdata_path+csvschedulefile
            csv = True
            web = True # This ensures that thecmd are preceded by './'.
        elif arg== '-s':
            min_block_size = int(cmdline.pop(0))

    return {
        "num_days": num_days,
        "outfile": outfile,
        "web": web,
        "csv": csv,
        "min_block_size": min_block_size,
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
    daysmap, fixed_consumed = map_fixed_target_date_entries(days, daysmap, options)
    daysmap, variable_consumed = map_variable_target_date_entries(days, daysmap, options=options)
    remaining_minutes = total_minutes - exact_consumed - fixed_consumed - variable_consumed - passed_minutes
    print('Remaining minutes to fill with variable target date entries: %d' % remaining_minutes)
    if remaining_minutes > 0:
        daysmap, more_variable_consumed = get_and_map_more_variable_target_date_entries(days, daysmap, remaining_minutes, options)
    if options['csv']:
        print_map_csv(daysmap, passed_minutes, days, options)
    else:
        print_map_html(daysmap, passed_minutes, days, options)
    #clean_up_temporary_files()
