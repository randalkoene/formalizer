#!/usr/bin/env python3
# consumed.py
# Copyright 2022 Randal A. Koene
# License TBD
#
# Convenient page through which to pick nutrition consumed.
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
from daywiz import add_nutrition_to_log

# Create instance of FieldStorage 
# NOTE: Only parameters that have a "name" (not just an "id") are submitted.
#       We do not always supply a name, because it is faster if we only receive
#       the par_changed and par_newval values to parse.
form = cgi.FieldStorage()
#cgitb.enable()

print("Content-type:text/html\n\n")

# *** DEBUG:
thekeys=list(form.keys())
with open('/dev/shm/formkeys','w') as f:
    f.write(str(thekeys))

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

JSON_DATA_PATH='/var/www/webdata/formalizer/.daywiz_data.json'
WEB_WRITABLE_DIR='/var/www/webdata/formalizer/'

# ====================== String templates used to generate content for page areas:

NUTRITION_TABLES_STYLE='''
'''
NUTRITION_TABLES_FRAME_BEGIN = '''<table class="col_right_separated">
<tr>%s</tr>
<tr valign="top">
'''
NUTRITION_TABLES_FRAME_END = '''</tr>
</table>
'''

BUTTONCOMMON='''<tr><td><button type="button" class="button button2" onclick="window.open('/cgi-bin/consumed.py?cmd=sel'''
# Testing:
BUTTONOTHER='''<button type="button" class="button button2" onclick="window.open('/cgi-bin/consumed.py?cmd=sel'''

# LOWCALNUTRITIOUS_FRAME = '''<table>
# %s
# </table>
# '''
#LOWCALNUTRITIOUS_LINE = '''&sel=%s','_self');">%s</button></td></tr>\n'''

# LOWCALSNACK_FRAME = '''<table>
# %s
# </table>
# '''
#LOWCALSNACK_LINE = '''&sel=%s','_self');">%s</button></td></tr>\n'''

# FRUIT_FRAME = '''<table>
# %s
# </table>
# '''
#FRUIT_LINE = '''&sel=%s','_self');">%s</button></td></tr>\n'''

EMPTY_TABLE_FRAME = '''<table>
%s
</table>
'''
COMMON_CELL_ROW_END = '''&sel=%s','_self');">%s</button></td></tr>\n'''

# OTHER_FRAME = '''<table>
# %s
# </table>
# '''
# OTHER_LINE = '''&sel=%s','_self');">%s</button></td></tr>\n'''
OTHER_FRAME = '''<table><tr></td>
%s
</td></tr></table>
'''
OTHER_LINE = '''&sel=%s','_self');">%s</button>\n'''

grouped = set([])
for group_key, group_data in nutrition_groups.items():
    grouped.update(set(group_data[1]))
all_nutrition_keys = list(nutrition.keys())
other = list(set(all_nutrition_keys).difference(grouped))

QUANTITY_INPUT_TEMPLATE = '''
%s
'''

# ====================== Classes that manage content in page areas:

# class consumed_lowcalnutritious(fz_html_multilinetable):
#     def __init__(self, day: datetime, line_template:str):
#         super().__init__('', EMPTY_TABLE_FRAME, line_template, self.generate_data_list)
#         self.day = day

#     def generate_data_list(self) ->list:
#         sortedkeys = sorted(lowcal_filling_nutritious)
#         return zip(sortedkeys, sortedkeys)

# class consumed_lowcalsnack(fz_html_multilinetable):
#     def __init__(self, day: datetime, line_template:str):
#         super().__init__('', EMPTY_TABLE_FRAME, line_template, self.generate_data_list)
#         self.day = day

#     def generate_data_list(self) ->list:
#         sortedkeys = sorted(lowcal_snack)
#         # Duplicate each in order to have data for inside and outside button.
#         return zip(sortedkeys, sortedkeys)

# class consumed_fruit(fz_html_multilinetable):
#     def __init__(self, day: datetime, line_template:str):
#         super().__init__('', EMPTY_TABLE_FRAME, line_template, self.generate_data_list)
#         self.day = day

#     def generate_data_list(self) ->list:
#         sortedkeys = sorted(fruit)
#         # Duplicate each in order to have data for inside and outside button.
#         return zip(sortedkeys, sortedkeys)

