#!/usr/bin/python3
#
# Randal A. Koene, 20210308
#
# This CGI handler displays the simple terminal System Metrics extractions in an HTML page.
#

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path
home = str(Path.home())

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

showmetricsfile = webdata_path+'/sysmet-extract.html'
intentionscategoriesfile = "/var/www/webdata/formalizer/categories_a2c.json"
workcategoriesfile = "/var/www/webdata/formalizer/categories_work.json"
main2023categoriesfile = "/var/www/webdata/formalizer/categories_main2023.json"
new2023categoriesfile = "/var/www/webdata/formalizer/categories_new2023.json"

HTML_HEAD='''<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="application/xml+xhtml; charset=UTF-8"/>
<title>System Metrics - Extract</title>
</head>
<body style="color:white; background-color:black">
<pre>
'''

HTML_TAIL='''</pre>
</body>
</html>
'''

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
categories = form.getvalue('categories')
showmap = form.getvalue('showmap')

# *** OBTAIN THIS SOMEHOW!
#with open('./server_address','r') as f:
#    fzserverpq_addrport = f.read()

def try_command_call(thecmd, print_result=True)->str:
    try:
        #print(thecmd, flush=True)
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        child_stdout.close()
        if print_result:
            print(result)
            return ''
        #print(result.replace('\n', '<BR>'))
        return result

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return ''


# def log(msg):
#     with open(logfile,'w') as f:
#         f.write(msg)

def now_timestamp()->str:
    now = datetime.datetime.now()
    return now.strftime('%Y%m%d%H%M')

def most_recent_Sunday_datestamp()->str:
    today = datetime.date.today()
    recent_sunday = today - datetime.timedelta(days=(today.weekday()+1))
    return recent_sunday.strftime('%Y%m%d')

def Sunday_to_now_nweeks_ago(n_weeks:int)->tuple:
    ondayafter = datetime.datetime.now() - datetime.timedelta(days=(7*n_weeks))
    sundaybefore = ondayafter - datetime.timedelta(days=(ondayafter.weekday()+1))
    return (sundaybefore.strftime('%Y%m%d'), ondayafter.strftime('%Y%m%d%H%M'))

def generate_and_show(thecmd):
    try_command_call(thecmd)
    print(f"<!-- The command: {thecmd}  -->")
    with open(showmetricsfile, "r") as f:
        filecontent = f.read()
    print(filecontent)

def show_intentions_categories():
    #print("Content-type:text/html\n\n")
    thecmd = f"./fzlogmap -D 7 -r -o STDOUT -q -C -f {intentionscategoriesfile} | ./aha -b > {showmetricsfile}"
    generate_and_show(thecmd)

def show_work_categories():
    #print("Content-type:text/html\n\n")
    thecmd = f"./fzlogmap -D 7 -r -o STDOUT -q -f {workcategoriesfile} | ./aha -b > {showmetricsfile}"
    generate_and_show(thecmd)

def generate_main2023_thisday(categoriesfile:str):
    thecmd = f"./fzlogmap -D 1 -r -t -o STDOUT -q -f {categoriesfile} | ./aha -n -b > {showmetricsfile}"
    generate_and_show(thecmd)

def generate_main2023_thisweek(categoriesfile:str):
    from_tstamp = most_recent_Sunday_datestamp()
    to_tstamp = now_timestamp()
    thecmd = f"./fzlogmap -1 {from_tstamp} -2 {to_tstamp} -t -o STDOUT -q -f {categoriesfile} | ./aha -n -b > {showmetricsfile}"
    generate_and_show(thecmd)

def generate_main2023_7days(categoriesfile:str):
    thecmd = f"./fzlogmap -D 6 -r -t -o STDOUT -q -f {categoriesfile} | ./aha -n -b > {showmetricsfile}"
    generate_and_show(thecmd)

