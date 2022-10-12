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
NEW_JSON_DATA_PATH='/var/www/webdata/formalizer/.new_daywiz_data.json'

# Collection of tables and their prefix abbreviations:
data_tables = {
	'wiztable': 'wiz',
	'nutrition': 'nutri',
	'exercise': 'exerc',
	'accounts': 'acct',
}

# ====================== Table row content template(s):

# full: [ id, time.time(), type, hr_ideal_from, hr_ideal_to, description, state, ]
# stored in JSON: [ id, time.time(), state, ]
WIZTABLE_LINES=[
	[ 'coffee', 0, 'checkbox', 6, 9, 'Make coffee.', '' ],
	[ 'vitamins', 0, 'checkbox', 6, 9, 'Take vitamins and supplements.', '' ],
	[ 'log', 0, 'checkbox', 6, 9, 'Update (or catch up) the Log.', '' ], # Log autodetectable.
	[ 'news', 0, 'checkbox', 6, 9, 'Use <a href="/formalizer/test_maketimer.html" target="_blank">a timer or skip news reading.', '' ],
	[ 'ritual', 0, 'checkbox', 6, 9, 'Ritual: Have some coffee outside.', '' ],
	[ 'weight', 0, 'number', 6, 10, 'Measure weight.', '' ],
	[ 'pushup1', 0, 'checkbox', 6, 10, 'First push-ups.', '' ],
	[ 'shower', 0, 'checkbox', 6, 10, 'Have a shower.', '' ],
	[ 'lotion', 0, 'checkbox', 6, 10, 'Put on lotion.', '' ],
	[ 'teeth', 0, 'checkbox', 6, 10, 'Brush teeth.', '' ],
	[ 'pushup2', 0, 'checkbox', 6, 10, 'Second push-ups.', '' ],
	[ 'emailparsed', 0, 'number', 8, 20, 'Emails parsed.', '' ],
	[ 'emailresp', 0, 'number', 8, 20, 'Emails responded to.', '' ],
	[ 'nextnutri', 0, 'checkbox', 18, 24, 'Plan nutrition for the next day.', '' ],
	[ 'calsync', 0, 'checkbox', 20, 24, 'Sync <a href="https://calendar.google.com/calendar/?authuser=randal.a.koene@gmail.com" target="_blank">Google Calendar</a> with Schedule.', '' ], # Automatable.
	[ 'promises', 0, 'checkbox', 20, 24, 'Review <a href="https://trello.com/b/KsIljqx3/promises"><b>Promises</b></a>, update, revise and re-prioritize as needed. Make sure Scheduling is correct.', '' ],
	[ 'passedfixed', 0, 'checkbox', 20, 24, '<a href="/cgi-bin/fzupdate-cgi.py?update=passedfixed" target="_blank">Confirm passed non-repeating Fixed Nodes to convert to Variable</a> - or - manually update them.', '' ],
	[ 'passedrepeat', 0, 'checkbox', 20, 24, 'Auto-update passed repeating Nodes to their next instances.', '' ],
	[ 'priority', 0, 'checkbox', 20, 24, 'Ensure that highest priority Promises are represented in the Schedule.', '' ], # Schedule autodetectable.
	[ 'milestones', 0, 'checkbox', 20, 24, 'Review <a href="https://trello.com/b/tlgXjZBm" target="_blank">Milestone Tracks</a>. These map to Milestones, Dependencies, Roadmap Path, Values and Urgency.', '' ],
	[ 'decisions', 0, 'checkbox', 20, 24, 'Identify Challenges and Decisions in the Schedule, tag as such. <a href="https://trello.com/b/mnu1VKIn" target="_blank">Known Challenges and Decisions</a> must be represented by at least one Node.', '' ],
	[ 'schedfits', 0, 'checkbox', 20, 24, 'Ensure schedule is mathematically doable. (Sum of required times fits into available time.)', '' ],
	[ 'variable', 0, 'checkbox', 20, 24, 'Auto-update Variable Nodes. (Or replace this with different handling of Variable Nodes.)', '' ],
	[ 'realistic', 0, 'checkbox', 20, 24, 'Ensure Schedule is realistic. E.g. map to time intervals on calendar day. Account for mini-days, and Nodes best in morning/evening.', '' ], # Automatable.
	[ 'backup', 0, 'checkbox', 20, 24, 'Make sure that Formalizer database backups are mirrored to separate storage (USB drive, mirrored account, cloud storage).', '' ], # Automatable.
	[ 'pushup3', 0, 'checkbox', 20, 24, 'Third push-ups.', '' ],
	[ 'acctcomplete', 0, 'checkbox', 20, 24, 'Accounting of the day completed.', '' ],
	[ 'armodafinil', 0, 'number', 21, 24, 'Armodafinil taken (mg).', '' ],
	[ 'alcohol', 0, 'number', 21, 24, 'Alcohol consumed (ml).', '' ],
	[ 'daynutri', 0, 'checkbox', 22, 24, 'Finalize day nutrition.', '' ],
]

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
}

