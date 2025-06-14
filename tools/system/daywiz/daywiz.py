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
from subprocess import Popen, PIPE
from io import StringIO

from wiztable import *
from fzhtmlpage import *
from fznutrition import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

webdata_path = "/var/www/webdata/formalizer"

debugdatabase = webdata_path+'/daywiz_database.debug'

error_file = webdata_path + '/daywiz_error.log'

# ====================== Data store:

# TODO: *** Switch to using the database.
#JSON_DATA_PATH='/home/randalk/.formalizer/.daywiz_data.json' # Permission issues.
#JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'
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
    'climbing': [ 'minutes', ],
    'balance board': [ 'minutes', ],
    'leg elastic': [ 'minutes', ],
    'snow shoveling': [ 'minutes', ],
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
        total_score_dict[daystr] = round(10.0*score/max_possible)
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

DAYPAGE_WIZTABLE_STYLE='''<style>
th {
font-weight: 700;
}
.secondcolfixedw td:nth-child(2) {
  width: 8em;
}
table.secondcolfixedw tr:hover td {
background-color: var(--tr-hover);
}
</style>
'''
DAYPAGE_TABLES_FRAME='''<table class="col_right_separated">
<tr><th>Wizard Record</th><th>Additional Day Records</th></tr>
<tr valign="top">
<td width="50%%">%s</td>
<td>%s</td>
</tr>
</table>
'''

DAYPAGE_HTML_TOP_EXTRA='''<button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?startfrom=%s0000&daysinterval=1','_blank');">Visit corresponding Log interval</button>
'''

WIZTABLE_TOP='''<table class="secondcolfixedw">
<tr><th>t recommended</th><th>t actual</th><th>description</th><th>state</th><th>extra</th></tr>
'''
WIZTABLE_SUMMARY='''<tr><td></td><td></td><td><a href="/cgi-bin/score.py?cmd=show&selectors=wiztable">score</a>: %s/%s</td><td>%s/%s</td><td></td></tr>
'''

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

EXERCISE_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th><span class="tooltip">exercised<span class="tooltiptext">%s</span></span></th><th>quantity</th><th>units</th></tr>\n'
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

def make_tooltip(two_column_table:dict)->str:
    tooltip_str = '<table><tbody>'
    for key, val in two_column_table.items():
        tooltip_str += '<tr><td>%s</td><td>%s</td></tr>' % (key, val)
    tooltip_str += '</tbody></table>'
    return tooltip_str

# ====================== Classes that manage content in page areas:

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

    def generate_html_body(self) ->str:
        table_str = WIZTABLE_TOP
        self.checkbox_metrics = [ 0, 0 ]
        self.number_metrics = [0, 0 ]
        self.score = 0.0
        self.score_possible = 0.0
        for wizline in self.lines:
            table_str += wizline.generate_html_tr()
            addpossible, addscore = wizline.add_to_checkbox_metrics(self.checkbox_metrics)
            self.score_possible += addpossible
            self.score += addscore
            addpossible, addscore = wizline.add_to_number_metrics(self.number_metrics)
            self.score_possible += addpossible
            self.score += addscore
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
        table_str = EXERCISE_TABLE_HEAD % make_tooltip(exercises)
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
        self.html_tooltip = fz_html_tooltip()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('DayWiz')
        self.html_top_extra = fz_html_verbatim(DAYPAGE_HTML_TOP_EXTRA % self.day.strftime("%Y%m%d"))

        #     Components with main actions in body content:
        self.date_picker = fz_html_datepicker(self.day)
        self.tables = daypage_tables(self.day, self.day_data, self.multiday_data, self.is_new)

        # TODO: *** Remove the following line and all references to self.debug once no longer needed.
        self.debug = debug_test(directives, form)

        # --- Components that have action-calls in head, body and tail groups.
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_tooltip, self.html_clock, self.tables, self.html_title, self.debug, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, self.date_picker, self.html_top_extra, self.tables, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

    # === Member functions for data dictionary storage and retrieval:

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

    def get_day_data_from_database(self, daydate:str)->dict:
        thecmd = "./fzmetricspq -q -d formalizer -s randalk -E STDOUT -R -i "+daydate+" -F json -o STDOUT -w true -n true -e true -a true -m true -c true"
        datastr = self.database_call(thecmd)
        try:
            data = json.loads(datastr)
            if not isinstance(data, dict) or len(data)==0:
                return {}
            else:
                return data
        except:
            return {}

    def load_daywiz_json(self):
        daytimestamp = self.day_str[0:4]+self.day_str[5:7]+self.day_str[8:10]+'0000'
        data = self.get_day_data_from_database(daytimestamp)
        if len(data)==0:
            self.daywiz_data = {}
        else:
            for tablekey in data:
                self.daywiz_data[tablekey] = { self.day_str: data[tablekey] }

    # *** WHEN WE START USING THE DATABASE INSTEAD OF A FILE:
    # We'd like to move away from loading the whole thing into self.daywiz_data.
    # Instead, find where self.daywiz_data is referenced and try to replace those
    # references with calls that are day-specific and that try to either
    # a) Pull the data from the cache.
    # b) Make a database call that retrieves the data for the specified day.
    # That can still be put into the cache as well.

    def save_day_data_to_database(self):
        daystosave = list(self.daywiz_data['wiztable'].keys())
        for daytosave in daystosave:
            daytimestamp = daytosave[0:4]+daytosave[5:7]+daytosave[8:10]+'0000'
            # Put data into temporary files and build call arguments
            addargs = ''
            for tablekey in self.daywiz_data:
                if daytosave in self.daywiz_data[tablekey]:
                    jsonfilename = webdata_path+'/'+tablekey+'.json'
                    addargs += ' -'+tablekey[0]+' file:'+jsonfilename
                    with open(jsonfilename, 'w') as f:
                        json.dump(self.daywiz_data[tablekey][daytosave] ,f)
            # Call database write to multiple tables
            thecmd = "./fzmetricspq -q -d formalizer -s randalk -E STDOUT -S -i "+daytimestamp+addargs
            self.database_call(thecmd)

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
        #self.daywiz_data['cache'] = self.multiday_data
        try:
            if dryrun:
                with open('/dev/shm/daywiz_update_dryrun.json', 'w') as f:
                    json.dump(self.daywiz_data, f)
            else:
                self.save_day_data_to_database()
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
        nutri_multiday = {
            'caloriethreshold': WEIGHTLOSS_TARGET_CALORIES,
            'caloriestotal': 0,
            'dayscaloriesunknown': 0,
        }
        if 'nutrition' in self.daywiz_data:
            nutri = self.daywiz_data['nutrition']
        else:
            nutri = {}
        nutridays = list(nutri.keys())

        if len(nutridays) <= 0:
            return nutri_multiday

        # Find the earliest in the data:
        nutridays.sort()
        start_idx = 0
        nextdaystr = nutridays[start_idx]
        # Skip empties at the start:
        while len(nutri[nextdaystr]) < 1:
            start_idx += 1
            if start_idx >= len(nutridays):
                return nutri_multiday
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
    # with open(debugdatabase, 'w') as f:
    #     f.write('DEBUG START\n')
    if len(argv) > 2:
        if argv[2] == 'date':
            launch(directives={ 'cmd': argv[1], 'date': argv[2], 'args': argv[2:], })
        else:
            launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'args': argv[2:], })
    else:
        launch_as_cgi()