def show_main2023_categories_map(categoriesfile:str):
    print(HTML_HEAD)
    generate_main2023_thisday(categoriesfile)
    generate_main2023_thisweek(categoriesfile)
    generate_main2023_7days(categoriesfile)
    print(HTML_TAIL)

MAIN2023_PLOT = '''
<table>
<tr>
<td style="width:30%%">%s</td>
<td style="width:30%%">%s</td>
<td style="width:30%%">%s</td>
<td style="width:30%%">%s</td>
</tr>
<p>
<a href="/cgi-bin/sysmet-extract-cgi.py?categories=%s&showmap=yes">Show map of time usage.</a>
</p>
</table>
'''

intended_hours_per_week_main2023 = {
    'Voxa': 40,
    'CCF': 40,
    'OtherValues': 20,
    'Procrastinated': 0,
    'Sleep': 49,
    'other': 19,
}

intended_hours_per_week_new2023 = {
    'Nextup': 40,
    'CCF': 40,
    'OtherValues': 20,
    'Procrastinated': 0,
    'Sleep': 49,
    'other': 19,
}

# Categories in the order specified in the JSON file:
def get_categories_main(json_filepath:str)->list:
    if not exists(json_filepath):
        print('<b>Categories JSON not found at '+json_filepath+'.</b>')
        return None
    try:
        with open(json_filepath, 'r') as f:
            categories_dict = load(f)
        return list(categories_dict.keys())
    except Exception as e:
        print('<b>Exception: '+str(e)+'</b>')
        return None

# hours_per_category is, e.g., { 'Voxa': 52, 'CCF': 42, ... }
def hours_in_categories_order(all_categories:list, hours_per_category:dict)->list:
    return [ hours_per_category[k] if k in hours_per_category else 0 for k in all_categories ]

def bar_plot_actual_and_intended(figtitle:str, allcategories:list, actualhours:list, intendedhours:list)->str:
    import plotly.express as px
    import plotly.io as pxio
    import pandas as pd
    df = pd.DataFrame( dict(categories=allcategories, hours=actualhours) )
    fig = px.bar(
        data_frame=df,
        x='categories',
        y='hours',
        width=400,
        title=figtitle,
        )
    fig.add_scatter(x=allcategories, y=intendedhours, mode='markers', showlegend=False)
    return pxio.to_html(fig, full_html=False)

# Returns hours per category.
def this_plus_n_days(json_filepath:str, n_days=6, use_this_day=False)->dict:
    thecmd = f"./fzlogmap -D {str(n_days)} -r -n -t -o STDOUT -q -F json -f {json_filepath}" # removed -G
    ndaysplustoday_json = try_command_call(thecmd, print_result=False)
    ndaysplustoday = loads(ndaysplustoday_json)
    #print('DEBUG: ndaysplustoday='+str(ndaysplustoday))
    if use_this_day:
        return ndaysplustoday['days'][-1]
    return ndaysplustoday['totals']

# Returns hours per category.
# n_weeks_ago==0 means the most recent Sun-Sat week.
def a_week(json_filepath:str, n_weeks_ago:int)->dict:
    from_tstamp, to_tstamp = Sunday_to_now_nweeks_ago(n_weeks_ago)
    thecmd = f"./fzlogmap -1 {from_tstamp} -2 {to_tstamp} -n -t -o STDOUT -q -F json -G -f {json_filepath}"
    week_json = try_command_call(thecmd, print_result=False)
    week = loads(week_json)
    week_totals = {}
    for cat_data in week:
        week_totals[cat_data['header']] = sum(cat_data['days'][0:7])
    return week_totals

def score(actualhours:list, intendedhours:list)->float:
    dt_list = [ abs(actualhours[i]-intendedhours[i]) for i in range(len(actualhours)) ]
    sum_dt = sum(dt_list)
    max_t = sum(intendedhours)
    return 1.0 - (sum_dt/max_t)

