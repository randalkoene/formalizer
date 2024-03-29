#!/usr/bin/env python3
# daywiz.py
# Copyright 2022 Randal A. Koene
# License TBD
#
# Day wizard page generator, parser.
#
# For details, see: https://trello.com/c/JssbodOF .
#
# This can be launched as a CGI script.
#
# This can also be used by other scripts, e.g. consumed.py, to update the daywiz log.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout

from datetime import datetime, timedelta
from time import time
import json
from os.path import exists
import traceback

from wiztable import *
from fzhtmlpage import *
from fznutrition import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

webdata_path = "/var/www/webdata/formalizer"

HREFBASE = 'http://localhost/'
NODELINKCGI = 'cgi-bin/fzlink.py?id='

# ====================== Data store:

# TODO: *** Switch to using the database.
#JSON_DATA_PATH='/home/randalk/.formalizer/.daywiz_data.json' # Permission issues.
JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'
#NEW_JSON_DATA_PATH='/var/www/webdata/formalizer/.new_daywiz_data.json'

# Collection of tables and their prefix abbreviations:
data_tables = {
    'wiztable': 'wiz',
    'nutrition': 'nutri',
    'exercise': 'exerc',
    'accounts': 'acct',
    'milestones': 'mile',
    'comms': 'comms',
}

bool_to_checked = {
    0: '',
    1: 'checked',
}

truefalse_to_bool = {
    'true': 1,
    'false': 0,
}

global last_line_node
last_line_node=''

# ====================== Exercise information:

exercises = {
    'pushups': [ 'pushups', ],
    'rowing': [ 'rows', ],
    'dancing': [ 'minutes', ],
    'rollerblading': [ 'minutes', ],
    'poi': [ 'minutes', ],
    'firepoi': [ 'minutes', ],
    'hillhiking': [ 'minutes', ],
    'kayaking': [ 'minutes', ],
    'skiing': [ 'minutes', ],
    'stamina': [ 'minutes', ],
    'weights': [ 'minutes', ],
    'wood splitting': [ 'minutes', ],
    'crunches': [ 'crunches', ],
    'squats': [ 'squats', ],
}

# ====================== Debug helpers:

global global_debug_str
global_debug_str = ''

DEBUG_GOT_HERE='''Content-type:text/html

<html>
<body>
<h1>Got here: %s</h1>
</body>
</html>
'''

global got_here_active
got_here_active = False
def gothere(mark: str):
    global got_here_active
    if got_here_active:
        print(DEBUG_GOT_HERE % mark)
        exit(0)

def update_total_scores(day: datetime, score: float, max_possible: float):
    total_score_path = webdata_path + '/daywiz_total_scores.json'
    try:
        # Get existing scores stored.
        if exists(total_score_path):
            with open(total_score_path, 'r') as f:
                total_score_dict = json.load(f)
        else:
            total_score_dict = {}
        # Update current day score.
        daystr = day.strftime('%Y.%m.%d')
        #total_score_dict[daystr] = (score, max_possible)
        total_score_dict[daystr] = int(10.0*score/max_possible)
        # Save updated day scores.
        with open(total_score_path, 'w') as f:
            json.dump(dict(sorted(total_score_dict.items())[-7:]), f)
    except Exception as e:
        pass

# TODO: *** Remove this when no longer needed.
class debug_test:
    def __init__(self, directives: dict, formfields: cgi.FieldStorage):
        from os import getcwd
        #self.current_dir = getcwd()
        self.directives = directives
        self.form_input = self.get_form_input(formfields)
        self.print_this = ''

    def get_form_input(self, formfields: cgi.FieldStorage):
        form_input = {}
        for input_key in formfields.keys():
            form_input[input_key] = formfields.getvalue(input_key) # *** Safer to use getlist().
        return form_input

    def generate_html_head(self) ->str:
        head_str = '<!-- Input Directives: %s -->\n' % str(self.directives)
        head_str += '<!-- Form input: %s -->\n' % str(self.form_input)
        return head_str

    def generate_html_body(self) ->str:
        return self.print_this
        #return '<p><b>%s</b></p>\n' % self.current_dir

def debugmark(mark:str, err=None):
    try:
        with open('/var/www/webdata/debugmark-'+str(mark), 'w') as f:
            if err is None:
                f.write('NO ERROR'+mark)
            else:
                f.write('Exception: '+str(err))
    except:
        pass

# ====================== String templates used to generate content for page areas:

TIME_FRAME='<input type="number" min=0 max=23 id="%s" value="%s" style="width: 3em;" %s>:<input type="number" min=0 max=59 id="%s" value="%s" style="width: 3em;" %s>'

DAYPAGE_WIZTABLE_STYLE='''<style>
.secondcolfixedw td:nth-child(2) {
  width: 8em;
}
</style>
'''
DAYPAGE_TABLES_FRAME='''<table class="col_right_separated">
<tr><th>Wizard Record</th><th><a href="https://trello.com/b/t2RmUmlN/milestones-prioritization-experimental-visualization">Milestones</a> <a href="https://trello.com/b/tlgXjZBm/system-experiments">In-Rotation</a><br>
[List of links to in-rotation milestone Nodes belongs here with progress indication.]<br>
[A link to milestone planning should be here, something like a tree.]</th></tr>
<tr valign="top">
<td width="50%%">%s</td>
<td>%s</td>
</tr>
</table>
'''

WIZTABLE_TOP='''<table class="secondcolfixedw">
<tr><th>t recommended</th><th>t actual</th><th>description</th><th>state</th><th>extra</th></tr>
'''
WIZTABLE_SUMMARY='''<tr><td></td><td></td><td><a href="/cgi-bin/score.py?cmd=show&selectors=wiztable">score</a>: %s/%s</td><td>%s/%s</td><td></td></tr>
'''

# WIZLINE_FRAME='''<tr><td>%s</td><td><input type="time" id="%s" value="%s" %s></td><td>%s</td><td>%s</td><td>%s</td></tr>
# '''
WIZLINE_FRAME='''<tr><td>%s</td><td>%s</td><td %s>[%s, <a href="%s">node</a>] %s [<a href="/formalizer/system-documentation.html#wiztable-%s">ref</a>]</td><td>%s</td><td>%s</td></tr>
'''
WIZLINE_VISIBLE_TOPBORDER='style="border-top: 1px solid black;"'
WIZLINE_RECOMMENDED_FRAME='%s - %s'
WIZLINE_CHECKBOX_FRAME='<input id="%s" type="checkbox" %s %s>'
WIZLINE_NUMBER_FRAME='<input id="%s" type="text" value="%s" style="width: 8em;" %s>'
WIZLINE_TEXT_FRAME='<input id="%s" type="text" value="%s" %s>'
WIZLINE_LONGER_TEXT_FRAME='<input id="%s" type="text" value="%s" style="width: 40em;" %s>'

