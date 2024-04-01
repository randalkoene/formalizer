#!/usr/bin/env python3
# dayreview.py
# Copyright 2024 Randal A. Koene
# License TBD

from datetime import datetime
import json
import subprocess

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

def sum_of_type(typeid:str)->float:
    hours = 0.0
    for i in range(len(chunkdata)):
        if chunkdata[i][1] == typeid[0]:
            hours += chunkdata[i][0]
    return hours

hours_summary = {
    'self-work': sum_of_type('s'),
    'work': sum_of_type('w'),
    'system/care': sum_of_type('S'),
    'dayspan': wakinghours,
    'nap': sum_of_type('n'),
}
hours_summary['awake'] = hours_summary['dayspan'] - hours_summary['nap']
hours_summary['sleep'] = 24 - hours_summary['awake']
hours_summary['other'] = hours_summary['awake'] - (hours_summary['self-work']+hours_summary['work']+hours_summary['system/care'])

intended = { # target, min-lim, max-lim, penalize-above, max-score
    'self-work': (6.0, 2.0, 8.0, False, 10.0),
    'work': (6.0, 3.0, 9.0, False, 10.0),
    'sleep': (7.0, 5.5, 8.5, True, 10.0),
}
# No review delivers an automatic score of 0.0,
# i.e. missing out on 30 points.

def get_intended(htype:str)->float:
    if htype in intended:
        return intended[htype][0]
    return 0.0

def get_minlim(htype:str)->float:
    if htype in intended:
        return intended[htype][1]
    return 0.0

def get_maxlim(htype:str)->float:
    if htype in intended:
        return intended[htype][2]
    return 0.0

def get_totintended()->float:
    tot = 0.0
    for key_str in intended:
        tot += intended[key_str][-1]
    return tot

print('Summary of hours:')
print('actual | intended | min-lim | max-lim | type')
for htype in hours_summary.keys():
    print(' %5.2f   %5.2f      %5.2f     %5.2f     %s' % (hours_summary[htype], get_intended(htype), get_minlim(htype), get_maxlim(htype), str(htype)))

def get_score(actual: float, scoring_data:tuple)->float:
    target, minlim, maxlim, penalizeabove, maxscore = scoring_data
    if actual < minlim: return 0.0
    if penalizeabove:
        if actual > maxlim: return 0.0
    if actual > target:
        if not penalizeabove: return maxscore
        ratio_above = (actual - target) / (maxlim - target) # E.g. (8.0 - 6.0) / (9.0 - 6.0) = 2/3
        reduce_by = ratio_above * (maxscore - 1.0)          # E.g. 2/3 * (10.0 - 1.0) = 6.0
        return maxscore - reduce_by                         # E.g. 10.0 - 6.0 = 4.0
    ratio_below = (target - actual) / (target - minlim)     # E.g. (6.0 - 4.0) / (6.0 - 3.0) = 2/3
    reduce_by = ratio_below * (maxscore - 1.0)              # E.g. 2/3 * (10.0 - 1.0) = 6.0
    return maxscore - reduce_by                             # E.g. 10.0 - 6.0 = 4.0

totscore = 0.0
for htype in intended.keys():
    score = get_score(hours_summary[htype], intended[htype])
    print('%s score = %.2f' % (str(htype), score))
    totscore += score
print('Total score: %.2f' % totscore)

totintended = get_totintended()
totscore_ratio = totscore / totintended

print('\nFormatted for addition to Log:\n\n')

OUTPUTTEMPLATE='The waking day yesterday was from %s to %s, containing about %.2f waking (non-nap) hours. I did %.2f hours of self-work and %.2f hours of work. I did %.2f hours of System and self-care. Other therefore took %.2f hours. The ratio total performance score is %.2f / %.2f = %.2f.'

print(OUTPUTTEMPLATE % (
        str(starttimestr),
        str(endtimestr),
        hours_summary['awake'],
        hours_summary['self-work'],
        hours_summary['work'],
        hours_summary['system/care'],
        hours_summary['other'],
        totscore,
        totintended,
        totscore_ratio,
        ))

def score_data_summary_line(key_str:str, actual_data:dict, intended_data:dict)->tuple:
    return (
            actual_data[key_str],
            intended_data[key_str][0],
            get_score(actual_data[key_str], intended_data[key_str]),
            intended_data[key_str][-1],
        )

score_data_summary = {}
actual_hours_total = 0.0
intended_hours_total = 0.0
for key_str in intended:
    actual_hours_total += hours_summary[key_str]
    intended_hours_total += intended[key_str][0]
    score_data_summary[key_str] = score_data_summary_line(key_str, hours_summary, intended)
score_data_summary['totscore'] = ( actual_hours_total, intended_hours_total, totscore, totintended )

def update_score_file(day:datetime, score_data:dict):
    # Read existing data from file.
    scorefile = '/var/www/webdata/formalizer/dayreview_scores.json'
    try:
        with open(scorefile, 'r') as f:
            data = json.load(f)
    except:
        print('No dayreview_scores.json file found, starting a fresh one.')
        data = {}
    # Update data.
    dstamp = date_stamp(day)
    data[dstamp] = score_data
    # Save data.
    try:
        with open(scorefile, 'w') as f:
            json.dump(data, f)
    except Exception as e:
        print('Unable to save data: '+str(e))

update_score_file(date_object, score_data_summary)