# n_weeks=1 means just the most recente Sun-Sat week.
def weeks_scores(json_filepath:str, allcategories:list, intendedhours:list, n_weeks:int)->list:
    if n_weeks<1:
        return None
    scores = []
    today_tstamp = datetime.datetime.now().strftime('%Y%m%d%H%M')
    from_tstamp, _ = Sunday_to_now_nweeks_ago(n_weeks-1)
    thecmd = f"./fzlogmap -1 {from_tstamp} -2 {today_tstamp} -n -t -o STDOUT -q -F json -G -f {json_filepath}"
    weeks_json = try_command_call(thecmd, print_result=False)
    weeks = loads(weeks_json)
    for n in range(n_weeks):
        numdays = n*7
        week_totals = {}
        for cat_data in weeks:
            week_totals[cat_data['header']] = sum(cat_data['days'][numdays:numdays+7])
        week_score = score(hours_in_categories_order(allcategories, week_totals), intendedhours)
        scores.append(week_score)
    return scores

def line_graph_scores(figtitle:str, scores:list)->str:
    import plotly.express as px
    import plotly.io as pxio
    import pandas as pd
    df = pd.DataFrame( dict(weeks=[x for x in range(len(scores))], scores=scores) )
    fig = px.line(
        data_frame=df,
        x='weeks',
        y='scores',
        width=400,
        title=figtitle,
        )
    fig.add_hline(y=1.0)
    return pxio.to_html(fig, full_html=False)

def show_main2023_categories(categoriesfile:str, catselect:str, intended_hours_per_week:dict):
    import plotly.express as px
    import plotly.io as pxio
    import pandas as pd

    all_categories = get_categories_main(categoriesfile)
    intended_hours = hours_in_categories_order(all_categories, intended_hours_per_week)

    thisday_thisday = this_plus_n_days(categoriesfile, n_days=1, use_this_day=True)
    thisday_content = bar_plot_actual_and_intended(
        'Today',
        all_categories,
        hours_in_categories_order(all_categories, thisday_thisday),
        [x/7.0 for x in intended_hours],
        )

    thisweek_totals = a_week(categoriesfile, n_weeks_ago=0)
    thisweek_content = bar_plot_actual_and_intended(
        'This week (Sunday to Saturday)',
        all_categories,
        hours_in_categories_order(all_categories, thisweek_totals),
        intended_hours,
        )

    sevendays_totals = this_plus_n_days(categoriesfile, n_days=6, use_this_day=False)
    sevendays_content = bar_plot_actual_and_intended(
        'The last 7 days',
        all_categories,
        hours_in_categories_order(all_categories, sevendays_totals),
        intended_hours,
        )

    num_weeks = 26
    scores_content = ''
    scores_weeks = weeks_scores(categoriesfile, all_categories, intended_hours, n_weeks=num_weeks)
    if scores_weeks is not None:
        scores_content = line_graph_scores(
            'Scores for the previous %d weeks' % num_weeks,
            scores_weeks,
            )

    print(HTML_HEAD)
    print(MAIN2023_PLOT % (thisday_content, thisweek_content, sevendays_content, scores_content, catselect))
    print(HTML_TAIL)
    
if __name__ == '__main__':
    if (categories == 'work'):
        show_work_categories()
        sys.exit(0)

    if (categories == 'main2023'):
        if (showmap == 'yes'):
            show_main2023_categories_map(main2023categoriesfile)
            sys.exit(0)
        else:
            show_main2023_categories(main2023categoriesfile, 'main2023', intended_hours_per_week_main2023)
            sys.exit(0)

    if (categories == 'new2023'):
        if (showmap == 'yes'):
            show_main2023_categories_map(new2023categoriesfile)
            sys.exit(0)
        else:
            show_main2023_categories(new2023categoriesfile, 'new2023', intended_hours_per_week_new2023)
            sys.exit(0)

    show_intentions_categories()
    sys.exit(0)
