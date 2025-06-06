# wiztable.py
# Copyright 2023 Randal A. Koene
# License TBD
#
# WIZTABLE data.

from datetime import datetime, timedelta

from fzhtmlpage import *

HREFBASE = 'http://localhost/'
NODELINKCGI = 'cgi-bin/fzlink.py?id='

global last_line_node
last_line_node=''

t_run = datetime.now() # Use this to make sure that all auto-generated times on the page use the same time.

# ====================== Table row content template(s):

# full: [ node, weight, id, time.time(), type, hr_ideal_from, hr_ideal_to, description, state, ]
#       weight is 1 to 5, weights are often given in accordance with discipline challenge or critical importance
#       for number (rather than checkbox) lines, if the weight is negative then its absolute value is used
#       when the value is non-empty, and if the weight is positive then it is multiplied with the number value
# stored in JSON: [ id, time.time(), state, ]
# NOTE: Horizontal lines are drawn using the WIZLINE_VISIBLE_TOPBORDER code, demarkating transitions
#       to items with a different Node link.
# >>> For more informaiton, see the description of the score calculation and the requirements for weight
# >>> settings in the comments above 'class wiztable_line' below!
WIZTABLE_LINES=[
	[ '20060914084328.1', 5, 'log', 0, 'checkbox', 6, 8, 'Update (or catch up) the Log.', '' ], # Log autodetectable.
	[ '20060914084328.1', 1, 'ritual', 0, 'checkbox', 6, 8, 'Ritual: Meditate and/or do yoga (preferably outside).', '' ],
	[ '20060914084328.1', 1, 'coffee', 0, 'checkbox', 6, 8, 'Start timer and make coffee.', '' ],
	[ '20060914084328.1', 2, 'vitamins', 0, 'checkbox', 6, 8, 'Take vitamins and supplements.', '' ],
    [ '20060914084328.1', 2, 'vasodilator', 0, 'checkbox', 6, 8, 'Take a vasodilator.', '' ],
	[ '20060914084328.1', 3, 'news', 0, 'checkbox', 6, 8, 'Use <a href="/formalizer/test_maketimer.html" target="_blank">a timer or skip news reading.', '' ],

	[ '20091115180507.1', 3, 'finalized', 0, 'checkbox', 6, 9, 'Daywiz of previous day was finalized.', '' ], # Daywiz autodetectable.
	[ '20091115180507.1', 4, 'timeuse', 0, 'checkbox', 6, 9, 'Review how I <b>used my time</b> (tool: dayreview).', '' ],
	[ '20091115180507.1', 5, 'calsync', 0, 'checkbox', 6, 9, 'Sync <a href="https://calendar.google.com/calendar/?authuser=rkoene@carboncopies.org" target="_blank">Google Calendar</a> with Schedule.', '' ], # Automatable.
	[ '20091115180507.1', 3, 'promises', 0, 'checkbox', 6, 9, 'Review <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=promises&sort_by=targetdate" target="_blank"><b>Promises</b></a>. Update, revise and re-prioritize as needed. Make sure Scheduling is correct.', '' ],
	[ '20091115180507.1', 5, 'passedfixed', 0, 'checkbox', 6, 9, '<a href="/cgi-bin/fzupdate-cgi.py?update=passedfixed" target="_blank">Confirm passed non-repeating Fixed Nodes to convert to Variable</a> - or - manually update them.', '' ],
	[ '20091115180507.1', 5, 'passedrepeat', 0, 'checkbox', 6, 9, 'Auto-update passed repeating Nodes to their next instances.', '' ],
	[ '20091115180507.1', 3, 'priority', 0, 'checkbox', 6, 9, 'Ensure that highest priority <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=promises" target="_blank">Promises are represented in the Schedule</a>.', '' ], # Schedule autodetectable.
	[ '20091115180507.1', 4, 'milestones', 0, 'checkbox', 6, 9, 'Review progress on <a class="nnl" href="/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u=204512311159&r=100&U=true&N=7" target="_blank">Weekly Goals</a> and current <a href="/cgi-bin/nodeboard-cgi.py?D=threads&T=true" target="_blank"><b>Threads</b></a> and their <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=threads" target="_blank">Scheduling</a>. Ensure there are Nodes to complete upcoming <a href="/cgi-bin/nodeboard-cgi.py?n=20101230042609.1" target="_blank"><b>Milestones</b></a>.', '' ],
	[ '20091115180507.1', 4, 'decisions', 0, 'checkbox', 6, 9, 'Identify <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=challenges_open" target="_blank"><b>Challenges and Decisions in the Schedule</b></a>. 1) Add to <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=challenges_open&sort_by=targetdate" target="_blank">NNL</a> as needed. 2) Ensure challenges are scheduled early in the day.', '' ],
    [ '20091115180507.1', 2, 'checkboxes', 0, 'checkbox', 6, 9, 'Review and potentially convert recent open checkboxes in the Log.', '' ],
    [ '20091115180507.1', 2, 'variable', 0, 'checkbox', 6, 9, 'Auto-update Variable Nodes.', '' ],
	[ '20091115180507.1', 5, 'schedfits', 0, 'checkbox', 6, 9, 'Ensure schedule is mathematically doable.', '' ],
	[ '20091115180507.1', 5, 'realistic', 0, 'checkbox', 6, 9, 'Ensure Schedule is realistic.', '' ], # Automatable.
	[ '20091115180507.1', 5, 'relations', 0, 'checkbox', 6, 9, 'Prepare the list of people that I already know I should communicate with this day.', '' ],
	[ '20091115180507.1', 5, 'backup', 0, 'checkbox', 6, 9, '<button class="button button1" onclick="window.open(\'/cgi-bin/fzbackup-from-web-cgi.py\',\'_blank\');">Ensure backup of the Formalizer database</button>.', '' ], # Automatable.

	[ '20211017053846.1', -4, 'weight', 0, 'number', 6, 10, 'Measure <a href="/cgi-bin/metrics.py?cmd=show&selectors=wiztable" target="_blank">weight</a>.', '' ],
    [ '20211017053846.1', -4, 'bloodpressure', 0, 'number', 6, 10, 'Measure blood pressure.', '' ],
	[ '20211017053846.1', 4, 'pushup1', 0, 'checkbox', 6, 10, 'First push-ups or weights (e.g. before/after shower).', '' ],
	[ '20211017053846.1', 2, 'shower', 0, 'checkbox', 6, 10, 'Have a shower.', '' ],
	[ '20211017053846.1', 3, 'lotion', 0, 'checkbox', 6, 10, 'Put on lotion.', '' ],
	[ '20211017053846.1', 4, 'teeth', 0, 'checkbox', 6, 10, 'Brush teeth and/or chew gum.', '' ],

	[ '20081125102516.1', -5, 'emailparsed', 0, 'number', 7, 12, 'Emails parsed.', '' ],
	[ '20081125102516.1', 10, 'emailurgent', 0, 'checkbox', 7, 12, 'Urgent emails identified.', '' ],
	[ '20081125102516.1', -10, 'emailresp', 0, 'number', 7, 12, 'Emails responded to.', '' ],
	[ '20081125102516.1', 4, 'signalcheck', 0, 'checkbox', 7, 12, 'Signal messages caught up.', '' ],
	[ '20081125102516.1', 4, 'whatsappcheck', 0, 'checkbox', 7, 12, 'Whatsapp messages caught up.', '' ],
	[ '20081125102516.1', 4, 'fbcheck', 0, 'checkbox', 7, 12, 'Messenger messages and Facebook caught up.', '' ],
	[ '20081125102516.1', 4, 'chatcheck', 0, 'checkbox', 7, 12, 'Google chat messages and spaces caught up.', '' ],
    [ '20081125102516.1', 4, 'discord', 0, 'checkbox', 7, 12, 'Discord caught up.', '' ],
    [ '20081125102516.1', 4, 'taigacheck', 0, 'checkbox', 7, 12, 'Taiga notifications caught up.', '' ],

	[ '20140428114648.1', 2, 'neoprene', 0, 'checkbox', 7, 13, 'Wearing clothing where bloat/size is detectable (or neoprene).', '' ],
	[ '20140428114648.1', 4, 'pushup2', 0, 'checkbox', 7, 13, 'Second push-ups or weights (e.g. during lunch).', '' ],

	[ '20140921041431.1', 5, 'startedearly', 0, 'checkbox', 8, 13, 'Started early, at least an hour into priority intentions by 10 am.', ''],
	[ '20140921041431.1', 4, 'lessthan300', 0, 'checkbox', 8, 13, 'Had less than 300 calories in the morning.', ''],
	[ '20140921041431.1', 3, 'morningfast', 0, 'checkbox', 8, 13, 'Did not eat in the morning.', ''],

	[ '20040402045825.1', 5, 'firstwork', 0, 'checkbox', 13, 16, 'Managed at least 4 hours of actual work progress in the first miniday (by 2pm).', ''],
	[ '20040402045825.1', 4, 'progress', 0, 'checkbox', 13, 16, '<a class="nnl" href="/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u=204512311159&r=100&U=true&N=7" target="_blank">Checked progress bars</a> for Weekly Goals (by 2pm).', ''],
    [ '20040402045825.1', 5, 'standing', 0, 'checkbox', 13, 16, 'I stood rather than sat for desk work.', ''],

	[ '20100403134619.1', 4, 'endurance', 0, 'checkbox', 9, 22, 'Endurance exercise (e.g rowing, dancing, rollerblading).', '' ],
	[ '20100403134619.1', 4, 'pushup3', 0, 'checkbox', 20, 24, 'Third push-ups or weights (e.g. during dinner).', '' ],
	[ '20100403134619.1', 4, 'weight2', 0, 'checkbox', 19, 24, 'Weighed myself (again) in the evening.', '' ],

	[ '20200601093905.1', 5, 'acctcomplete', 0, 'checkbox', 20, 24, 'Accounting of the day completed.', '' ],
	[ '20200601093905.1', 5, 'secondwork', 0, 'checkbox', 20, 24, 'Managed at least 4 hours of actual work progress in the second miniday.', '' ],

	[ '20230727204001.1', 5, 'lovedbody', 0, 'checkbox', 20, 24, 'Loved my body.', '' ],
	[ '20230727204001.1', 5, 'lovedmind', 0, 'checkbox', 20, 24, 'Loved my mind.', '' ],

	[ '20200601093905.1', -3, 'armodafinil', 0, 'number', 21, 24, 'Armodafinil taken (mg).', '' ],
	[ '20200601093905.1', -3, 'alcohol', 0, 'number', 21, 24, 'Alcohol consumed (ml).', '' ],
    [ '20200601093905.1', -3, 'nicotine', 0, 'number', 21, 24, 'Nicotine consumed (mg). (Target: up to 20 mg.)', '' ],
    [ '20200601093905.1', -3, 'caffeine', 0, 'number', 21, 24, 'Caffeine consumed (cups). (Target: up to 9 cups.)', '' ],
	[ '20200601093905.1', 4, 'daynutri', 0, 'checkbox', 22, 24, 'Finalize day nutrition.', '' ],
	[ '20200601093905.1', 4, 'fastafter8', 0, 'checkbox', 22, 24, 'Did not eat after 8pm.', '' ],
	[ '20200601093905.1', 2, 'nextnutri', 0, 'checkbox', 22, 24, 'Plan nutrition for the next day.', '' ],
	[ '20200601093905.1', 5, 'challenge', 0, 'checkbox', 22, 24, 'Addressed at least 1 challenge.', '' ],
	[ '20200601093905.1', 10, 'visible', 0, 'checkbox', 22, 24, 'Did something externally visible (<b>add NNL here!</b>) today.', '' ],
	[ '20200601093905.1', 45, 'reviewscore', 0, 'number', 22, 24, '<button class="button button1" onclick="window.open(\'/cgi-bin/fzloghtml-cgi.py?review=true\',\'_blank\');">Day review score</button>.', '' ],
]
WIZTABLE_LINES_NODE=0
WIZTABLE_LINES_WEIGHT=1
WIZTABLE_LINES_ITEM=2
WIZTABLE_LINES_TIME=3
WIZTABLE_LINES_TYPE=4
WIZTABLE_LINES_HRFROM=5
WIZTABLE_LINES_HRTO=6
WIZTABLE_LINES_DESC=7
WIZTABLE_LINES_STATE=8

