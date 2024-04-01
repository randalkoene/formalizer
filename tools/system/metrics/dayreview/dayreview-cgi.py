#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# This CGI handler provides a near-verbatim equivalent access to fzloghtml via web form.

print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from time import strftime
from datetime import datetime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
import json

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage()

cgi_keys = list(form.keys())

#print(str(cgi_keys))

review_date=form.getvalue('date')
review_wakeup=form.getvalue('wakeup')
review_gosleep=form.getvalue('gosleep')

chunks = []
for key_str in cgi_keys:
    if key_str[-8:] == 'category':
        chunks.append( key_str[:-8] )

#print(str(chunks))

category_to_char = {
    'work': 'w',
    'selfwork': 's',
    'system': 'S',
    'nap': 'n',
    'other': 'o',
}

chunkdata = []
for chunk in chunks:
    seconds = form.getvalue(chunk+'seconds')
    category = form.getvalue(chunk+'category')
    chunkhours = float(seconds) / 3600.0
    chunkdata.append( (chunkhours, category_to_char[category]) )

#print(str(chunkdata))

date_object = datetime.strptime(review_date, '%Y%m%d').date()

startmin = int(review_wakeup[-2:])
starthr = int(review_wakeup[:-2])

endmin = int(review_gosleep[-2:])
endhr = int(review_gosleep[:-2])

if endhr < 12:
    endhr = endhr + 24

starthours = float(starthr) + (float(startmin)/60.0)
endhours = float(endhr) + (float(endmin)/60.0)

wakinghours = endhours - starthours

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

def date_stamp(day:datetime)->str:
    return day.strftime('%Y.%m.%d');

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
    #print('%s score = %.2f' % (str(htype), score))
    totscore += score
#print('Total score: %.2f' % totscore)

totintended = get_totintended()
totscore_ratio = totscore / totintended

OUTPUTTEMPLATE='The waking day yesterday was from %s to %s, containing about %.2f waking (non-nap) hours. I did %.2f hours of self-work and %.2f hours of work. I did %.2f hours of System and self-care. Other therefore took %.2f hours. The ratio total performance score is %.2f / %.2f = %.2f.'

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

DAYREVIEW_SUMMARY = '''<html>
<head>
<title>DayReview Summary</title>
</head>
<body>
<h1>DayReview Summary</h1>

<p>
Number of waking hours in previous day: %.2f
</p>

<p>
Summary of hours:
</p>
<pre>
actual | intended | min-lim | max-lim | type

%s
</pre>

<p>
Scores:
</p>

<pre>
%s
</pre>
<p>
Total score: %.2f
</p>

<p>
Formatted for addition to Log:
</p>

%s

</body>
</html>
'''

summary_table = ''
for htype in hours_summary.keys():
    summary_table += ' %5.2f   %5.2f      %5.2f     %5.2f     %s\n' % (hours_summary[htype], get_intended(htype), get_minlim(htype), get_maxlim(htype), str(htype))

score_table = ''
for htype in intended.keys():
    score = get_score(hours_summary[htype], intended[htype])
    score_table += '%s score = %.2f\n' % (str(htype), score)

formatted_for_log = OUTPUTTEMPLATE % (
    str(review_wakeup),
    str(review_gosleep),
    hours_summary['awake'],
    hours_summary['self-work'],
    hours_summary['work'],
    hours_summary['system/care'],
    hours_summary['other'],
    totscore,
    totintended,
    totscore_ratio,
    )

print(DAYREVIEW_SUMMARY % (
        wakinghours,
        summary_table,
        score_table,
        totscore,
        formatted_for_log,
    ))
