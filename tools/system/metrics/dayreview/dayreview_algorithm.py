# Randal A. Koene, 20240402
#
# This module contains the algorithms used to calculate the dayreview
# scores. It is shared by dayreview.py (run on the command line) and
# dayreview-cgi.py (run as a CGI script).

from datetime import datetime
from time import strftime
from json import load, dump

scorefile = '/var/www/webdata/formalizer/dayreview_scores.json'

OUTPUTTEMPLATE='The waking day yesterday was from %s to %s, containing about %.2f waking (non-nap) hours. I did %.2f hours of self-work and %.2f hours of work. I did %.2f hours of System and self-care. Other therefore took %.2f hours. The ratio total performance score is %.2f / %.2f = %.2f.'

# target, min-lim, max-lim, penalize-above, penalize-below, max-score
intended = {
    'self-work':    (6.0, 2.0, 0.0, False, True, 10.0),
    'work':         (6.0, 3.0, 0.0, False, True, 10.0),
    'sleep':        (7.0, 5.5, 8.5, True,  True, 10.0),
    'system/care':  (2.0, 0.0, 3.0, True, False,  5.0),
    'other':        (3.0, 0.0, 4.0, True, False, 10.0),
}
unmodifiable = ['sleep','system/care','other']

btf2intended = {
    'WORK': 'work',
    'SELFWORK': 'self-work',
    'SYSTEM': 'system/care',
    'OTHER': 'other',
}

def date2weekday()->str:
    pass

def btfdays2dayfocus(btf_days:str)->dict:
    if btf_days is None:
        return {}
    if btf_days == '':
        return {}
    focus_vec = btf_days.split('_')
    dayfocus = {}
    for focus in focus_vec:
        btf_focus = focus.split(':')
        btf = btf_focus[0]
        days = btf_focus[1].split(',')
        for focus_day in days:
            dayfocus[focus_day] = btf
    return dayfocus


# If btf_days is '' (empty) then use the intended matrix as above.
# Otherwise, modify it to focus on the category of the week day.