NUTRI_ACCOUNTS_TABLES_FRAME='''<table>
<tr>
<td>%s</td>
</tr>
<tr>
<td>&nbsp;</td>
</tr>
<tr>
<th>Nutrition <a href="/cgi-bin/metrics.py?cmd=show&selectors=nutrition">Record</a> (<a href="/cgi-bin/nutrition.py?cmd=show">Planner</a>)</th>
</tr>
<tr>
<td>%s</td>
</tr>
<tr>
<td>&nbsp;</td>
</tr>
<tr>
<th>Exercise</th>
</tr>
<tr>
<td>%s</td>
</tr>
<tr>
<td>&nbsp;</td>
</tr>
<tr>
<th>Accounts</th>
</tr>
<tr>
<td>%s</td>
</tr>
<tr>
<th>Communications</th>
</tr>
<tr>
<td>%s</td>
</tr>
</table>
'''

MILESTONES_TABLE_HEAD='''<table>
<tr>
'''
MILESTONE_CELL_TEMPLATE='<td><a href="/cgi-bin/fzlink.py?id=%s">%s</a><br>%s</td>\n'

NUTRI_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th><a href="/cgi-bin/consumed.py?cmd=show&date=%s&selectors=nutrition">consumed</a></th><th>quantity</th><th>units</th><th>calories</th></tr>\n'
CONSUMED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="text" value="%s" style="width: 8em;" %s></td><td>%s</td><td>%s</td>
'''
NUTRI_TABLE_SUMMARY='<tr><th>Prev:</th><th>%s</th><th></th><th>Sum:</th><th>%s</th></tr>\n'
NUTRI_TABLE_TARGET='<tr><th>Unknown:</th><th>%s</th><th></th><th>Target:</th><th>%s</th></tr>\n'
ENTRYLINE_TR_FRAME='''<tr><td><input type="time" id="nutri_add_time" value="%s" %s></td><td><input id="nutri_add_name" type="text" %s></td><td><input id="nutri_add_quantity" type="text" style="width: 8em;" %s></td><td>(units)</td><td>(calories)</td>
'''

EXERCISE_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>exercised</th><th>quantity</th><th>units</th></tr>\n'
EXERCISED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="text" value="%s" style="width: 8em;" %s></td><td>%s</td>
'''
EXERCISEENTRY_TR_FRAME='''<tr><td><input type="time" id="exerc_add_time" value="%s" %s></td><td><input id="exerc_add_name" type="text" %s></td><td><input id="exerc_add_quantity" type="text" style="width: 8em;" %s></td><td>(units)</td>
'''

