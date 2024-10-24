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
from io import StringIO
from subprocess import Popen, PIPE
import traceback

from datetime import datetime
from time import time
import json
from os.path import exists
import plotly.express as px
import plotly.io as pxio

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

webdata_path = "/var/www/webdata/formalizer"
error_file = webdata_path + '/metrics_error.log'
debugdatabase = webdata_path+'/metrics_database.debug'

def DebugDatabase(msg:str):
    with open(debugdatabase, 'w') as f:
        f.write(msg)

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

WEIGHT_FIGURE='''<tr><td>Weight:<br>%s</td></tr>
'''

print("Content-type:text/html\n\n")

class nutrition_line:
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

# class wiztable_line:
#     def __init__(self, data, idx:int):
#         self.data = data # This is something like ['weight',0,'']
#         self.idx = idx

#         # NOTE: *** At present, this shows only weight.
#         self._t = None
#         self._weight = None
#         if data[0] == 'weight':
#             if data[1] != 0:
#                 self._t = datetime.fromtimestamp(data[1])
#                 self._weight = float(data[2])

#     def time_str(self) ->str:
#         return self._t.strftime('%Y.%m.%d %H:%M')

#     def generate_html_body(self) ->str:
#         if self._t is None:
#             return ''
#         else:
#             return METRIC_LINE_FRAME % (
#                     'weight',
#                     self.time_str(),
#                     str(self._weight),
#                 )

class unknown_line:
    def __init__(self, selector:str):
        self.selector = selector

    def generate_html_body(self) ->str:
        return 'Unknown metrics selector: "%s".' % self.selector

class metric_day:
    def __init__(self, data, day_key: str, selector: str):
        self.data = data # Assumed to be a list.
        self.day_key = day_key
        self.selector = selector

        # Parse data:
        if selector == 'nutrition':
            self.lines = [ nutrition_line(self.data[i], i) for i in range(len(self.data)) ]
        else:
            self.lines = [ unknown_line(selector) ]
        # self._t = datetime.fromtimestamp(data[0])
        # self._description = data[1]
        # self._quantity = data[2]

    # def time_str(self) ->str:
    #     return self._t.strftime('%Y.%m.%d %H:%M')

    def generate_html_body(self) ->str:
        multi_line = ''
        for line in self.lines:
            if line is not None:
                multi_line += line.generate_html_body()
        return METRIC_DAY_FRAME % multi_line

class metric_graph_day:
    def __init__(self, data, day_key:str, selector:str):
        self.data = data # Assumed to be a list.
        self.day_key = day_key
        self.selector = selector

        # Parse data:
        self.weight = None
        if selector == 'wiztable':
            # NOTE: *** At present, this shows only weight.
            for data_item in data:
                if data_item[0] == 'weight':
                    if data_item[2] != '':
                        self.weight = float(data_item[2])

    # def generate_html_body(self) ->str:
    #     multi_line = ''
    #     for line in self.lines:
    #         if line is not None:
    #             multi_line += line.generate_html_body()
    #     return METRIC_DAY_FRAME % multi_line

    def generate_html_body_and_graph_data(self) ->tuple:
        weight_day = datetime.strptime(self.day_key, '%Y.%m.%d')
        weight_value = self.weight
        html_str = ''
        return html_str, weight_value, weight_day

class metric_tables:
    '''
    This inserts the HTML code for a list of nutrition consumptions.
    '''
    def __init__(self, day: datetime, metric_data: dict, selector: str):
        self.day = day
        self.metric_data = metric_data

        #print('===========> '+str(self.metric_data))
        #self.lines = [ metric_line(self.metric_data[i], i) for i in range(len(self.metric_data)) ]
        self.days = [ metric_day(self.metric_data[day_key], day_key, selector) for day_key in self.metric_data ]
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