WIZTABLE_LINE_LENGTH=len(WIZTABLE_LINES[0])

WIZLINE_VISIBLE_TOPBORDER='style="border-top: 1px solid var(--color-text);"'

# WIZLINE_FRAME='''<tr><td>%s</td><td><input type="time" id="%s" value="%s" %s></td><td>%s</td><td>%s</td><td>%s</td></tr>
# '''
WIZLINE_FRAME='''<tr><td>%s</td><td>%s</td><td %s>[%s, <a href="%s">node</a>] %s [<a href="/formalizer/system-documentation.html#wiztable-%s">ref</a>]</td><td>%s</td><td>%s</td></tr>
'''

WIZLINE_RECOMMENDED_FRAME='%s - %s'

WIZLINE_CHECKBOX_FRAME='<input id="%s" type="checkbox" %s %s>'
WIZLINE_NUMBER_FRAME='<input id="%s" type="text" value="%s" style="width: 8em;" %s>'

TIME_FRAME='<input type="number" min=0 max=23 id="%s" value="%s" style="width: 3em;" %s>:<input type="number" min=0 max=59 id="%s" value="%s" style="width: 3em;" %s>'

# How Scores are calculated and the importance of careful positive or
# negative weight setting in wiztable.py:
# -------------------------------------------------------------------
#
# wizline._state: str 'checked' or new data value (not that this is not normalized to [0,1])
# wizline._weight: pos or neg weight as listed in wiztable
# wizline._checkbox_metrics[0]: 1 if checkbox, 0 otherwise -- CHECKBOX flag
# wizline._checkbox_metrics[1]: 1 if checked, 0 otherwise -- value used for CHECKBOX
# wizline._number_metrics[0]: 1 if number, 0 otherwise -- NUMBER flag
# wizline._number_metrics[1]: -- value used for NUMBER
#   0 if _state is '', or
#     1 if _weight is negative
#     float(_state) if _weight >=0, should be between 0 and 1 ***

