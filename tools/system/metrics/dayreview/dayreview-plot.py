#!/usr/bin/python3
#
# Randal A. Koene, 20240406
#
# Create a plot that shows dayreview scores.

scorefile = '/var/www/webdata/formalizer/dayreview_scores.json'

out_path = '/var/www/webdata/formalizer/dayreview_scores.html'

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
#from subprocess import Popen, PIPE

import json
import plotly.express as px
import plotly.io as pxio
import pandas as pd
import argparse

form = cgi.FieldStorage()

cgioutput = form.getvalue('cgioutput')
earliest = form.getvalue('earliest')
latest = form.getvalue('latest')
workselfwork = form.getvalue('workselfwork')

if not cgioutput:
    parser = argparse.ArgumentParser(description="Plot DayReview Scores.")
    parser.add_argument("-c", "--cgioutput", type=str, help="Operate at CGI script")
    parser.add_argument("-e", "--earliest", type=str, help="Earliest date in format 2025.02.20")
    parser.add_argument("-l", "--latest", type=str, help="Latest date in format 2025.03.04")
    parser.add_argument("-w", "--workselfwork", type=str, help="Show WORK and SELFWORK only")
    args = parser.parse_args()

    if args.cgioutput:
        cgioutput = args.cgioutput
    if args.earliest:
        earliest = args.earliest
    if args.latest:
        latest = args.latest
    if args.workselfwork:
        workselfwork = args.workselfwork

if cgioutput:
    print("Content-type:text/html\n")

def make_dayreview_plot(figtitle:str, dates:list, values:list, value_axis_name:str, out_format:str='show', width=None, height=None):
    data_dict = {
        'date': dates,
        value_axis_name: values,
    }

    df = pd.DataFrame( data_dict )
    fig = px.bar(
        data_frame=df,
        x='date',
        y=value_axis_name,
        width=400,
        title=figtitle,
        )
    #fig.add_scatter(x=allcategories, y=intendedhours, mode='markers', showlegend=False)
    if out_format=='show':
        pxio.show(fig)
        return
    elif out_format=='html':
        return pxio.to_html(fig, full_html=False)
    else:
        return pxio.to_image(fig,out_format, width, height) # e.g. 'png', 'jpg', 'svg'

def reformulate_category_data(category:str, data:dict, date_keys:list)->tuple:
    actual_hours = []
    percent_hours = []
    ratio_score = []
    for date_key in date_keys:
        if category in data[date_key]:
            hours, intended, score, possible = data[date_key][category]
            actual_hours.append( hours )
            percent_hours.append( 100.0*(hours/intended) )
            ratio_score.append( score/possible )
        else:
            actual_hours.append( -1.0 )
            percent_hours.append( -1.0 )
            ratio_score.append( -1.0 )
    return (actual_hours, percent_hours, ratio_score)

def reformulate_total_scores(data:dict, date_keys:list)->list:
    total_score_ratios = []
    for date_key in date_keys:
        skip1, skip2, score, possible = data[date_key]['totscore']
        total_score_ratios.append( score/possible )
    return total_score_ratios

with open(scorefile, 'r') as f:
    data = json.load(f)

if earliest and latest:
    new_data = {}
    for k in data:
        if k >= earliest and k <= latest:
            new_data[k] = data[k]
    data = new_data

dates_str_list = list(data.keys())

categories_list = list(data[dates_str_list[-1]].keys())

#print(dates_str_list)

#print(categories_list)

info='''
We are going to use:
- Actual hours dedicated.
- The percentage of intended hours based on the intention stored
  for that date and category.
- Ratio score based on the possible score stored for that date
  and category.
- Ratio total score achieved based on the total score possible
  stored for that date.

The principal x axis used is the date.
We create independent data arrays for those dates for each
category and for the total score.
'''

if not cgioutput:
    print(info)

reformulated = {}
for category in categories_list:
    if category != 'totscore':
        reformulated[category] = reformulate_category_data(category, data, dates_str_list)

total_score_ratios = reformulate_total_scores(data, dates_str_list)

#print(reformulated)

#print(total_score_ratios)

#html_plot = make_dayreview_plot('Day Review Score', dates_str_list, total_score_ratios, 'score ratio')
#print('Size of HTML plot: %d' % len(html_plot))

#make_dayreview_plot('Day Review Score', dates_str_list, total_score_ratios, 'score ratio', 'show')

SVG_TEMPLATE='''
<svg width="%d" height="%d" xmlns="http://www.w3.org/2000/svg">
%s
</svg>
'''

HTML_TEMPLATE='''<html>
<head>
<title>Day Review Scores</title>
</head>
<h1>Day Review Scores</h1>

%s

</html>
'''

TABLE_CELL_TEMPLATE='''<td>
%s
</td>
'''

TABLE_ROW_TEMPLATE='''<tr>
%s
</tr>
'''

TABLE_TEMPLATE='''<table><tbody>
%s
</tbody></table>
'''

def all_data_plots()->str:
    table_rows_html = ''

    svg_plot = make_dayreview_plot('Day Review Score', dates_str_list, total_score_ratios, 'score ratio', 'svg')
    svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
    table_cell_html = TABLE_CELL_TEMPLATE % svg_html
    table_rows_html += TABLE_ROW_TEMPLATE % table_cell_html

    for category in categories_list:
        if category != 'totscore':
            is_a_main_category = category == 'self-work' or category == 'work' or category == 'sleep'
            cells_html = ''

            svg_plot = make_dayreview_plot(category+' review actual hours', dates_str_list, reformulated[category][0], 'actual hours', 'svg')
            svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
            cells_html += TABLE_CELL_TEMPLATE % svg_html

            if is_a_main_category:
                svg_plot = make_dayreview_plot(category+' review percent hours', dates_str_list, reformulated[category][1], 'pct hours', 'svg')
                svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
                cells_html += TABLE_CELL_TEMPLATE % svg_html

            if is_a_main_category:
                svg_plot = make_dayreview_plot(category+' review score ratio', dates_str_list, reformulated[category][2], 'score ratio', 'svg')
                svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
                cells_html += TABLE_CELL_TEMPLATE % svg_html

            table_rows_html += TABLE_ROW_TEMPLATE % cells_html

    table_html = TABLE_TEMPLATE % table_rows_html

    html_doc = HTML_TEMPLATE % table_html

    return html_doc

def work_and_selfwork_only()->str:
    table_rows_html = ''

    cells_html = ''
    for category in categories_list:
        if category == 'work':
            svg_plot = make_dayreview_plot('work hours', dates_str_list, reformulated[category][0], 'actual hours', 'svg')
            svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
            cells_html += TABLE_CELL_TEMPLATE % svg_html

        if category == 'self-work':
            svg_plot = make_dayreview_plot('self-work hours', dates_str_list, reformulated[category][0], 'actual hours', 'svg')
            svg_html = SVG_TEMPLATE % ( 400, 400, svg_plot )
            cells_html += TABLE_CELL_TEMPLATE % svg_html

    table_rows_html += TABLE_ROW_TEMPLATE % cells_html

    table_html = TABLE_TEMPLATE % table_rows_html

    html_doc = HTML_TEMPLATE % table_html

    return html_doc

if workselfwork:
    html_doc = work_and_selfwork_only()
else:
    html_doc = all_data_plots()

if not cgioutput:
    print('Sending html doc (%d bytes) to %s' % (len(html_doc), out_path))

with open(out_path, 'w') as f:
    f.write(html_doc)

if not cgioutput:
    print('Done.')
