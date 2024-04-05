#!/usr/bin/python3
#
# Randal A. Koene, 20200921
#
# This CGI handler provides a near-verbatim equivalent access to fzloghtml via web form.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
from datetime import datetime
import traceback
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

# Create instance of FieldStorage 
form = cgi.FieldStorage()

cgi_keys = list(form.keys())

print("Content-type:text/html\n\n")

# Uses './', because it is run as CGI script from /usr/lib/cgi-bin.
try:
    from dayreview_algorithm import dayreview_algorithm
except Exception as e:
    print('Exception while importing dayreview_algorithm: '+str(e))
    exit(0)

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

DAYREVIEW_SUMMARY = '''<html>
<head>
<title>DayReview Summary</title>
</head>
<body>
<h1>DayReview Summary</h1>

<p>
Number of waking hours in previous day: %.2f
</p>

<h3>Summary of hours</h3>

<pre>
actual | intended | min-lim | max-lim | max-score | type

%s
</pre>

<h3>Scores</h3>

<pre>
%s
</pre>
<p>
Total score: %.2f
</p>

<h3>Formatted for addition to Log</h3>

<table border="1"><tr><td>
%s
</td></tr></table>

<p>
<h3>Notes</h3
<ol>
<li>If a category is penalized below and the actual hours are below the minimum limit then 0 score is earned.</li>
<li>If a category is penalized above and the actual hours are above the maximum limit then 0 score is earned.</li>
<li>If the actual hours are on target or in a non-penalized range then the maximum score is earned.</li>
<li>If within the penalized range, but not beyond the limit, then the ratio to the limit is used to penalize the score, earning 1.0 at the limit.</li>
<li>No review delivers an automatic score of 0.0, i.e. missing out on %d points.</li>
</ol>
</p>

</body>
</html>
'''

#print(str(cgi_keys))

review_date=form.getvalue('date')
review_wakeup=form.getvalue('wakeup')
review_gosleep=form.getvalue('gosleep')

chunks = []
for key_str in cgi_keys:
    if key_str[-8:] == 'category':
        chunks.append( key_str[:-8] )

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

review_algo = dayreview_algorithm(wakinghours, chunkdata)

score_data_summary = review_algo.calculate_scores()

review_algo.update_score_file(date_object, score_data_summary)

summary_table_dict = review_algo.summary_table()

score_table_dict = review_algo.score_table()

hours_summary = review_algo.get_hours_summary()

summary_table = ''
for htype in summary_table_dict:
    summary_table += ' %5.2f   %s       %s      %s      %s        %s\n' % summary_table_dict[htype]

score_table = ''
for htype in score_table_dict:
    score_table += '%s score = %.2f\n' % ( str(htype), score_table_dict[htype] )

print(DAYREVIEW_SUMMARY % (
        wakinghours,
        summary_table,
        score_table,
        review_algo.totscore,
        review_algo.string_for_log(review_wakeup, review_gosleep),
        review_algo.totintended,
    ))