class dayreview_algorithm:
    def __init__(self, wakinghours:float, chunkdata:list, btf_days:str='', datestamp:str=''):
        self.wakinghours = wakinghours
        self.chunkdata = chunkdata
        self.dayfocus = btfdays2dayfocus(btf_days)
        if len(self.dayfocus) > 0:
            self.modify_intended(datestamp)

        self.hours_summary = None
        self.totscore = None
        self.totintended = None
        self.totscore_ratio = None

    def modify_intended(self, datestamp:str):
        date_object = datetime.strptime(datestamp, '%Y%m%d').date()
        weekday = date_object.strftime("%a").upper()
        focus = self.dayfocus[weekday]
        intendedfocus = btf2intended[focus]
        for key in intended:
            if key in unmodifiable:
                continue
            if key == intendedfocus:
                intended[key] = (8.0, 4.0, 0.0, False, True, 10.0)
            else:
                intended[key] = (4.0, 0.0, 0.0, False, True, 10.0)

    def sum_of_type(self, typeid:str)->float:
        hours = 0.0
        for i in range(len(self.chunkdata)):
            if self.chunkdata[i][1] == typeid[0]:
                hours += self.chunkdata[i][0]
        return hours

    def date_stamp(self, day:datetime)->str:
        return day.strftime('%Y.%m.%d');

    def get_intended(self, htype:str)->float:
        if htype in intended:
            return intended[htype][0]
        return 0.0

    def get_minlim(self, htype:str)->float:
        if htype in intended:
            return intended[htype][1]
        return 0.0

    def get_maxlim(self, htype:str)->float:
        if htype in intended:
            return intended[htype][2]
        return 0.0

    def get_maxscore(self, htype:str)->float:
        if htype in intended:
            return intended[htype][-1]
        return 0.0

    def get_totintended(self)->float:
        tot = 0.0
        for key_str in intended:
            tot += intended[key_str][-1]
        return tot

    def ratio_below(self, target:float, actual:float, minlim:float)->float:
        diff_below = target - actual
        below_range = target - minlim
        return diff_below / below_range # E.g. (6.0 - 4.0) / (6.0 - 3.0) = 2/3

    def ratio_above(self, target:float, actual:float, maxlim:float)->float:
        diff_above = actual - target
        above_range = maxlim - target
        return diff_above / above_range # E.g. (8.0 - 6.0) / (9.0 - 6.0) = 2/3

    def get_score(self, intended_key:str)->float:
        actual = self.hours_summary[intended_key]
        target, minlim, maxlim, penalizeabove, penalizebelow, maxscore = intended[intended_key]

        if penalizebelow:
            if actual < minlim: return 0.0
        if penalizeabove:
            if actual > maxlim: return 0.0

        if penalizebelow and (actual < target):
            reduce_by = (maxscore - 1.0)*self.ratio_below(target, actual, minlim) # E.g. 2/3 * (10.0 - 1.0) = 6.0
            return maxscore - reduce_by # E.g. 10.0 - 6.0 = 4.0

        if penalizeabove and (actual > target):
            reduce_by = (maxscore - 1.0)*self.ratio_above(target, actual, maxlim) # E.g. 2/3 * (10.0 - 1.0) = 6.0
            return maxscore - reduce_by # E.g. 10.0 - 6.0 = 4.0

        return maxscore

    def score_data_summary_line(self, key_str:str)->tuple:
        return (
                self.hours_summary[key_str],
                intended[key_str][0],
                self.get_score(key_str),
                intended[key_str][-1],
            )

    def calculate_scores(self)->dict:
        self.hours_summary = {
            'self-work': self.sum_of_type('s'),
            'work': self.sum_of_type('w'),
            'system/care': self.sum_of_type('S'),
            'dayspan': self.wakinghours,
            'nap': self.sum_of_type('n'),
        }
        self.hours_summary['awake'] = self.hours_summary['dayspan'] - self.hours_summary['nap']
        self.hours_summary['sleep'] = 24 - self.hours_summary['awake']
        self.hours_summary['other'] = self.hours_summary['awake'] - (self.hours_summary['self-work']+self.hours_summary['work']+self.hours_summary['system/care'])

        self.totscore = 0.0
        for htype in intended.keys():
            score = self.get_score(htype)
            #print('%s score = %.2f' % (str(htype), score))
            self.totscore += score
        #print('Total score: %.2f' % self.totscore)

        self.totintended = self.get_totintended()
        self.totscore_ratio = self.totscore / self.totintended

        score_data_summary = {}
        actual_hours_total = 0.0
        intended_hours_total = 0.0
        for key_str in intended:
            actual_hours_total += self.hours_summary[key_str]
            intended_hours_total += intended[key_str][0]
            score_data_summary[key_str] = self.score_data_summary_line(key_str)
        score_data_summary['totscore'] = ( actual_hours_total, intended_hours_total, self.totscore, self.totintended )
        return score_data_summary

    def summary_table(self)->dict:
        summarytable = {}
        for htype in self.hours_summary.keys():
            _intended = self.get_intended(htype)
            if _intended == 0.0:
                intended_str = '----'
            else:
                intended_str = '%.2f' % _intended
            _minlim = self.get_minlim(htype)
            if _minlim == 0.0:
                minlim_str = '----'
            else:
                minlim_str = '%.2f' % _minlim
            _maxlim = self.get_maxlim(htype)
            if _maxlim == 0.0:
                maxlim_str = '----'
            else:
                maxlim_str = '%.2f' % _maxlim
            _maxscore = self.get_maxscore(htype)
            if _maxscore == 0.0:
                maxscore_str = '----'
            else:
                maxscore_str = '%4.1f' % _maxscore
            summarytable[htype] = (
                self.hours_summary[htype],
                intended_str,
                minlim_str,
                maxlim_str,
                maxscore_str,
                str(htype))
        return summarytable

    def score_table(self)->dict:
        scoretable = {}
        for htype in intended.keys():
            scoretable[htype] = self.get_score(htype)
        return scoretable

    def get_hours_summary(self)->dict:
        return self.hours_summary

    def data_for_log(self, wakeup_stamp, gosleep_stamp)->tuple:
        return (
            str(wakeup_stamp),
            str(gosleep_stamp),
            self.hours_summary['awake'],
            self.hours_summary['self-work'],
            self.hours_summary['work'],
            self.hours_summary['system/care'],
            self.hours_summary['other'],
            self.totscore,
            self.totintended,
            self.totscore_ratio,
            )

    def string_for_log(self, wakeup_stamp, gosleep_stamp)->str:
        return OUTPUTTEMPLATE % self.data_for_log(wakeup_stamp, gosleep_stamp)

    def update_score_file(self, day:datetime, score_data:dict)->bool:
        # Read existing data from file.
        try:
            with open(scorefile, 'r') as f:
                data = load(f)
        except:
            print('No dayreview_scores.json file found, starting a fresh one.')
            data = {}
        # Update data.
        dstamp = self.date_stamp(day)
        data[dstamp] = score_data
        # Save data.
        try:
            with open(scorefile, 'w') as f:
                dump(data, f)
            return True
        except Exception as e:
            print('Unable to save data: '+str(e))
            return False