class consumed_nutrition_group(fz_html_multilinetable):
    def __init__(self, day: datetime, line_template:str, group_list:list):
        super().__init__('', EMPTY_TABLE_FRAME, line_template, self.generate_data_list)
        self.day = day
        self.group_list = group_list

    def generate_data_list(self) ->list:
        sortedkeys = sorted(self.group_list)
        # Duplicate each in order to have data for inside and outside button.
        return zip(sortedkeys, sortedkeys)

class consumed_other(fz_html_multilinetable):
    def __init__(self, day: datetime, line_template:str):
        super().__init__('', OTHER_FRAME, line_template, self.generate_data_list)
        self.day = day

    def generate_data_list(self) ->list:
        sortedkeys = sorted(other)
        return zip(sortedkeys, sortedkeys)

class consumed_tables(fz_html_multicomponent):
    def __init__(self,
        day: datetime,
        button_common:str,
        button_other:str,
        horizontal_cells:fz_html_horizontal_component_cells,
        ):
        super().__init__(NUTRITION_TABLES_STYLE, horizontal_cells.get_tables_frame())
        self.day = day
        self.components = horizontal_cells.get_components()

class consumedpage(fz_htmlpage):
    def __init__(self, directives: dict):
        super().__init__()
        self.day_str = directives['date']
        button_common = BUTTONCOMMON+'&date='+self.day_str
        # Testing:
        button_other = BUTTONOTHER+'&date='+self.day_str
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

        # Prepare HTML output template:
        self.html_std = fz_html_standard('consumed.py')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('Consumed on '+self.day.strftime('%A %Y.%m.%d'))

        group_builders = {}
        for nutri_keys, nutri_data in nutrition_groups.items():
            group_builders[nutri_data[0]] = consumed_nutrition_group(self.day, button_common+COMMON_CELL_ROW_END, nutri_data[1])
        group_builders['other'] = consumed_other(self.day, button_other+OTHER_LINE)
        self.tables = consumed_tables(
            self.day,
            button_common,
            button_other,
            fz_html_horizontal_component_cells(
                NUTRITION_TABLES_FRAME_BEGIN,
                NUTRITION_TABLES_FRAME_END,
                list(nutrition_groups.keys())+['Other'],
                group_builders,),
            )

        # Prepare head, body and tail HTML generators:
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.html_title, self.tables, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, self.tables, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

    def show(self):
        #print("Content-type:text/html\n\n") # Done above to ensure output even if there is an error.
        print(self.generate_html())

class enter_quantity_of_selected:
    def __init__(self, day: datetime, selected:str):
        self.day = day
        self.selected = selected
        # This uses the cmd=update form output produced with SUBMIT_ON_INPUT in fz_html_standard.
        self.hidden_directives='''
            <input type="hidden" name="date" value="%s">
            <input type="hidden" name="sel" value="%s">
            ''' % (day.strftime('%Y.%m.%d'), selected)

    def generate_html_head(self)->str:
        return ''

    def generate_html_body(self)->str:
        if self.selected in nutrition:
            calories, units = nutrition[self.selected]
            return self.hidden_directives+'<input %s id="quantity" name="quantity" type="number" value="0"> %s (%s calories per %s)' % (SUBMIT_ON_INPUT, units, calories, units)
            #return self.hidden_directives+'<input %s id="quantity" name="quantity" type="number" value="0"> %s (%s calories per %s)' % (TEST_ON_INPUT, units, calories, units)
        else:
            return 'Not found in nutrition table: '+str(self.selected)

class quantity_consumed(fz_html_multicomponent):
    def __init__(self, day: datetime, selected:str):
        super().__init__('', QUANTITY_INPUT_TEMPLATE)
        self.day = day
        self.selected = selected
        self.components = {
            'enter_quantity': enter_quantity_of_selected(day, selected),
        }

