#!/usr/bin/python3
#
# Randal A. Koene, 20240406
#
# Create a plot that shows dayreview scores.

scorefile = '/var/www/webdata/formalizer/dayreview_scores.json'

out_path = '/var/www/webdata/formalizer/dayreview_scores.html'

import json

def make_dayreview_plot(figtitle:str, dates:list, values:list, value_axis_name:str, out_format:str='show', width=None, height=None):
    import plotly.express as px
    import plotly.io as pxio
    import pandas as pd

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

print('Sending html doc (%d bytes) to %s' % (len(html_doc), out_path))

with open(out_path, 'w') as f:
    f.write(html_doc)

print('Done.')