ACCOUNTS_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>description</th><th>spent</th><th>received</th><th>category</th></tr>\n'
ACCOUNTED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="number" value="%s" style="width: 8em;" %s></td><td><input id="%s" type="number" value="%s" style="width: 8em;" %s></td><td><input id="%s" type="text" value="%s" %s></td>
'''
ACCOUNTENTRY_TR_FRAME='''<tr><td><input type="time" id="acct_add_time" value="%s" %s></td><td><input id="acct_add_name" type="text" %s></td><td><input id="acct_add_spent" type="number" style="width: 8em;" %s></td><td><input id="acct_add_received" type="number" style="width: 8em;" %s></td><td><input id="acct_add_category" type="text" %s></td>
'''

COMMS_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>contact name</th><th>done</th><th>note</th></tr>\n'
COMMS_TR_FRAME='''<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>
'''
COMMSENTRY_TR_FRAME='''<tr><td><input type="time" id="comms_add_time" value="%s" %s></td><td><input id="comms_add_name" type="text" %s></td><td><input id="comms_add_done" type="checkbox" %s></td><td><input id="comms_add_note" type="text" style="width: 40em;" %s></td>
'''

# ====================== Helpful standardized functions:

def state_html(idstr: str, intype: str, state) ->str:
    if intype == 'checkbox':
        # Expects that state is int 0 or 1.
        return WIZLINE_CHECKBOX_FRAME % ( idstr, bool_to_checked[int(state)], SUBMIT_ON_CHANGE )
    elif intype == 'number':
        # Expects that state is float or int.
        return WIZLINE_NUMBER_FRAME % ( idstr, str(state), SUBMIT_ON_INPUT )
    elif intype == 'text':
        # Expects that state is str.
        return WIZLINE_TEXT_FRAME % ( idstr, str(state), SUBMIT_ON_INPUT )
    elif intype == 'longertext':
        return WIZLINE_LONGER_TEXT_FRAME % ( idstr, str(state), SUBMIT_ON_INPUT )
    else:
        return ''

def datetime2datestr(day: datetime) ->str:
    return day.strftime('%Y.%m.%d')

def datestr2datetime(datestr: str) ->datetime:
    return datetime.strptime(datestr, '%Y.%m.%d')

def consumed_calories_and_units(nutri_data: list) ->tuple:
    name = str(nutri_data[1]).lower()
    quantity = float(nutri_data[2])
    if name in nutrition:
        return ( name, quantity, int(quantity*nutrition[name][0]), nutrition[name][1] )
    else:
        return ( name, quantity, 5000, '(unit)' ) # As a warning that there is unmatched data.

def consumed_calories(nutri_data: list) ->int:
    name = str(nutri_data[1]).lower()
    quantity = float(nutri_data[2])
    if name in nutrition:
        return int(quantity*nutrition[name][0])
    else:
        return 5000 # As a warning that there is unmatched data.

def day_calories(day_consumption: list) ->int:
    calories = 0
    for nutri_data in day_consumption:
        calories += consumed_calories(nutri_data)
    return calories

# ====================== Classes that manage content in page areas:

class wiztable_line:
    # data_list is a line of WIZTABLE_LINES.
    def __init__(self, day: datetime, data_list: list, idx: int):
        self.day = day
        self._idx = idx
        self._node = ''
        self._nodelink = ''
        self._weight = 0
        self._id = ''
        self._t = None # Beware! Remains None if t_logged <= 0.
        self._hr_ideal_from = 0
        self._hr_ideal_to = 0
        self._type = ''
        self._description = ''
        self._state = ''
        self.checkbox_metrics = [0, 0] # Number of checkboxes on this line, number that are checked.
        self.number_metrics = [0, 0]   # Number of number inputs on this line, number that are filled.
        self.parse_from_list(data_list)

    # data_list is a line of WIZTABLE_LINES.
    def parse_from_list(self, data_list: list):
        if len(data_list) >= WIZTABLE_LINE_LENGTH:
            self._node = str(data_list[WIZTABLE_LINES_NODE])
            self._nodelink = HREFBASE+NODELINKCGI+self._node
            self._weight = data_list[WIZTABLE_LINES_WEIGHT]
            self._id = str(data_list[WIZTABLE_LINES_ITEM])
            t_logged = float(data_list[WIZTABLE_LINES_TIME])
            if t_logged > 0:
                self._t = datetime.fromtimestamp(t_logged)
            self._type = str(data_list[WIZTABLE_LINES_TYPE])
            self._hr_ideal_from = int(data_list[WIZTABLE_LINES_HRFROM])
            self._hr_ideal_to = int(data_list[WIZTABLE_LINES_HRTO])
            self._description = str(data_list[WIZTABLE_LINES_DESC])
            self._state = str(data_list[WIZTABLE_LINES_STATE])

    def weight(self)->float:
        return abs(self._weight)

    # === Produce HTML:

    def id_str(self, pre='') ->str:
        return pre+self._id
        #return pre+str(self._idx)

    def zpadded_time(self, hr: int, mins: int) ->str:
        if hr > 23:
            hr = 23
            mins = 59
        return str(hr).zfill(2)+':'+str(mins).zfill(2)

    def time_str(self) ->str:
        dtime = t_run if self._t is None else self._t
        return dtime.strftime('%H:%M')

    def recommended_str(self) ->str:
        return WIZLINE_RECOMMENDED_FRAME % (self.zpadded_time(self._hr_ideal_from, 0), self.zpadded_time(self._hr_ideal_to, 0))

    def state_str(self) ->str:
        if self._type == 'checkbox':
            self.checkbox_metrics[0] = 1
            if self._state == 'checked':
                self.checkbox_metrics[1] = 1
            return WIZLINE_CHECKBOX_FRAME % ( self.id_str('wiz_state_'), self._state, SUBMIT_ON_CHANGE )
        elif self._type == 'number':
            self.number_metrics[0] = 1
            if self._state != '':
                if self._weight < 0:
                    self.number_metrics[1] = 1
                else:
                    self.number_metrics[1] = float(self._state) # The value should be between 0.0 and 1.0.
            return WIZLINE_NUMBER_FRAME % ( self.id_str('wiz_state_'), str(self._state), SUBMIT_ON_INPUT )
        else:
            return ''

    def extra_str(self) ->str:
        return '' # TODO: *** Determine if we need this for something.

    def time_tuple(self) ->tuple:
        dtime = t_run if self._t is None else self._t
        return ( dtime.hour, dtime.minute )

    def time_html(self) ->str:
        h, m = self.time_tuple()
        return TIME_FRAME % ( self.id_str('wiz_th_'), str(h), SUBMIT_ON_INPUT, self.id_str('wiz_tm_'), str(m), SUBMIT_ON_INPUT )

    def generate_html_tr(self) ->str:
        global last_line_node
        if last_line_node != self._node:
            top_border = WIZLINE_VISIBLE_TOPBORDER
        else:
            top_border = ''
        last_line_node = self._node
        return WIZLINE_FRAME % ( self.recommended_str(), self.time_html(), top_border, str(self._weight), self._nodelink, self._description, self._id, self.state_str(), self.extra_str() )
        #return WIZLINE_FRAME % ( self.recommended_str(), self.id_str('wiz_t_'), self.time_str(), SUBMIT_ON_INPUT, self._description, self.state_str(), self.extra_str() )

    # === Produce data dictionary:

    def get_data(self) ->list:
        t = 0 if self._t is None else datetime.timestamp(self._t)
        return [ self._id, t, self._state, ]

    # === Member functions for data updates:

    def update_time(self, t_new: str) ->bool:
        t_list = t_new.split(':')
        if self._t is None:
            self._t = self.day
        self._t = self._t.replace(hour=int(t_list[0]), minute=int(t_list[1]))
        return True

    def update_hour(self, hr_new: str) ->bool:
        if self._t is None:
            self._t = self.day
        self._t = self._t.replace(hour=int(hr_new))
        return True

    def update_minute(self, min_new: str) ->bool:
        if self._t is None:
            self._t = self.day
        self._t = self._t.replace(minute=int(min_new))
        return True

    def update_state(self, new_state: str) ->bool:
        if self._type == 'checkbox':
            self._state = 'checked' if new_state=='true' else ''
        else:
            self._state = str(new_state)
        if self._t is None:
            return self.update_time(datetime.now().strftime('%H:%M'))
        return True

# Left side checklist objects.
class daypage_wiztable:
    def __init__(self, day: datetime, day_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.lines_list = WIZTABLE_LINES
        self.lines_indices = self.make_wiztable_index() # Helps with searching.
        if not is_new:
            if 'wiztable' in self.day_data:
                self.merge_data(self.day_data['wiztable'])
        self.lines = [ wiztable_line(self.day, self.lines_list[i], i) for i in range(len(self.lines_list)) ]
        self.checkbox_metrics = [ 0, 0 ] # Number of checkboxes in table, number of checked checkboxes.
        self.number_metrics = [0, 0 ]    # Number of number inputs in table, number of filled number inputs.
        self.score = 0.0
        self.score_possible = 0.0

    def merge_data(self, day_data: list):
        for wizline in day_data:
            _id = wizline[0]
            if _id in self.lines_indices:
                idx = self.lines_indices[_id]
                self.lines_list[idx][WIZTABLE_LINES_TIME] = wizline[1]
                self.lines_list[idx][WIZTABLE_LINES_STATE] = wizline[2]

    def make_wiztable_index(self) ->dict:
        wiztable_dict = {}
        for i in range(len(self.lines_list)):
            wiztable_dict[self.lines_list[i][WIZTABLE_LINES_ITEM]] = i
        return wiztable_dict

    # === Produce HTML:

    def add_to_checkbox_metrics(self, chkmetrics_pair: list, weight: float):
        self.checkbox_metrics[0] += chkmetrics_pair[0]
        self.checkbox_metrics[1] += chkmetrics_pair[1]
        self.score_possible += weight * chkmetrics_pair[0]
        self.score += weight * chkmetrics_pair[1]

    def add_to_number_metrics(self, nummetrics_pair: list, weight: float):
        self.number_metrics[0] += nummetrics_pair[0]
        if nummetrics_pair[1] > 0:
            self.number_metrics[1] += 1
        self.score_possible += weight * nummetrics_pair[0]
        self.score += int(weight * nummetrics_pair[1])

    def generate_html_body(self) ->str:
        self.checkbox_metrics = [ 0, 0 ]
        self.number_metrics = [0, 0 ]
        self.score = 0.0
        self.score_possible = 0.0
        table_str = WIZTABLE_TOP
        for wizline in self.lines:
            table_str += wizline.generate_html_tr()
            weight = wizline.weight()
            self.add_to_checkbox_metrics(wizline.checkbox_metrics, weight)
            self.add_to_number_metrics(wizline.number_metrics, weight)
        update_total_scores(self.day, self.score, self.score_possible)
        table_str += WIZTABLE_SUMMARY % ( str(int(self.score)), str(int(self.score_possible)), str(self.checkbox_metrics[1]+self.number_metrics[1]), str(self.checkbox_metrics[0]+self.number_metrics[0]) )
        return table_str + '</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return [ self.lines[i].get_data() for i in range(len(self.lines)) ]

    # === Member functions for data updates:

    def update(self, element: str, line_id: str, new_val: str) ->bool:
        if line_id in self.lines_indices:
            idx = self.lines_indices[line_id]
            if element == 'th':
                return self.lines[idx].update_hour(new_val)
            if element == 'tm':
                return self.lines[idx].update_minute(new_val)
            if element == 'state':
                return self.lines[idx].update_state(new_val)
            return False
        return False

# in_rotation milestone data: [ NodeID, description, pattern_pct, ]
class daypage_milestones:
    def __init__(self, day: datetime, day_data: dict, multiday_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.multiday_data = multiday_data
        self.in_rotation = []
        if not is_new:
            if 'milestones' in self.day_data:
                self.in_rotation = self.day_data['milestones']

    # === Produce HTML:

    def milestone_html(self, milestone_data: list, idx: int) ->str:
        node_id, description, pattern_pct = milestone_data
        return MILESTONE_CELL_TEMPLATE % ( str(node_id), str(description), str(pattern_pct) )

    def generate_html_body(self) ->str:
        table_str = MILESTONES_TABLE_HEAD
        for i in range(len(self.in_rotation)):
            table_str += self.milestone_html(self.in_rotation[i], i)
        return table_str + '</tr>\n</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return self.in_rotation

class daypage_nutritable:
    def __init__(self, day: datetime, day_data: dict, multiday_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.multiday_data = multiday_data
        self.logged = [] # Each element contains: [ time, name, quantity ]
        if not is_new:
            if 'nutrition' in self.day_data:
                self.logged = self.day_data['nutrition']
        self.calories = 0

    # === Produce HTML:

    def time_tuple(self, idx: int) ->tuple:
        t = self.logged[idx][0]
        dtime = t_run if t==0 else datetime.fromtimestamp(t)
        return ( dtime.hour, dtime.minute )

    def time_html(self, idx: int) ->str:
        h, m = self.time_tuple(idx)
        return TIME_FRAME % ( 'nutri_edit_th_'+str(idx), str(h), SUBMIT_ON_INPUT, 'nutri_edit_tm_'+str(idx), str(m), SUBMIT_ON_INPUT )

    def consumed_html(self, idx: int) ->str:
        name, quantity, calories, unit = consumed_calories_and_units(self.logged[idx])
        self.calories += calories
        return CONSUMED_TR_FRAME % (
            self.time_html(idx),
            'nutri_edit_name_'+str(idx),
            name,
            SUBMIT_ON_INPUT,
            'nutri_edit_quantity_'+str(idx),
            str(quantity),
            SUBMIT_ON_INPUT,
            unit,
            str(calories) )

    def entryline_html(self) ->str:
        return ENTRYLINE_TR_FRAME % (datetime.now().strftime('%H:%M'), SUBMIT_ON_INPUT, SUBMIT_ON_INPUT, SUBMIT_ON_INPUT)

    def generate_html_body(self) ->str:
        table_str = NUTRI_TABLE_HEAD % self.day.strftime('%Y.%m.%d')
        self.calories = 0
        for i in range(len(self.logged)):
            table_str += self.consumed_html(i)
        table_str += NUTRI_TABLE_SUMMARY % ( str(self.multiday_data['caloriestotal']), str(self.calories) )
        table_str += NUTRI_TABLE_TARGET % ( str(self.multiday_data['dayscaloriesunknown']), str(self.multiday_data['caloriethreshold']) )
        table_str += self.entryline_html()
        return table_str + '</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return self.logged

    # === Member functions for data updates:

    def update_add(self, element: str, new_val: str) ->bool:
        add_list = [ time(), '', 0 ]
        if element == 'name':
            add_list[1] = new_val
            self.logged.append(add_list)
            return True
        return False

    def update_edit(self, idx: int, element: str, new_val: str) ->bool:
        if idx > len(self.logged):
            return False
        edit_list = self.logged[idx]

        if element == 'name':
            edit_list[1] = new_val
            self.logged[idx] = edit_list
            return True

        if element == 'th':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=int(new_val), minute=m)
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'tm':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=h, minute=int(new_val))
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'quantity':
            edit_list[2] = float(new_val)
            self.logged[idx] = edit_list
            return True
        return False

    def update_addto(self, name:str, new_val:str)->bool:
        # Is the name already here?
        idx = -1
        for i in range(len(self.logged)):
            if self.logged[i][1]==name:
                idx = i
                break
        if idx<0: # Not there yet, append.
            add_list = [ time(), name, float(new_val) ]
            self.logged.append(add_list)
            return True
        else: # Found, increase.
            edit_list = self.logged[idx]
            edit_list[0] = time()
            edit_list[2] += float(new_val)
            self.logged[idx] = edit_list
            return True

class daypage_exercise:
    def __init__(self, day: datetime, day_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.logged = []
        if not is_new:
            if 'exercise' in self.day_data:
                self.logged = self.day_data['exercise']

    # === Produce HTML:

    def time_tuple(self, idx: int) ->tuple:
        t = self.logged[idx][0]
        dtime = t_run if t==0 else datetime.fromtimestamp(t)
        return ( dtime.hour, dtime.minute )

    def time_html(self, idx: int) ->str:
        h, m = self.time_tuple(idx)
        return TIME_FRAME % ( 'exerc_edit_th_'+str(idx), str(h), SUBMIT_ON_INPUT, 'exerc_edit_tm_'+str(idx), str(m), SUBMIT_ON_INPUT )

    def exercised_html(self, data_list: list, idx: int) ->str:
        name = str(self.logged[idx][1])
        unit = exercises[name][0] if name in exercises else 'unknown'
        return EXERCISED_TR_FRAME % ( self.time_html(idx), 'exerc_edit_name_'+str(idx), name, SUBMIT_ON_INPUT, 'exerc_edit_quantity_'+str(idx), str(self.logged[idx][2]), SUBMIT_ON_INPUT, unit )

    def entryline_html(self) ->str:
        return EXERCISEENTRY_TR_FRAME % (datetime.now().strftime('%H:%M'), SUBMIT_ON_INPUT, SUBMIT_ON_INPUT, SUBMIT_ON_INPUT)

    def generate_html_body(self) ->str:
        table_str = EXERCISE_TABLE_HEAD
        for i in range(len(self.logged)):
            table_str += self.exercised_html(self.logged[i], i)
        table_str += self.entryline_html()
        return table_str + '</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return self.logged

    # === Member functions for data updates:

    def update_add(self, element: str, new_val: str) ->bool:
        add_list = [ time(), '', 0 ]
        if element == 'name':
            add_list[1] = new_val.lower()
            self.logged.append(add_list)
            return True
        return False

    def update_edit(self, idx: int, element: str, new_val: str) ->bool:
        if idx > len(self.logged):
            return False
        edit_list = self.logged[idx]

        if element == 'name':
            edit_list[1] = new_val.lower()
            self.logged[idx] = edit_list
            return True

        if element == 'th':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=int(new_val), minute=m)
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'tm':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=h, minute=int(new_val))
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'quantity':
            edit_list[2] = float(new_val)
            self.logged[idx] = edit_list
            return True
        return False

class daypage_accounts:
    def __init__(self, day: datetime, day_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.logged = []
        if not is_new:
            if 'accounts' in self.day_data:
                self.logged = self.day_data['accounts']

    # === Produce HTML:

    def time_tuple(self, idx: int) ->tuple:
        t = self.logged[idx][0]
        dtime = t_run if t==0 else datetime.fromtimestamp(t)
        return ( dtime.hour, dtime.minute )

    def time_html(self, idx: int) ->str:
        h, m = self.time_tuple(idx)
        return TIME_FRAME % ( 'acct_edit_th_'+str(idx), str(h), SUBMIT_ON_INPUT, 'acct_edit_tm_'+str(idx), str(m), SUBMIT_ON_INPUT )

    def accounted_html(self, data_list: list, idx: int) ->str:
        return ACCOUNTED_TR_FRAME % (
            self.time_html(idx),
            'acct_edit_name_'+str(idx),
            str(self.logged[idx][1]),
            SUBMIT_ON_INPUT,
            'acct_edit_spent_'+str(idx),
            str(self.logged[idx][2]),
            SUBMIT_ON_INPUT,
            'acct_edit_received_'+str(idx),
            str(self.logged[idx][3]),
            SUBMIT_ON_INPUT,
            'acct_edit_category_'+str(idx),
            str(self.logged[idx][4]),
            SUBMIT_ON_INPUT, )

    def entryline_html(self) ->str:
        return ACCOUNTENTRY_TR_FRAME % (
            datetime.now().strftime('%H:%M'),
            SUBMIT_ON_INPUT,
            SUBMIT_ON_INPUT,
            SUBMIT_ON_INPUT,
            SUBMIT_ON_INPUT,
            SUBMIT_ON_INPUT, )

    def generate_html_body(self) ->str:
        table_str = ACCOUNTS_TABLE_HEAD
        for i in range(len(self.logged)):
            table_str += self.accounted_html(self.logged[i], i)
        table_str += self.entryline_html()
        return table_str + '</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return self.logged

    # === Member functions for data updates:

    def update_add(self, element: str, new_val: str) ->bool:
        add_list = [ time(), '', 0, 0, '', ]
        if element == 'name':
            add_list[1] = new_val
            self.logged.append(add_list)
            return True
        return False

    def update_edit(self, idx: int, element: str, new_val: str) ->bool:
        if idx > len(self.logged):
            return False
        edit_list = self.logged[idx]

        if element == 'name':
            edit_list[1] = new_val
            self.logged[idx] = edit_list
            return True

        if element == 'th':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=int(new_val), minute=m)
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'tm':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=h, minute=int(new_val))
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'spent':
            edit_list[2] = float(new_val)
            self.logged[idx] = edit_list
            return True

        if element == 'received':
            edit_list[3] = float(new_val)
            self.logged[idx] = edit_list
            return True

        if element == 'category':
            edit_list[4] = new_val
            self.logged[idx] = edit_list
            return True
        return False

class daypage_communications:
    def __init__(self, day: datetime, day_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.logged = []
        if not is_new:
            if 'comms' in self.day_data:
                self.logged = self.day_data['comms']

    # === Produce HTML:

    def time_tuple(self, idx: int) ->tuple:
        t = self.logged[idx][0]
        dtime = t_run if t==0 else datetime.fromtimestamp(t)
        return ( dtime.hour, dtime.minute )

    def time_html(self, idx: int) ->str:
        h, m = self.time_tuple(idx)
        return TIME_FRAME % ( 'comms_edit_th_'+str(idx), str(h), SUBMIT_ON_INPUT, 'comms_edit_tm_'+str(idx), str(m), SUBMIT_ON_INPUT )

    def comms_html(self, data_list: list, idx: int) ->str:
        return COMMS_TR_FRAME % (
            self.time_html(idx),
            state_html('comms_edit_name_'+str(idx), 'text', str(self.logged[idx][1])),
            state_html('comms_edit_done_'+str(idx), 'checkbox', int(self.logged[idx][2])),
            state_html('comms_edit_note_'+str(idx), 'longertext', str(self.logged[idx][3])),
            )

    def entryline_html(self) ->str:
        return COMMSENTRY_TR_FRAME % (
            datetime.now().strftime('%H:%M'),
            SUBMIT_ON_INPUT,
            SUBMIT_ON_INPUT,
            SUBMIT_ON_CHANGE,
            SUBMIT_ON_INPUT, )

    def generate_html_body(self) ->str:
        table_str = COMMS_TABLE_HEAD
        for i in range(len(self.logged)):
            table_str += self.comms_html(self.logged[i], i)
        table_str += self.entryline_html()
        return table_str + '</table>\n'

    # === Produce data dictionary:

    def get_data(self) ->list:
        return self.logged

    # === Member functions for data updates:

    def update_add(self, element: str, new_val: str) ->bool:
        add_list = [ time(), '', 0, '', ]
        if element == 'name':
            add_list[1] = new_val
            self.logged.append(add_list)
            return True
        return False

    def update_edit(self, idx: int, element: str, new_val: str) ->bool:
        if idx > len(self.logged):
            return False
        edit_list = self.logged[idx]

        if element == 'name':
            edit_list[1] = new_val
            self.logged[idx] = edit_list
            return True

        if element == 'th':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=int(new_val), minute=m)
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'tm':
            h, m = self.time_tuple(idx)
            t_edited = self.day.replace(hour=h, minute=int(new_val))
            edit_list[0] = datetime.timestamp(t_edited)
            self.logged[idx] = edit_list
            return True

        if element == 'done':
            edit_list[2] = truefalse_to_bool[new_val]
            self.logged[idx] = edit_list
            return True

        if element == 'note':
            edit_list[3] = new_val
            self.logged[idx] = edit_list
            return True

        return False

# Right side data tables objects.
class nutri_and_accounts_tables:
    def __init__(self, day: datetime, day_data: dict, multiday_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.multiday_data = multiday_data
        self.is_new = is_new

        self.frame = NUTRI_ACCOUNTS_TABLES_FRAME

        # --- Individual components of right side data tables.
        self.tables = {
            'milestones': daypage_milestones(day, self.day_data, self.multiday_data, is_new),
            'nutrition': daypage_nutritable(day, self.day_data, self.multiday_data, is_new),
            'exercise': daypage_exercise(day, self.day_data, is_new),
            'accounts': daypage_accounts(day, self.day_data, is_new),
            'comms': daypage_communications(day, self.day_data, is_new),
        }

    # === Produce HTML:

    def generate_html_head(self) ->str:
        return ''

    def generate_html_body(self) ->str:
        return self.frame % tuple([ self.tables[table].generate_html_body() for table in self.tables ])

    # === Produce data dictionary:

    def get_dict(self) ->dict:
        self.day_data = {}
        for table in self.tables:
            self.day_data[table] = self.tables[table].get_data()
        return self.day_data

class daypage_tables:
    def __init__(self, day: datetime, day_data: dict, multiday_data: dict, is_new: bool):
        self.day = day
        self.day_data = day_data
        self.multiday_data = multiday_data
        self.is_new = is_new

        self.frame = DAYPAGE_TABLES_FRAME
        # --- Left side checklist.
        self.wiztable = daypage_wiztable(day, self.day_data, is_new)
        # --- Right side data tables.
        self.nutriaccountstable = nutri_and_accounts_tables(day, self.day_data, self.multiday_data, is_new)

    # === Produce HTML:

    def generate_html_head(self) ->str:
        return DAYPAGE_WIZTABLE_STYLE

    def generate_html_body(self) ->str:
        return self.frame % (self.wiztable.generate_html_body(), self.nutriaccountstable.generate_html_body())

    # === Produce data dictionary:

    def get_dict(self) ->dict:
        self.day_data = {
            'wiztable': self.wiztable.get_data(),
        }
        nea = self.nutriaccountstable.get_dict()
        self.day_data.update(  nea )
        return self.day_data

class daypage(fz_htmlpage):
    def __init__(self, directives: dict, force_new=False):
        # === Initializing page objects (not yet generating or showing data in a specific format):
        super().__init__()
        self.day_str = directives['date']
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')
        self.is_new = force_new

        self.daywiz_data = {}
        self.load_daywiz_json()
        if 'cache' in self.daywiz_data:
            self.multiday_data = self.daywiz_data['cache']
        else:
            self.multiday_data = self.get_multiday_data()

        #gothere('daypage.__init__')
        if force_new:
            self.day_data = {}
        else:
            self.day_data = self.get_day_data(self.day_str)
            self.is_new = (len(self.day_data) == 0)

        # --- Individual components of the page.
        #     Components with main actions in page style and standard presentation:
        self.html_std = fz_html_standard('daywiz.py')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('DayWiz')

        #     Components with main actions in body content:
        self.date_picker = fz_html_datepicker(self.day)
        self.tables = daypage_tables(self.day, self.day_data, self.multiday_data, self.is_new)

        # TODO: *** Remove the following line and all references to self.debug once no longer needed.
        self.debug = debug_test(directives, form)

        # --- Components that have action-calls in head, body and tail groups.
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.tables, self.html_title, self.debug, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, self.date_picker, self.tables, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

    # def legacy_load_daywiz_json(self):
    #   if exists(JSON_DATA_PATH):
    #       try:
    #           with open(JSON_DATA_PATH, 'r') as f:
    #               self.daywiz_data = json.load(f)
    #       except:
    #           self.daywiz_data = {}

    # Store JSON in format: {'YYYY.mm.dd': { 'wiztable': ..., 'nutrition': ..., 'exercise': ..., 'accounts': ...}, etc.}
    # def legacy_save_daywiz_json(self) ->bool:
    #   global global_debug_str
    #   self.daywiz_data[self.day_str] = self.tables.get_dict()
    #   try:
    #       with open(JSON_DATA_PATH, 'w') as f:
    #           json.dump(self.daywiz_data, f)
    #       return True
    #   except Exception as e:
    #       global_debug_str += '<p><b>'+str(e)+'</b></p>'
    #       return False

    # The following can be used to convert from legacy format to new format:
    # def convert_save_daywiz_json(self) ->bool:
    #   global global_debug_str
    #   self.daywiz_data[self.day_str] = self.tables.get_dict()
    #   self.new_daywiz_data = {
    #       'wiztable': {},
    #       'nutrition': {},
    #       'exercise': {},
    #       'accounts': {},
    #   }
    #   for daystr in self.daywiz_data:
    #       daydata = self.daywiz_data[daystr]
    #       if 'wiztable' in daydata:
    #           self.new_daywiz_data['wiztable'][daystr] = daydata['wiztable']
    #       if 'nutrition' in daydata:
    #           self.new_daywiz_data['nutrition'][daystr] = daydata['nutrition']
    #       if 'exercise' in daydata:
    #           self.new_daywiz_data['exercise'][daystr] = daydata['exercise']
    #       if 'accounts' in daydata:
    #           self.new_daywiz_data['accounts'][daystr] = daydata['accounts']
    #   try:
    #       with open(NEW_JSON_DATA_PATH, 'w') as f:
    #           json.dump(self.new_daywiz_data, f)
    #       return True
    #   except Exception as e:
    #       global_debug_str += '<p><b>'+str(e)+'</b></p>'
    #       return False

    # def legacy_get_day_data(self, daystr: str) ->dict:
    #   if daystr in self.daywiz_data:
    #       return self.daywiz_data[daystr]
    #   else:
    #       return {}

    # === Member functions for data dictionary storage and retrieval:

    def load_daywiz_json(self):
        if exists(JSON_DATA_PATH):
            try:
                with open(JSON_DATA_PATH, 'r') as f:
                    self.daywiz_data = json.load(f)
            except:
                self.daywiz_data = {}

    # Store JSON in format: {'wiztable': {...days...}, 'nutrition': {...days...}, 'exercise': {...days...}, 'accounts': {...days...}}
    def save_daywiz_json(self, dryrun=False) ->bool:
        global global_debug_str
        # Replace data for this day with updated data:
        day_dict = self.tables.get_dict()
        for tablekey in data_tables:
            if tablekey in day_dict:
                if tablekey in self.daywiz_data:
                    self.daywiz_data[tablekey][self.day_str] = day_dict[tablekey]
                else: # First save to a newly added data table.
                    self.daywiz_data[tablekey] = { self.day_str: day_dict[tablekey], }
        # Replace multiday cache with updated cached data:
        self.daywiz_data['cache'] = self.multiday_data
        try:
            if dryrun:
                with open('/dev/shm/daywiz_update_dryrun.json', 'w') as f:
                    json.dump(self.daywiz_data, f)
            else:
                with open(JSON_DATA_PATH, 'w') as f:
                    json.dump(self.daywiz_data, f)
            return True
        except Exception as e:
            global_debug_str += '<p><b>'+str(e)+'</b></p>'
            return False

    # === Member functions for data parsing:

    def get_day_data(self, daystr: str) ->dict:
        day_dict = {}
        for tablekey in data_tables:
            if tablekey in self.daywiz_data: # Deal gracefully with the appearance of newly added data tables.
                if daystr in self.daywiz_data[tablekey]:
                    day_dict[tablekey] = self.daywiz_data[tablekey][daystr]
        return day_dict

    # Get the label-specific data for the day in datestr.
    def get_wiz_data(self, datestr: str, label: str, from_end=True) ->list:
        try:
            wiz_list = self.daywiz_data['wiztable'][datestr]
            if from_end:
                for data in reversed(wiz_list):
                    if data[0] == label:
                        return data
                return None
            for data in wiz_list:
                if data[0] == label:
                    return data
                return None
        except:
            return None

    # The colorie total is determined over the range of days for which such
    # data is known.
    # Days within that range where the `daynutri` checkbox is not checked are counted
    # as 2300 calories to encourage tracking and remove not tracking as a way to avoid
    # detection of undesired behavior.
    # An excess of the total subtracts from the normal threshold.
    def get_calorie_threshold(self) ->dict:
        nutri = self.daywiz_data['nutrition']
        nutridays = list(nutri.keys())
        nutri_multiday = {
            'caloriethreshold': WEIGHTLOSS_TARGET_CALORIES,
            'caloriestotal': 0,
            'dayscaloriesunknown': 0,
        }
        if len(nutridays) <= 0:
            return nutri_multiday
        # Find the earliest in the data:
        nutridays.sort()
        start_idx = 0
        nextdaystr = nutridays[start_idx]
        # Skip empties at the start:
        while len(nutri[nextdaystr]) < 1:
            start_idx += 1
            nextdaystr = nutridays[start_idx]
        # From the first non-empty, check every day until the current day:
        nextday = datestr2datetime(nextdaystr)
        numdays = 0
        total_calories = 0
        unknown = 0
        while nextdaystr != self.day_str:
            nutri_data = self.get_wiz_data(nextdaystr, 'daynutri')
            if nutri_data is None:
                #print('===========> no daynutri: '+nextdaystr)
                unknown += 1
                total_calories += UNKNOWN_DAY_CALORIES_ASSUMED
            else:
                if nutri_data[2] != 'checked':
                    unknown += 1
                    #print('==============> daynutri unchecked: '+nextdaystr)
                    total_calories += UNKNOWN_DAY_CALORIES_ASSUMED
                else:
                    if nextdaystr not in nutri:
                        unknown += 1
                        #print('==============> date missing in nutri: '+nextdaystr)
                        total_calories += UNKNOWN_DAY_CALORIES_ASSUMED
                    else:
                        nextday_consumption = nutri[nextdaystr]
                        if len(nextday_consumption) < 1:
                            unknown += 1
                            #print('==============> consumption untracked: '+nextdaystr)
                            total_calories += UNKNOWN_DAY_CALORIES_ASSUMED
                        else:
                            total_calories += day_calories(nextday_consumption)
            numdays += 1
            nextday = nextday + timedelta(days=1)
            nextdaystr = datetime2datestr(nextday)
        # Calculate possible discrepancy:
        nutri_multiday['caloriestotal'] = total_calories
        nutri_multiday['dayscaloriesunknown'] = unknown
        intended_calories = numdays*WEIGHTLOSS_TARGET_CALORIES
        cal_over = max(0, total_calories - intended_calories)
        threshold = WEIGHTLOSS_TARGET_CALORIES - cal_over
        nutri_multiday['caloriethreshold'] = max(MIN_CALORIES_THRESHOLD, threshold)
        return nutri_multiday

    # This returns data that is derived from information about multiple days, e.g.
    # calorie threshold derived from the sum over the previous days.
    # The data may be retrieved from storage or calculated on the spot.
    def get_multiday_data(self) ->dict:
        multiday_data = self.get_calorie_threshold()
        return multiday_data

    # === Member functions for data updates:

    def _update_nutri_multiday(self):
        self.multiday_data.update( self.get_calorie_threshold() )

    def _update_wiz(self, update_target: list, update_val: str) ->bool:
        if self.tables.wiztable.update(update_target[1], update_target[2], update_val):
            return self.save_daywiz_json()
        return False

    def _update_nutri_add(self, update_target: list, update_val: str) ->bool:
        if self.tables.nutriaccountstable.tables['nutrition'].update_add(update_target[2], update_val):
            self._update_nutri_multiday()
            return self.save_daywiz_json()
        return False

    def _update_nutri_edit(self, update_target: list, update_val: str) ->bool:
        if self.tables.nutriaccountstable.tables['nutrition'].update_edit(int(update_target[3]), update_target[2], update_val):
            self._update_nutri_multiday()
            return self.save_daywiz_json()
        return False

    # update_target = [ nutri, addto, <name> ]
    def _update_nutri_addto(self, update_target: list, update_val: str, dryrun=False) ->bool:
        if self.tables.nutriaccountstable.tables['nutrition'].update_addto(update_target[2], update_val):
            self._update_nutri_multiday()
            return self.save_daywiz_json(dryrun=dryrun)
        return False

    # update_target = [ nutri, add/edit, name/th/tm/quantity, idx, ] or [ nutri, addto, <name> ]
    def _update_nutri(self, update_target: list, update_val: str, dryrun=False) ->bool:
        if update_target[1] == 'add': # NOTE: This add is really "append".
            return self._update_nutri_add(update_target, update_val)

        if update_target[1] == 'edit': # NOTE: This means to change the value to update_val.
            return self._update_nutri_edit(update_target, update_val)

        if update_target[1] == 'addto': # NOTE: This adds to value or appends a new entry as needed.
            return self._update_nutri_addto(update_target, update_val, dryrun=dryrun)

        return False

    # def _update_exerc_add(self, update_target: list, update_val: str) ->bool:
    #     if self.tables.nutriaccountstable.tables['exercise'].update_add(update_target[2], update_val):
    #         return self.save_daywiz_json()
    #     return False

    # def _update_acct_add(self, update_target: list, update_val: str) ->bool:
    #     if self.tables.nutriaccountstable.tables['accounts'].update_add(update_target[2], update_val):
    #         return self.save_daywiz_json()
    #     return False

    # def _update_exerc_edit(self, update_target: list, update_val: str) ->bool:
    #     if self.tables.nutriaccountstable.tables['exercise'].update_edit(int(update_target[3]), update_target[2], update_val):
    #         return self.save_daywiz_json()
    #     return False

    # def _update_acct_edit(self, update_target: list, update_val: str) ->bool:
    #     if self.tables.nutriaccountstable.tables['accounts'].update_edit(int(update_target[3]), update_target[2], update_val):
    #         return self.save_daywiz_json()
    #     return False

    # def _update_exerc(self, update_target: list, update_val: str) ->bool:
    #     if update_target[1] == 'add':
    #         return self._update_exerc_add(update_target, update_val)

    #     if update_target[1] == 'edit':
    #         return self._update_exerc_edit(update_target, update_val)

    #     return False

    # def _update_acct(self, update_target: list, update_val: str) ->bool:
    #     if update_target[1] == 'add':
    #         return self._update_acct_add(update_target, update_val)

    #     if update_target[1] == 'edit':
    #         return self._update_acct_edit(update_target, update_val)

    #     return False

    def _update_stddata_add(self, datatable: str, update_target: list, update_val: str) ->bool:
        if self.tables.nutriaccountstable.tables[datatable].update_add(update_target[2], update_val):
            return self.save_daywiz_json()
        return False

    def _update_stddata_edit(self, datatable: str, update_target: list, update_val: str) ->bool:
        if self.tables.nutriaccountstable.tables[datatable].update_edit(int(update_target[3]), update_target[2], update_val):
            return self.save_daywiz_json()
        return False

    def _update_stddata(self, datatable: str, update_target: list, update_val: str) ->bool:
        if update_target[1] == 'add':
            return self._update_stddata_add(datatable, update_target, update_val)

        if update_target[1] == 'edit':
            return self._update_stddata_edit(datatable, update_target, update_val)

        return False

    # To use this method or the methods it calls as an entry point for
    # updating, take note of the list of information needed for the
    # update_target, in addition to the update_val.
    def _update(self, update_id: str, update_val: str) ->bool:
        update_target = update_id.split('_')

        if update_target[0] == 'wiz': # [ wiz, element, line_id, ]
            return self._update_wiz(update_target, update_val)

        if update_target[0] == 'nutri': # [ nutri, add/edit, name/th/tm/quantity, idx, ] or [ nutri, addto, <name> ]
            return self._update_nutri(update_target, update_val)

        if update_target[0] == 'exerc': # [ exerc, add/edit, name/th/tm/quantity, idx, ]
            return self._update_stddata('exercise', update_target, update_val)

        if update_target[0] == 'acct': # [ acct, add/edit, name/th/tm/category, idx, ]
            return self._update_stddata('accounts', update_target, update_val)

        if update_target[0] == 'comms': # [ comms, add/edit, name/th/tm/done/note, idx, ]
            return self._update_stddata('comms', update_target, update_val)

        return False

    def update_from_form(self, formfields: cgi.FieldStorage) ->bool:
        try:
            return self._update(formfields.getvalue('par_changed'), formfields.getvalue('par_newval'))
        except Exception as e:
            self.body_list.pop()
            self.body_list.append( fz_errorpage('Update caused exception: '+str(e)+'\n<pre>'+traceback.format_exc()+'</pre>\n') )
            return False

    def update_from_list(self, args: list) ->bool:
        return self._update(args[0], args[1])

    # === Member functions for use by other scripts (e.g. consumed.py, see add_nutrition_to_log below):

    def add_nutrition_to_multidaylog(self, name:str, quantity:float, hour:int, minute:int, dryrun=False) ->str:
        dtime = self.day.replace(hour=hour, minute=minute)
        t = datetime.timestamp(dtime)
        # Let's reuse the existing nutri edit and nutri add methods.
        # - make the call data list
        # - call the update function (with dryrun setting)
        # - make sure the time is updated as well
        update_target = [ 'nutri', 'addto', name ]
        res = self._update_nutri(update_target, str(quantity), dryrun=dryrun)
        return self.generate_html()
        # 1. Is this already in the specified day's nutrition log?

        # idx = len(self.logged)
        # for i in range(len(self.logged)):
        #     if name == self.logged[i][1]:
        #         idx = i
        # if idx < len(self.logged):
        #     # Update existing entry:
        #     edit_list = self.logged[idx]
        #     edit_list[0] = t
        #     edit_list[2] += quantity
        #     self.logged[idx] = edit_list
        # else:
        #     # Add entry:
        #     add_list = [ t, name, quantity ]
        #     self.logged[idx] = add_list
        # self._update_nutri_multiday()
        # self.save_daywiz_json()
        # return self.generate_html()

    # === Member function for DayWiz page HTML generation:

    def show(self):
        #print("Content-type:text/html\n\n")
        print(self.generate_html())
        #self.convert_save_daywiz_json() # To convert the JSON data from legacy format to new format.

# ====================== End of class definitions.

# ====================== Entry parsers:

def get_directives(formfields: cgi.FieldStorage) ->dict:
    directives = {
        'cmd': formfields.getvalue('cmd'),
        'date': formfields.getvalue('date'),
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
    return directives

def launch_as_cgi():
    # if form.getvalue('date'):
    #   global got_here_active
    #   got_here_active = True
    directives = get_directives(form)
    directives = check_directives(directives)
    _page = daypage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_form(form)
        _page.show()
    else:
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)
    _page = daypage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_list(directives['args'])
        _page.show()
    else:
        _page.show()

# Call this from other scripts, e.g. consumed.py.
# This does not produce daywiz page output, but it can use the information.
def add_nutrition_to_log(datestr:str, name:str, quantity:float, hour:int, minute:int, dryrun=False)->str:
    directives = { 'date': datestr }
    _page = daypage(directives)
    htmlstr = _page.add_nutrition_to_multidaylog(name, quantity, hour, minute, dryrun=dryrun)
    return htmlstr


if __name__ == '__main__':
    from sys import argv
    # Put this here to catch errors.
    # This should only be printed if this is not being imported as a module.
    print("Content-type:text/html\n\n")
    if len(argv) > 2:
        if argv[2] == 'date':
            launch(directives={ 'cmd': argv[1], 'date': argv[2], 'args': argv[2:], })
        else:
            launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'args': argv[2:], })
    else:
        launch_as_cgi()
