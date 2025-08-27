#!/usr/bin/env python3
# dayreview.py
# Copyright 2024 Randal A. Koene
# License TBD

from datetime import datetime
import json
import subprocess

from dayreview_algorithm import dayreview_algorithm

results = {}
def try_subprocess_check_output(thecmdstring: str, resstore: str, verbosity=1) -> int:
    if verbosity > 1:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    try:
        res = subprocess.check_output(thecmdstring, shell=True)
    except subprocess.CalledProcessError as cpe:
        if verbosity > 0:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
            if (cpe.returncode>0):
                print(f'Formalizer error return code: {cpe.returncode}')
        return cpe.returncode
    else:
        if resstore:
            results[resstore] = res
        if verbosity > 1:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

def fzdate_stamp(day:datetime)->str:
    return day.strftime('%Y%m%d');

def date_stamp(day:datetime)->str:
    return day.strftime('%Y.%m.%d');

date_str = input('Please specify the date to which this review applies in the format "2024.03.25": ')
try:
    date_object = datetime.strptime(date_str, '%Y.%m.%d').date()
    print(type(date_object))
    print(date_object)  # printed in default format
    print('Date stamp: '+date_stamp(date_object))
except:
    print('Unrecognized date format.')
    exit(1)


status = try_subprocess_check_output('fzloghtml -q -a '+fzdate_stamp(date_object)+' -D 2 -i -F json -o STDOUT', 'review')
if (status != 0):
    print('Error calling fzloghtml.')
    exit(1)
data_str = results['review'].decode()
auto_data = json.loads(data_str)

print('Please answer the following questions about the preceding day.\n')

starttimestr = input('What time did the day begin ("HHMM", def="%s")? : ' % auto_data['wakeup'])
if starttimestr == '':
    starttimestr = auto_data['wakeup']
    print('Using '+starttimestr)
startmin = int(starttimestr[-2:])
starthr = int(starttimestr[:-2])

endtimestr =   input('What time did the day end ("HHMM", def="%s")?   : ' % auto_data['gosleep'])
if endtimestr == '':
    endtimestr = auto_data['gosleep']
    print('Using '+endtimestr)
endmin = int(endtimestr[-2:])
endhr = int(endtimestr[:-2])

if endhr < 12:
    endhr = endhr + 24

starthours = float(starthr) + (float(startmin)/60.0)
endhours = float(endhr) + (float(endmin)/60.0)

if starthours > endhours:
    # I started the new waking day before 00:00.
    starthours -= 24.0

wakinghours = endhours - starthours

print('Number of waking hours in previous day: %.2f' % wakinghours)

INSTRUCTIONS = '''
Please review the Log Chunks in the previous day. Take note of
Chunks of the following kind:

  w - Work
  s - Self-Work
  S - System or self-care
  n - Nap
  o - Other

For each Chunk, you will be asked to indicate the hours or minutes
that were logged (e.g. "3.4" hours or "97" minutes). Then you will
be asked to indicate the type (w, s, S, n, o).

If you give an empty response you will be asked if you wish to redo
the current entry or if you have completed entries.
'''

print(INSTRUCTIONS)

def get_data(inputstr:str, default)->tuple:
    while True:
        redo=False
        done=False
        if default:
            datastr = input(inputstr+' (def=%s): ' % str(default))
        else:
            datastr = input(inputstr+': ')
        if not datastr:
            if default:
                return (True, str(default))
            while not redo and not done:
                redo_or_done = input('Do you wish to [r]edo, or are you [d]one? ')
                if not redo_or_done:
                    redo=True
                else:
                    if redo_or_done[0]=='r':
                        redo=True
                    elif redo_or_done[0]=='d':
                        done=True
                    else:
                        redo=True
        if done:
            return (False, '')
        if not redo:
            return (True, datastr)

chunkdata = []
while True:

    auto_available = len(auto_data['chunks'])>0
    auto_mins = None
    if auto_available:
        print('Auto-chunk available: ')
        print(auto_data['chunks'][0]['node'])
        print(auto_data['chunks'][0]['log'])
        seconds = auto_data['chunks'][0]['seconds']
        auto_mins = seconds // 60
    validdata, data = get_data('Hours or minutes in Log Chunk', auto_mins)
    if not validdata:
        break
    if '.' in data:
        try:
            chunkhours = float(data)
        except:
            print('Discarding entry (%s). Not interpretable as hours or minuntes.' % data)
            continue
    else:
        try:
            chunkhours = int(data)/60.0
        except:
            print('Discarding entry (%s). Not interpretable as hours or minuntes.' % data)
            continue
    if chunkhours == 0.0:
        print('Discarding entry (%s), because interval hours are zero.' % str(data))
        continue

    auto_category = None
    if auto_available and auto_data['chunks'][0]['category'] != '?':
        auto_category = auto_data['chunks'][0]['category']
    validdata, data = get_data('Type of Chunk ([w]ork, [s]elf-work, [S]ystem/care, [n]ap), [o]other', auto_category)
    if not validdata:
        continue
    if data[0] not in 'wsSno':
        print('Discarding entry, because "%s" is an unknown type.' % str(data[0]))
        continue
    chunkdata.append( (chunkhours, data[0]) )
    print('%s Chunks entered.\n' % str(len(chunkdata)))
    if auto_available:
        auto_data['chunks'] = auto_data['chunks'][1:]

# === From here is shared between the command line and cgi approaches:

review_algo = dayreview_algorithm(wakinghours, chunkdata)

score_data_summary = review_algo.calculate_scores()

review_algo.update_score_file(date_object, score_data_summary)

summary_table_dict = review_algo.summary_table()

score_table_dict = review_algo.score_table()

hours_summary = review_algo.get_hours_summary()

print('Summary of hours:')
print('actual | intended | min-lim | max-lim | type')
for htype in summary_table_dict:
    print(' %5.2f   %5.2f      %5.2f     %5.2f     %s' % summary_table_dict[htype])

print('Total score: %.2f' % review_algo.totscore)

print('\nFormatted for addition to Log:\n\n')

print(review_algo.string_for_log(starttimestr, endtimestr))
