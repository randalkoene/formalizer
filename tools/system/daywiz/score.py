#!/usr/bin/env python3
# score.py
# Copyright 2023 Randal A. Koene
# License TBD
#
# Score graphing.
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

from wiztable import *
from fzhtmlpage import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

HREFBASE = 'http://localhost/'
NODELINKCGI = 'cgi-bin/fzlink.py?id='

# ====================== Data store:

# TODO: *** Switch to using the database.
JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'

SCOREPAGE_HEAD_STYLE='''
'''

SCOREPAGE_BODY_FRAME='''<table>
%s
</table>
'''

# SCORE_DAY_FRAME='''%s
# '''
SCORE_DAY_FRAME='''<tr><td>%s</td></tr>
'''

# SCORE_LINE_FRAME='''<tr><td>%s</td><td>%s</td><td>%s</td></tr>
# '''

class score_daypage_wiztable:
    def __init__(self, day_key: str, day_data: dict, is_new: bool):
        self.day_key = day_key
        self.day = datetime.strptime(day_key, '%Y.%m.%d')
        self.day_data = day_data
        self.lines_list = WIZTABLE_LINES
        self.lines_indices = self.make_wiztable_index() # Helps with searching.
        # if not is_new:
        #     if 'wiztable' in self.day_data:
        #         self.merge_data(self.day_data['wiztable'])
        self.merge_data(self.day_data)
        self.lines = [ wiztable_line(self.day, self.lines_list[i], i) for i in range(len(self.lines_list)) ]
        self.checkbox_metrics = [ 0, 0 ] # Number of checkboxes in table, number of checked checkboxes.
        self.number_metrics = [0, 0 ]    # Number of number inputs in table, number of filled number inputs.
        self.score = 0.0
        self.score_possible = 0.0
        self.lines_parsed = 0

    def generate_dayscore(self) ->tuple:
        self.checkbox_metrics = [ 0, 0 ]
        self.number_metrics = [0, 0 ]
        self.score = 0.0
        self.score_possible = 0.0
        for wizline in self.lines:
            wizline.state_tuple()
            addpossible, addscore = wizline.add_to_checkbox_metrics(self.checkbox_metrics)
            self.score_possible += addpossible
            self.score += addscore
            addpossible, addscore = wizline.add_to_number_metrics(self.number_metrics)
            self.score_possible += addpossible
            self.score += addscore
            self.lines_parsed += 1
        return self.score, self.score_possible

    def make_wiztable_index(self) ->dict:
        wiztable_dict = {}
        for i in range(len(self.lines_list)):
            wiztable_dict[self.lines_list[i][WIZTABLE_LINES_ITEM]] = i
        return wiztable_dict

    def merge_data(self, day_data: list):
        for wizline in day_data:
            _id = wizline[0]
            if _id in self.lines_indices:
                idx = self.lines_indices[_id]
                self.lines_list[idx][WIZTABLE_LINES_TIME] = wizline[1]
                self.lines_list[idx][WIZTABLE_LINES_STATE] = wizline[2]

    def get_data(self) ->list:
        return [ self.lines[i].get_data() for i in range(len(self.lines)) ]

    def generate_html_body(self) ->str:
        score, scorepossible = self.generate_dayscore()
        day_str = '%s: %s/%s %s' % (self.day_key, str(score), str(scorepossible), str(self.lines_parsed))
        return SCORE_DAY_FRAME % day_str

    def generate_html_body_and_graph_data(self)->tuple:
        html_str = self.generate_html_body()
        score_ratio = self.score/self.score_possible
        return html_str, score_ratio, self.day

class score_tables:
    def __init__(self, day: datetime, data: dict):
        self.day = day
        self.data = data

        self.days = [ score_daypage_wiztable(day_key, self.data[day_key], False) for day_key in self.data ]

    def generate_html_head(self) ->str:
        return SCOREPAGE_HEAD_STYLE

    def generate_html_body(self) ->str:
        self.content = ''
        score_data = []
        score_days = []
        for i in range(len(self.days)):
            html_str, score_ratio, score_day = self.days[i].generate_html_body_and_graph_data()
            self.content += html_str
            score_data.append(score_ratio)
            score_days.append(score_day)
            #self.content += self.days[i].generate_html_body()
        fig = px.scatter(score_data, x=score_days, y=score_data)
        self.content = pxio.to_html(fig, full_html=False)
        return SCOREPAGE_BODY_FRAME % self.content

    def generate_html_tail(self) ->str:
        return ''


class scorepage(fz_htmlpage):
    def __init__(self, directives: dict):
        super().__init__()
        self.day_str = directives['date']
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

         # A list used to navigate to the data to display:
        self.selection = [ 'wiztable' ] #directives['selectors'].split(',')

        # Retrieve data:
        self.data = {}
        self.load_json()

        # Prepare HTML output template:
        self.html_std = fz_html_standard('score.py')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('Scores')

        # Prepare head, body and tail HTML generators:
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.html_title, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

        # Select metric to work with:
        self.data = self.select_data()
        if self.data is not None:
            self.scores = score_tables(self.day, self.data)
            self.head_list.append( self.scores )
            self.body_list.append( self.scores )
            self.tail_list.append( self.scores )

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

    def load_json(self):
        data = self.get_data_from_database()
        if len(data)==0:
            self.data = {}
        else:
            for tablekey in data:
                self.data[tablekey] = {}
                for dbdaykey in data[tablekey]:
                    wizday = dbdaykey[0:4]+'.'+dbdaykey[4:6]+'.'+dbdaykey[6:8]
                    self.data[tablekey][wizday] = data[tablekey][dbdaykey]

    def select_data(self) ->dict:
        entry_point = self.data
        for selector in self.selection:
            if selector in entry_point:
                entry_point = entry_point[selector]
            else:
                self.body_list.append( fz_errorpage('Unable to select metric %s from %s: ' % ( str(selector), str(self.selection))) )
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
    _page = scorepage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_form(form)
        _page.show()
    else:
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)
    _page = scorepage(directives)

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