# For each line:

# weight=abs(wizline._weight)

# checkbox_metrics_0 += CHECKBOX flag -- count number of checkboxes
# checkbox_metrics_1 += CHECKBOX value -- count number of checked checkboxes
# score_possible += weight*CHECKBOX flag -- add weighting if CHECKBOX
# score += weight*CHECKBOX value -- add weighting if checked

# number_metrics_0 += NUMBER flag -- count number of numbers
# number_metrics_1 += 1 if (NUMBER value>0) -- count number of filled-in numbers
# score_possible += weight*NUMBER flag -- add weighting if NUMBER
# score += int(weight*NUMBER value) -- weighted normalized value ***

# It's very important that only [0.0,1.0] number values have positive _weight
# and that all number values that can have any number have negative _weight.
#
# The calculations shown above take place in the function update_total_scores().
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

    def state_tuple(self)->tuple:
        if self._type == 'checkbox':
            self.checkbox_metrics[0] = 1
            if self._state == 'checked':
                self.checkbox_metrics[1] = 1
            return (self.checkbox_metrics[0], self.checkbox_metrics[1])
        elif self._type == 'number':
            self.number_metrics[0] = 1
            if self._state != '':
                if self._weight < 0:
                    self.number_metrics[1] = 1
                else:
                    self.number_metrics[1] = float(self._state) # The value should be between 0.0 and 1.0.
            return (self.number_metrics[0], self.number_metrics[1])
        else:
            return (0, 0)

    def state_str(self) ->str:
        self.state_tuple() # update metrics
        if self._type == 'checkbox':
            return WIZLINE_CHECKBOX_FRAME % ( self.id_str('wiz_state_'), self._state, SUBMIT_ON_CHANGE )
        elif self._type == 'number':
            return WIZLINE_NUMBER_FRAME % ( self.id_str('wiz_state_'), str(self._state), SUBMIT_ON_INPUT )
        else:
            return ''

    def add_to_checkbox_metrics(self, checkbox_metrics_ref)->tuple:
        checkbox_metrics_ref[0] += self.checkbox_metrics[0]
        checkbox_metrics_ref[1] += self.checkbox_metrics[1]
        addpossible = self.weight() * self.checkbox_metrics[0]
        addscore = self.weight() * self.checkbox_metrics[1]
        return addpossible, addscore

    def add_to_number_metrics(self, number_metrics_ref)->tuple:
        number_metrics_ref[0] += self.number_metrics[0]
        if self.number_metrics[1] > 0:
            number_metrics_ref[1] += 1
        addpossible = self.weight() * self.number_metrics[0]
        addscore = int(self.weight() * self.number_metrics[1])
        return addpossible, addscore

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
