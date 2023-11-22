#!/usr/bin/env python3
# nutrition.py
# Copyright 2022 Randal A. Koene
# License TBD
#
# Nutrition page generator, parser.
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
from fznutrition import *

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

print("Content-type:text/html\n\n")

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

# ====================== String templates used to generate content for page areas:

NUTRITION_TABLES_STYLE='''
'''
NUTRITION_TABLES_FRAME = '''<table class="col_right_separated">
<tr><th>Low Calorie Nutritious</th><th>Low Calorie Snack</th></tr>
<tr valign="top">
<td>%s</td>
<td>%s</td>
</tr>
</table>
'''

LOWCALNUTRITIOUS_FRAME = '''<table>
%s
</table>
'''
LOWCALNUTRITIOUS_LINE = '<tr><td>%s</td></tr>\n'

LOWCALSNACK_FRAME = '''<table>
%s
</table>
'''
LOWCALSNACK_LINE = '<tr><td>%s</td></tr>\n'

# ====================== Classes that manage content in page areas:

class nutrition_lowcalnutritious(fz_html_multilinetable):
    def __init__(self, day: datetime):
        super().__init__('', LOWCALNUTRITIOUS_FRAME, LOWCALNUTRITIOUS_LINE, self.generate_data_list)
        self.day = day

    def generate_data_list(self) ->list:
        return lowcal_filling_nutritious

class nutrition_lowcalsnack(fz_html_multilinetable):
    def __init__(self, day: datetime):
        super().__init__('', LOWCALSNACK_FRAME, LOWCALSNACK_LINE, self.generate_data_list)
        self.day = day

    def generate_data_list(self) ->list:
        return lowcal_snack

class nutrition_tables(fz_html_multicomponent):
    def __init__(self, day: datetime):
        super().__init__(NUTRITION_TABLES_STYLE, NUTRITION_TABLES_FRAME)
        self.day = day
        self.components = {
            'lowcalnutritious': nutrition_lowcalnutritious(day),
            'lowcalsnack': nutrition_lowcalsnack(day),
        }

class nutritionpage(fz_htmlpage):
    def __init__(self, directives: dict):
        super().__init__()
        self.day_str = directives['date']
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

        # Prepare HTML output template:
        self.html_std = fz_html_standard('nutrition.py')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('Nutrition')

        self.tables = nutrition_tables(self.day)

        # Prepare head, body and tail HTML generators:
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.html_title, self.tables, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, self.tables, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

    def show(self):
        #print("Content-type:text/html\n\n")
        print(self.generate_html())

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
    _page = nutritionpage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_form(form)
        _page.show()
    else:
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)
    _page = nutritionpage(directives)

    if directives['cmd'] == 'update':
        _page.update_from_list(directives['args'])
        _page.show()
    else:
        _page.show()

if __name__ == '__main__':
    from sys import argv
    if len(argv) > 2:
        if argv[2] == 'date':
            launch(directives={ 'cmd': argv[1], 'date': argv[2], 'args': argv[2:], })
        else:
            launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'args': argv[2:], })
    else:
        launch_as_cgi()