class consumed_selected_page(fz_htmlpage):
    def __init__(self, directives: dict):
        self.day_str = directives['date']
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

        # Prepare HTML output template:
        self.html_std = fz_html_standard('consumed.py', method='get')
        self.html_icon = fz_html_icon()
        self.html_style = fz_html_style(['fz', ])
        self.html_uistate = fz_html_uistate()
        self.html_clock = fz_html_clock()
        self.html_title = fz_html_title('Consumed on '+self.day.strftime('%A %Y.%m.%d'))

        self.enter_quantity = quantity_consumed(self.day, directives['sel'])

        # Prepare head, body and tail HTML generators:
        self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.html_title, self.enter_quantity, ]
        self.body_list = [ self.html_std, self.html_title, self.html_clock, self.enter_quantity, ]
        self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

    def show(self):
        print(self.generate_html())

class consumed_quantity_page:
    def __init__(self, directives: dict):
        self.day_str = directives['date']
        try:
            self.selected = directives['sel']
        except:
            self.selected = '(NO SELECTED)'
        try:
            self.quantity = directives['quantity']
        except:
            self.quantity = '(NO QUANTITY)'
        self.day = datetime.strptime(self.day_str, '%Y.%m.%d')

    def show(self):
        if self.selected is None:
            with open('/dev/shm/add_nutri.log','w') as f:
                f.write('step 1\n')
            return
        name = str(self.selected)
        if name=='':
            with open('/dev/shm/add_nutri.log','w') as f:
                f.write('step 1\n')
            return
        try:
            quantity = float(self.quantity)
        except:
            with open('/dev/shm/add_nutri.log','w') as f:
                f.write('step 1\n')
            return
        hr_min = datetime.now().strftime('%H:%M').split(':')
        with open('/dev/shm/add_nutri.log','w') as f:
            f.write('%s %s %s %s:%s\n' % (self.day_str, name, str(quantity), str(hr_min[0]), str(hr_min[1])))
        print(add_nutrition_to_log(self.day_str, name, quantity, hour=int(hr_min[0]), minute=int(hr_min[1]))) #, dryrun=True))

# ====================== Entry parsers:

# Explicitly convert form fields to be identified into directives dictionary.
def get_directives(formfields: cgi.FieldStorage) ->dict:
    directives = {
        'cmd': formfields.getvalue('cmd'),
        'date': formfields.getvalue('date'),
        'sel': formfields.getvalue('sel'),
        'quantity': formfields.getvalue('quantity'),
        'form': formfields,
    }
    return directives

# Convert command line arguments to directives dictionary.
def parse_command_line(args: list)->dict:
    directives = {}
    for arg in args:
        if arg[0:4]=='cmd=':
            directives['cmd'] = arg[4:]
        elif arg[0:5]=='date=':
            directives['date'] = arg[5:]
        elif arg[0:4]=='sel=':
            directives['sel'] = arg[4:]
        elif arg[0:9]=='quantity=':
            directives['quantity'] = arg[9:]
    return directives

# Modify directives here to ensure valid values.
def check_directives(directives: dict) ->dict:
    # Ensure valid cmd:
    if not directives['cmd']:
        directives['cmd'] = 'show'
    # Ensure valid date:
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

    # See form input received in /dev/shm/form.log
    with open('/dev/shm/form.log', 'a+') as f:
        f.write(t_run.strftime('%Y%m%d%H%M')+':'+str(form.keys())+'\n')
        for key in form.keys():
            f.write('%s = %s\n' % (str(key), str(form.getvalue(key))))

    if directives['cmd'] == 'update':
        _page = consumed_quantity_page(directives)
        _page.show()
    elif directives['cmd'] == 'sel':
        _page = consumed_selected_page(directives)
        _page.show()
    else:
        _page = consumedpage(directives)
        _page.show()

def launch(directives: dict):
    directives = check_directives(directives)

    if directives['cmd'] == 'update':
        _page = consumed_quantity_page(directives)
        _page.show()
    elif directives['cmd'] == 'sel':
        _page = consumed_selected_page(directives)
        _page.show()
    else:
        _page = consumedpage(directives)
        _page.show()

# All command line arguments are forwarded through the directives dictionary.
if __name__ == '__main__':
    from sys import argv
    if len(argv) > 2:
        launch(parse_command_line(argv))
    else:
        launch_as_cgi()

    #     if argv[2] == 'date':
    #         launch(directives={ 'cmd': argv[1], 'date': argv[2], 'args': argv[2:], })
    #     else:
    #         launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'args': argv[2:], })
    # else:
    #     launch_as_cgi()