# ====================== Nutrition information:

nutrition = {
	'coffee': [ 5, 'cup', ],
	'almond milk': [ 90, 'cup', ],
	'soylent': [ 400, 'bottle', ],
	'protein shake': [ 180, 'bottle', ],
	'yogurt': [ 140, '4oz', ],
	'dried apricots': [ 18, 'apricot', ],
	'raisins': [ 6, 'raisin', ],
	'dried figs': [ 36, 'fig', ],
	'celery': [ 7, 'stalk', ],
	'tomato grape': [ 1, 'grape', ],
	'orange': [ 45, 'orange', ],
	'mandarin': [ 47, 'mandarin', ],
	'blueberries': [ 85, 'cup', ],
	'avocado': [ 322, 'avocado', ],
	'banana': [ 105, 'banana', ],
	'walnut pieces': [ 190, 'quartercup', ],
	'peanuts': [ 161, 'oz', ],
	'peanut butter': [ 188, '2tbsp', ],
	'sardines': [ 36, 'oz', ],
	'tuna': [ 110, '3ozcan', ],
	'protein bar': [ 90, 'bar', ],
	'sushi': [ 33.3, 'roll', ],
	'hummus': [ 25, 'tbsp', ],
	'egg': [ 60, 'egg', ],
	'salami': [ 41, 'slice', ],
	'turkey': [ 54, 'oz', ],
	'jerky': [ 130, 'stick', ],
	'cereal with milk': [ 230, 'bowl', ],
	'rice cake': [ 35, 'rice cake', ],
	'seaweed': [ 48, 'serving', ],
	'knackebrot': [ 20, 'slice', ],
	'ezekiel bread': [ 80, 'slice', ],
	'falafel': [ 57, 'falafel', ],
	'orzo salad': [ 332, 'cup', ],
	'feta olive mix': [ 336, '200g' ],
	'broccoli cheddar bowl': [ 460, 'bowl', ],
	'palak paneer': [ 410, 'meal', ],
	'mac and cheese': [ 450, 'meal' ],
	'chili': [ 540, '2srvcan' ],
	'cheesy scramble': [ 210, 'package', ],
	'impossible burger': [ 240, 'patty', ],
	'rockstar recovery': [ 10, 'can', ],
	'potato salad': [ 44, 'oz', ],
	'egg salad': [ 90, 'oz', ],
	'beef patty': [ 240, 'patty', ],
	'hamburger': [ 420, 'burger', ],
	'sausage and egg breakfast sandwich': [ 400, 'sandwich', ],
	'quiche': [ 420, 'serving', ],
	'orowheat bread': [ 100, 'slice', ],
	'sourdough bread': [ 120, 'slice', ],
	'whole rye german breads': [ 180, 'slice', ],
	'honey': [ 64, 'tbsp', ],
	'nutella': [ 100, 'tbsp', ],
	'camembert': [ 114, 'wedge', ],
	'brie': [ 110, 'oz', ],
	'cheddar': [ 80, 'slice', ],
	'butter': [ 100, 'tbsp', ],
	'lasagna': [ 310, 'serving', ],
	'whiskey': [ 70, 'floz', ],
	'vodka': [ 64, 'floz', ],
	'morning pastry': [ 230, 'pastry', ],
	'cheetos': [ 160, 'oz', ],
	'candybar': [ 240, 'bar', ],
	'lollipop': [ 22, 'lollipop', ],
	'chips': [ 160, 'oz', ],
	'pizza with cheese': [ 250, 'slice', ],
	'gummi bears': [ 8, 'bear', ],
	'licorice pieces': [ 10, 'piece', ],
	'white chocolate': [ 160, 'oz', ],
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

# TODO: *** Remove this when no longer needed.
class debug_test:
	def __init__(self):
		from os import getcwd
		#self.current_dir = getcwd()
		self.print_this = ''

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
<tr><th>Wizard Record</th><th>Nutrition Record</th></tr>
<tr valign="top">
<td>%s</td>
<td>%s</td>
</tr>
</table>
'''

WIZTABLE_TOP='''<table class="secondcolfixedw">
<tr><th>t recommended</th><th>t actual</th><th>description</th><th>state</th><th>extra</th></tr>
'''
WIZTABLE_SUMMARY='''<tr><td></td><td></td><td></td><td>%s/%s</td><td></td></tr>
'''

# WIZLINE_FRAME='''<tr><td>%s</td><td><input type="time" id="%s" value="%s" %s></td><td>%s</td><td>%s</td><td>%s</td></tr>
# '''
WIZLINE_FRAME='''<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>
'''
WIZLINE_RECOMMENDED_FRAME='%s - %s'
WIZLINE_CHECKBOX_FRAME='<input id="%s" type="checkbox" %s %s>'
WIZLINE_NUMBER_FRAME='<input id="%s" type="text" value="%s" style="width: 8em;" %s>'

NUTRI_ACCOUNTS_TABLES_FRAME='''<table>
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
</table>
'''

NUTRI_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>consumed</th><th>quantity</th><th>units</th><th>calories</th></tr>\n'
CONSUMED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="text" value="%s" style="width: 8em;" %s></td><td>%s</td><td>%s</td>
'''
NUTRI_TABLE_SUMMARY='<tr><th></th><th></th><th></th><th></th><th>%s</th></tr>\n'
ENTRYLINE_TR_FRAME='''<tr><td><input type="time" id="nutri_add_time" value="%s" %s></td><td><input id="nutri_add_name" type="text" %s></td><td><input id="nutri_add_quantity type="text" style="width: 8em;" %s></td><td>(units)</td><td>(calories)</td>
'''

EXERCISE_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>exercised</th><th>quantity</th><th>units</th></tr>\n'
EXERCISED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="text" value="%s" style="width: 8em;" %s></td><td>%s</td>
'''
EXERCISEENTRY_TR_FRAME='''<tr><td><input type="time" id="exerc_add_time" value="%s" %s></td><td><input id="exerc_add_name" type="text" %s></td><td><input id="exerc_add_quantity type="text" style="width: 8em;" %s></td><td>(units)</td>
'''

ACCOUNTS_TABLE_HEAD='<table>\n<tr><th>approx. time</th><th>description</th><th>spent</th><th>received</th><th>category</th></tr>\n'
ACCOUNTED_TR_FRAME='''<tr><td>%s</td><td><input id="%s" type="text" value="%s" %s></td><td><input id="%s" type="number" value="%s" style="width: 8em;" %s></td><td><input id="%s" type="number" value="%s" style="width: 8em;" %s></td><td><input id="%s" type="text" value="%s" %s></td>
'''
ACCOUNTENTRY_TR_FRAME='''<tr><td><input type="time" id="acct_add_time" value="%s" %s></td><td><input id="acct_add_name" type="text" %s></td><td><input id="acct_add_spent type="number" style="width: 8em;" %s></td><td><input id="acct_add_received type="number" style="width: 8em;" %s></td><td><input id="acct_add_category type="text" %s></td>
'''

# ====================== Classes that manage content in page areas:

class wiztable_line:
	def __init__(self, day: datetime, data_list: list, idx: int):
		self.day = day
		self._idx = idx
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

	def parse_from_list(self, data_list: list):
		if len(data_list) >= 7:
			self._id = str(data_list[0])
			t_logged = float(data_list[1])
			if t_logged > 0:
				self._t = datetime.fromtimestamp(t_logged)
			self._type = str(data_list[2])
			self._hr_ideal_from = int(data_list[3])
			self._hr_ideal_to = int(data_list[4])
			self._description = str(data_list[5])
			self._state = str(data_list[6])

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
				self.number_metrics[1] = 1
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
		return WIZLINE_FRAME % ( self.recommended_str(), self.time_html(), self._description, self.state_str(), self.extra_str() )
		#return WIZLINE_FRAME % ( self.recommended_str(), self.id_str('wiz_t_'), self.time_str(), SUBMIT_ON_INPUT, self._description, self.state_str(), self.extra_str() )

	def get_data(self) ->list:
		t = 0 if self._t is None else datetime.timestamp(self._t)
		return [ self._id, t, self._state, ]

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

	def add_to_checkbox_metrics(self, chkmetrics_pair: list):
		self.checkbox_metrics[0] += chkmetrics_pair[0]
		self.checkbox_metrics[1] += chkmetrics_pair[1]

	def add_to_number_metrics(self, nummetrics_pair: list):
		self.number_metrics[0] += nummetrics_pair[0]
		self.number_metrics[1] += nummetrics_pair[1]

	def generate_html_body(self) ->str:
		self.checkbox_metrics = [ 0, 0 ]
		table_str = WIZTABLE_TOP
		for wizline in self.lines:
			table_str += wizline.generate_html_tr()
			self.add_to_checkbox_metrics(wizline.checkbox_metrics)
			self.add_to_number_metrics(wizline.number_metrics)
		table_str += WIZTABLE_SUMMARY % ( str(self.checkbox_metrics[1]+self.number_metrics[1]), str(self.checkbox_metrics[0]+self.number_metrics[0]) )
		return table_str + '</table>\n'

	def make_wiztable_index(self) ->dict:
		wiztable_dict = {}
		for i in range(len(self.lines_list)):
			wiztable_dict[self.lines_list[i][0]] = i
		return wiztable_dict

	def merge_data(self, day_data: list):
		for wizline in day_data:
			_id = wizline[0]
			if _id in self.lines_indices:
				idx = self.lines_indices[_id]
				self.lines_list[idx][1] = wizline[1]
				self.lines_list[idx][6] = wizline[2]

	def get_data(self) ->list:
		return [ self.lines[i].get_data() for i in range(len(self.lines)) ]

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

class daypage_nutritable:
	def __init__(self, day: datetime, day_data: dict, is_new: bool):
		self.day = day
		self.day_data = day_data
		self.logged = []
		if not is_new:
			if 'nutrition' in self.day_data:
				self.logged = self.day_data['nutrition']
		self.calories = 0

	def time_tuple(self, idx: int) ->tuple:
		t = self.logged[idx][0]
		dtime = t_run if t==0 else datetime.fromtimestamp(t)
		return ( dtime.hour, dtime.minute )

	def time_html(self, idx: int) ->str:
		h, m = self.time_tuple(idx)
		return TIME_FRAME % ( 'nutri_edit_th_'+str(idx), str(h), SUBMIT_ON_INPUT, 'nutri_edit_tm_'+str(idx), str(m), SUBMIT_ON_INPUT )

	def consumed_html(self, data_list: list, idx: int) ->str:
		name = str(self.logged[idx][1]).lower()
		quantity = float(self.logged[idx][2])
		if name in nutrition:
			calories = int(quantity*nutrition[name][0])
			unit = nutrition[name][1]
		else:
			calories = 5000 # As a warning that there is unmatched data.
			unit = '(unit)'
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
		table_str = NUTRI_TABLE_HEAD
		self.calories = 0
		for i in range(len(self.logged)):
			table_str += self.consumed_html(self.logged[i], i)
		table_str += NUTRI_TABLE_SUMMARY % str(self.calories)
		table_str += self.entryline_html()
		return table_str + '</table>\n'

	def get_data(self) ->list:
		return self.logged

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

class daypage_exercise:
	def __init__(self, day: datetime, day_data: dict, is_new: bool):
		self.day = day
		self.day_data = day_data
		self.logged = []
		if not is_new:
			if 'exercise' in self.day_data:
				self.logged = self.day_data['exercise']

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

	def get_data(self) ->list:
		return self.logged

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

	def get_data(self) ->list:
		return self.logged

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

class nutri_and_accounts_tables:
	def __init__(self, day: datetime, day_data: dict, is_new: bool):
		self.day = day
		self.day_data = day_data
		self.is_new = is_new

		self.frame = NUTRI_ACCOUNTS_TABLES_FRAME
		self.tables = {
			'nutrition': daypage_nutritable(day, self.day_data, is_new),
			'exercise': daypage_exercise(day, self.day_data, is_new),
			'accounts': daypage_accounts(day, self.day_data, is_new),
		}
		#print('---------------> Keys: '+str(self.tables.keys()))
		# self.nutritable = daypage_nutritable(day, self.day_data, is_new)
		# self.exercisetable = daypage_exercise(day, self.day_data, is_new)
		# self.accountstable = daypage_accounts(day, self.day_data, is_new)
		# self.tables = ( self.nutritable, self.exercisetable, self.accountstable )

	def generate_html_head(self) ->str:
		return ''

	def generate_html_body(self) ->str:
		return self.frame % tuple([ self.tables[table].generate_html_body() for table in self.tables ])
		#return self.frame % ( self.nutritable.generate_html_body(), self.exercisetable.generate_html_body(), self.accountstable.generate_html_body() )

	def get_dict(self) ->dict:
		self.day_data = {}
		for table in self.tables:
			self.day_data[table] = self.tables[table].get_data()
		# self.day_data = {
		# 	'nutrition': self.nutritable.get_data(),
		# 	'exercise': self.exercisetable.get_data(),
		# 	'accounts': self.accountstable.get_data(),
		# }
		#print('---------------> day_data Keys: '+str(self.day_data.keys()))
		return self.day_data

class daypage_tables:
	def __init__(self, day: datetime, day_data: dict, is_new: bool):
		self.day = day
		self.day_data = day_data
		self.is_new = is_new

		self.frame = DAYPAGE_TABLES_FRAME
		self.wiztable = daypage_wiztable(day, self.day_data, is_new)
		self.nutriaccountstable = nutri_and_accounts_tables(day, self.day_data, is_new)

	def generate_html_head(self) ->str:
		return DAYPAGE_WIZTABLE_STYLE

	def generate_html_body(self) ->str:
		return self.frame % (self.wiztable.generate_html_body(), self.nutriaccountstable.generate_html_body())

	def get_dict(self) ->dict:
		self.day_data = {
			'wiztable': self.wiztable.get_data(),
		}
		nea = self.nutriaccountstable.get_dict()
		#print('==============> daypage_tables.nutriaccountstable.day_data keys: '+str(nea.keys()))
		self.day_data.update(  nea )
		#print('==============> daypage_tables.day_data keys: '+str(self.day_data.keys()))
		return self.day_data

class daypage(fz_htmlpage):
	def __init__(self, directives: dict, force_new=False):
		super().__init__()
		self.day_str = directives['date']
		self.day = datetime.strptime(self.day_str, '%Y.%m.%d')
		self.is_new = force_new

		self.daywiz_data = {}
		#self.new_daywiz_data = {}
		#self.load_daywiz_json()
		self.new_load_daywiz_json()

		#gothere('daypage.__init__')
		if force_new:
			self.day_data = {}
		else:
			#self.day_data = self.get_day_data(self.day_str)
			self.day_data = self.new_get_day_data(self.day_str)
			self.is_new = (len(self.day_data) == 0)

		self.html_std = fz_html_standard('daywiz.py')
		self.html_icon = fz_html_icon()
		self.html_style = fz_html_style(['fz', ])
		self.html_uistate = fz_html_uistate()
		self.html_clock = fz_html_clock()
		self.html_title = fz_html_title('DayWiz')
		self.date_picker = fz_html_datepicker(self.day)
		self.tables = daypage_tables(self.day, self.day_data, self.is_new)
		# TODO: *** Remove the following line and all references to self.debug once no longer needed.
		self.debug = debug_test()

		self.head_list = [ self.html_std, self.html_icon, self.html_style, self.html_uistate, self.html_clock, self.tables, self.html_title, ]
		self.body_list = [ self.html_std, self.html_title, self.html_clock, self.date_picker, self.tables, self.debug, ]
		self.tail_list = [ self.html_std, self.html_uistate, self.html_clock, ]

	def load_daywiz_json(self):
		if exists(JSON_DATA_PATH):
			try:
				with open(JSON_DATA_PATH, 'r') as f:
					self.daywiz_data = json.load(f)
			except:
				self.daywiz_data = {}

	def new_load_daywiz_json(self):
		if exists(NEW_JSON_DATA_PATH):
			try:
				with open(NEW_JSON_DATA_PATH, 'r') as f:
					self.daywiz_data = json.load(f)
			except:
				self.daywiz_data = {}

	# Store JSON in format: {'YYYY.mm.dd': { 'wiztable': ..., 'nutrition': ..., 'exercise': ..., 'accounts': ...}, etc.}
	def save_daywiz_json(self) ->bool:
		global global_debug_str
		self.daywiz_data[self.day_str] = self.tables.get_dict()
		try:
			with open(JSON_DATA_PATH, 'w') as f:
				json.dump(self.daywiz_data, f)
			return True
		except Exception as e:
			global_debug_str += '<p><b>'+str(e)+'</b></p>'
			return False

	# Store JSON in format: {'wiztable': {...days...}, 'nutrition': {...days...}, 'exercise': {...days...}, 'accounts': {...days...}}
	def new_save_daywiz_json(self) ->bool:
		global global_debug_str
		day_dict = self.tables.get_dict()
		for tablekey in data_tables:
			if tablekey in day_dict:
				self.daywiz_data[tablekey][self.day_str] = day_dict[tablekey]
		try:
			with open(NEW_JSON_DATA_PATH, 'w') as f:
				json.dump(self.daywiz_data, f)
			return True
		except Exception as e:
			global_debug_str += '<p><b>'+str(e)+'</b></p>'
			return False

	# The following can be used to convert from legacy format to new format:
	# def new_save_daywiz_json(self) ->bool:
	# 	global global_debug_str
	# 	self.daywiz_data[self.day_str] = self.tables.get_dict()
	# 	self.new_daywiz_data = {
	# 		'wiztable': {},
	# 		'nutrition': {},
	# 		'exercise': {},
	# 		'accounts': {},
	# 	}
	# 	for daystr in self.daywiz_data:
	# 		daydata = self.daywiz_data[daystr]
	# 		if 'wiztable' in daydata:
	# 			self.new_daywiz_data['wiztable'][daystr] = daydata['wiztable']
	# 		if 'nutrition' in daydata:
	# 			self.new_daywiz_data['nutrition'][daystr] = daydata['nutrition']
	# 		if 'exercise' in daydata:
	# 			self.new_daywiz_data['exercise'][daystr] = daydata['exercise']
	# 		if 'accounts' in daydata:
	# 			self.new_daywiz_data['accounts'][daystr] = daydata['accounts']
	# 	try:
	# 		with open(NEW_JSON_DATA_PATH, 'w') as f:
	# 			json.dump(self.new_daywiz_data, f)
	# 		return True
	# 	except Exception as e:
	# 		global_debug_str += '<p><b>'+str(e)+'</b></p>'
	# 		return False

	def get_day_data(self, daystr: str) ->dict:
		if daystr in self.daywiz_data:
			return self.daywiz_data[daystr]
		else:
			return {}

	def new_get_day_data(self, daystr: str) ->dict:
		day_dict = {}
		for tablekey in data_tables:
			if daystr in self.daywiz_data[tablekey]:
				day_dict[tablekey] = self.daywiz_data[tablekey][daystr]
		return day_dict

	def _update_wiz(self, update_target: list, update_val: str) ->bool:
		if self.tables.wiztable.update(update_target[1], update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_nutri_add(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['nutrition'].update_add(update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_nutri_edit(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['nutrition'].update_edit(int(update_target[3]), update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_nutri(self, update_target: list, update_val: str) ->bool:
		if update_target[1] == 'add':
			return self._update_nutri_add(update_target, update_val)

		if update_target[1] == 'edit':
			return self._update_nutri_edit(update_target, update_val)

	def _update_exerc_add(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['exercise'].update_add(update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_exerc_edit(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['exercise'].update_edit(int(update_target[3]), update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_exerc(self, update_target: list, update_val: str) ->bool:
		if update_target[1] == 'add':
			return self._update_exerc_add(update_target, update_val)

		if update_target[1] == 'edit':
			return self._update_exerc_edit(update_target, update_val)

	def _update_acct_add(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['accounts'].update_add(update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_acct_edit(self, update_target: list, update_val: str) ->bool:
		if self.tables.nutriaccountstable.tables['accounts'].update_edit(int(update_target[3]), update_target[2], update_val):
			return self.new_save_daywiz_json()
			#return self.save_daywiz_json()
		return False

	def _update_acct(self, update_target: list, update_val: str) ->bool:
		if update_target[1] == 'add':
			return self._update_acct_add(update_target, update_val)

		if update_target[1] == 'edit':
			return self._update_acct_edit(update_target, update_val)

	def _update(self, update_id: str, update_val: str) ->bool:
		update_target = update_id.split('_')

		if update_target[0] == 'wiz': # [ wiz, element, line_id, ]
			return self._update_wiz(update_target, update_val)

		if update_target[0] == 'nutri': # [ nutri, add/edit, name/th/tm/quantity, idx, ]
			return self._update_nutri(update_target, update_val)

		if update_target[0] == 'exerc': # [ exerc, add/edit, name/th/tm/quantity, idx, ]
			return self._update_exerc(update_target, update_val)

		if update_target[0] == 'acct': # [ acct, add/edit, name/th/tm/quantity, idx, ]
			return self._update_acct(update_target, update_val)

		return False

	def update_from_form(self, formfields: cgi.FieldStorage) ->bool:
		return self._update(formfields.getvalue('par_changed'), formfields.getvalue('par_newval'))

	def update_from_list(self, args: list) ->bool:
		return self._update(args[0], args[1])

	def show(self):
		print("Content-type:text/html\n\n")
		print(self.generate_html())
		#self.new_save_daywiz_json() # To convert the JSON data from legacy format to new format.

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
	# 	global got_here_active
	# 	got_here_active = True
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

if __name__ == '__main__':
	from sys import argv
	if len(argv) > 2:
		if argv[2] == 'date':
			launch(directives={ 'cmd': argv[1], 'date': argv[2], 'args': argv[2:], })
		else:
			launch(directives={ 'cmd': argv[1], 'date': datetime.today().strftime('%Y.%m.%d'), 'args': argv[2:], })
	else:
		launch_as_cgi()
