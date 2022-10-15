#!/usr/bin/env python3
# metrics.py
# Copyright 2022 Randal A. Koene
# License TBD
#
# Metrics data access.
#
# For details, see: https://trello.com/c/JssbodOF .
#
# This can be launched as a CGI script.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout

from datetime import datetime
from time import time
import json
from os.path import exists

from fzhtmlpage import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

# ====================== Data store:

# TODO: *** Switch to using the database.
#JSON_DATA_PATH='/home/randalk/.formalizer/.daywiz_data.json' # Permission issues.
JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'

METRICPAGE_HEAD_STYLE='''
'''

METRICPAGE_BODY_FRAME='''<table>
%s
</table>
'''

METRIC_DAY_FRAME='''%s
'''

METRIC_LINE_FRAME='''<tr><td>%s</td><td>%s</td><td>%s</td></tr>
'''

class metric_line:
    def __init__(self, data, idx: int):
        self.data = data
        self.idx = idx

        # Parse data:
        self._t = datetime.fromtimestamp(data[0])
        self._description = data[1]
        self._quantity = data[2]

    def time_str(self) ->str:
        return self._t.strftime('%Y.%m.%d %H:%M')

    def generate_html_body(self) ->str:
        return METRIC_LINE_FRAME % (
            self.time_str(),
            self._description,
            str(self._quantity),
        )

class metric_day:
    def __init__(self, data, day_key: str):
        self.data = data # Assumed to be a list.
        self.day_key = day_key

        # Parse data:
        self.lines = [ metric_line(self.data[i], i) for i in range(len(self.data)) ]
        # self._t = datetime.fromtimestamp(data[0])
        # self._description = data[1]
        # self._quantity = data[2]

    # def time_str(self) ->str:
    #     return self._t.strftime('%Y.%m.%d %H:%M')

    def generate_html_body(self) ->str:
        multi_line = ''
        for i in range(len(self.data)):
            multi_line += self.lines[i].generate_html_body()
        return METRIC_DAY_FRAME % multi_line

class metric_tables:
    def __init__(self, day: datetime, metric_data: dict):
        self.day = day
        self.metric_data = metric_data

        #print('===========> '+str(self.metric_data))
        #self.lines = [ metric_line(self.metric_data[i], i) for i in range(len(self.metric_data)) ]
        self.days = [ metric_day(self.metric_data[day_key], day_key) for day_key in self.metric_data ]
        self.content = ''

    def generate_html_head(self) ->str:
        return METRICPAGE_HEAD_STYLE

    def generate_html_body(self) ->str:
        self.content = ''
        for i in range(len(self.days)):
            self.content += self.days[i].generate_html_body()
        return METRICPAGE_BODY_FRAME % self.content

    def generate_html_tail(self) ->str:
        return ''

class metricspage(fz_htmlpage):
    def __init__(self, directives: dict):
        super().__init__()
        self.day_str = directives['date']
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

        # A list used to navigate to the metric to display:
        self.metric_selection = directives['selectors'].split(',')

        # Retrieve data:
        self.metrics_data = {}
        self.load_metrics_json()

        # Prepare HTML output template:
        self.html_std = fz_html_standard('metrics.py')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('Metrics')

        # Prepare head, body and tail HTML generators:
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.html_title, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

        # Select metric to work with:
        self.metric = self.select_metric_data()
        if self.metric is not None:
            self.tables = metric_tables(self.day, self.metric)
            self.head_list.append( self.tables )
            self.body_list.append( self.tables )
            self.tail_list.append( self.tables )

    def load_metrics_json(self):
        if exists(JSON_DATA_PATH):
            try:
                with open(JSON_DATA_PATH, 'r') as f:
                    self.metrics_data = json.load(f)
            except:
                self.metrics_data = {}

    def select_metric_data(self) ->dict:
        entry_point = self.metrics_data
        for selector in self.metric_selection:
            if selector in entry_point:
                entry_point = entry_point[selector]
            else:
                self.body_list.append( fz_errorpage('Unable to select metric %s from %s: ' % ( str(selector), str(self.metric_selection))) )
                return None
        return entry_point

    def show(self):
        print("Content-type:text/html\n\n")
        print(self.generate_html())

# ====================== Entry parsers:

def get_directives(formfields: cgi.FieldStorage) ->dict:
    directives = {
        'cmd': formfields.getvalue('cmd'),
        'date': formfields.getvalue('date'),
        'selectors': formfields.getvalue('selectors')
    }
    return directives

def check_directives(directives: dict) ->dict:
    if not directives['cmd']:
        directives['cmd'] = 'show'
    if not directives['date']:
        directives['date'] = datetime.today().strftime('%Y.%m.%d')
    else:
        try:
            _date = datetime.strptime(directives['date'], '%Y.%m.%d')
        except:
            try:
                _date = datetime.strptime(directives['date'], '%Y-%m-%d')
            except:
                _date = datetime.today()
        date_str = _date.strftime('%Y.%m.%d')
        directives['date'] = date_str
    if not directives['selectors']:
        directives['selectors'] = 'NONE'
    return directives

def launch_as_cgi():
    directives = get_directives(form)
    directives = check_directives(directives)
    _page = metricspage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_form(form)
        _page.show()
    else:
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)
    _page = metricspage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_list(directives['args'])
        _page.show()
    else:
        _page.show()

if __name__ == '__main__':
    from sys import argv
    if len(argv) > 2:
        if argv[2] == 'date':
            launch(directives={ 'cmd': argv[1], 'date': argv[2], 'selectors': argv[3], })
        else:
            launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'selectors': argv[2], })
    else:
        launch_as_cgi()