class metric_graphs:
    '''
    This inserts the HTML code for a graph that shows weight changes over time.
    '''
    def __init__(self, day: datetime, metric_data: dict, selector: str):
        self.day = day
        self.metric_data = metric_data

        #print('===========> '+str(self.metric_data))
        #self.lines = [ metric_line(self.metric_data[i], i) for i in range(len(self.metric_data)) ]
        self.days = [ metric_graph_day(self.metric_data[day_key], day_key, selector) for day_key in self.metric_data ]
        self.content = ''

    def generate_html_head(self) ->str:
        return METRICPAGE_HEAD_STYLE

    def generate_html_body(self) ->str:
        self.content = ''
        metric_data = []
        metric_days = []
        for i in range(len(self.days)):
            html_str, weight_value, weight_day = self.days[i].generate_html_body_and_graph_data()
            if weight_value is not None:
                self.content += html_str
                metric_data.append(weight_value)
                metric_days.append(weight_day)
        #fig = px.scatter(metric_data, x=metric_days, y=metric_data)
        fig = px.line(metric_data, x=metric_days, y=metric_data)
        fig.update_yaxes(range=[173, 240])
        self.content = WEIGHT_FIGURE % pxio.to_html(fig, full_html=False)
        return '%s' % self.content

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
        self.metric, self.selector = self.select_metric_data()
        if self.metric is not None:
            if self.selector == 'nutrition':
                self.tables = metric_tables(self.day, self.metric, self.selector)
            else:
                self.tables = metric_graphs(self.day, self.metric, self.selector)
            self.head_list.append( self.tables )
            self.body_list.append( self.tables )
            self.tail_list.append( self.tables )

    def database_call(self, thecmd:str):
        try:
            p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
            (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
            child_stdin.close()
            result = child_stdout.read()
            # with open(debugdatabase, 'a') as f:
            #     f.write('Call: '+thecmd+'\n\n')
            #     f.write(result+'\n\n')
            error = child_stderr.read()
            child_stdout.close()
            child_stderr.close()
            if error:
                with open(error_file, 'w') as f:
                    f.write(error)
                return None
            return result

        except Exception as ex:
            with open(error_file, 'w') as e:              
                e.write(str(ex))
                f = StringIO()
                traceback.print_exc(file=f)
                a = f.getvalue().splitlines()
                for line in a:
                    e.write(line)
            return None

    def get_data_from_database(self)->dict:
        thecmd = "./fzmetricspq -q -d formalizer -s randalk -E STDOUT -R -i all -F json -o STDOUT -w true -n true -e true -a true -m true -c true"
        datastr = self.database_call(thecmd)
        try:
            data = json.loads(datastr)
            if not isinstance(data, dict) or len(data)==0:
                return {}
            else:
                return data
        except:
            return {}

    def load_metrics_json(self):
        data = self.get_data_from_database()
        if len(data)==0:
            self.metrics_data = {}
        else:
            for tablekey in data:
                self.metrics_data[tablekey] = {}
                for dbdaykey in data[tablekey]:
                    wizday = dbdaykey[0:4]+'.'+dbdaykey[4:6]+'.'+dbdaykey[6:8]
                    self.metrics_data[tablekey][wizday] = data[tablekey][dbdaykey]
            #DebugDatabase(json.dumps(self.metrics_data))

    def select_metric_data(self) ->tuple:
        entry_point = self.metrics_data
        the_selector = ''
        for selector in self.metric_selection:
            if selector in entry_point:
                entry_point = entry_point[selector]
                the_selector = selector
            else:
                self.body_list.append( fz_errorpage('Unable to select metric %s from %s: ' % ( str(selector), str(self.metric_selection))) )
                return None
        return entry_point, the_selector

    def show(self):
        #print("Content-type:text/html\n\n")
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
        # Disabled on 2024-10-22, because it would need to work with the database now.
        #_page.update_from_form(form)
        _page.show()
    else:
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)
    _page = metricspage(directives)

    if directives['cmd'] == 'update':
        # Disabled on 2024-10-22, because it would need to work with the database now.
        #_page.update_from_list(directives['args'])
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
