#!/usr/bin/env python3
# earlywiz.py
# Copyright 2020 Randal A. Koene
# License TBD
#
# Purpose and usage:
# 1. Access to the fzguide that is stored in the database.
# 2. Processing of EarlyWiz form data.
# 3. Interactive EarlyWiz support.

# Python modules
import os
import sys
import subprocess
try:
    import cgitb; cgitb.enable()
except:
    pass
import cgi

version = "0.1.0-0.1"

config = {
    'verbose' : False
}

mode = 'interactive'

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

form = cgi.FieldStorage()
if form is not None:
    mode = 'formproc'

def initialize():
    sys.path.append(os.getenv('HOME')+'/src/formalizer/core/lib')
    sys.path.append(os.getenv('HOME')+'/src/formalizer/core/include')
    import coreversion

    print("** CONFIG NOTE: This tool (and its components from core) still need a")
    print("*              standardized method of configuration. See Trello card")
    print("*              at https://trello.com/c/4B7x2kif.\n")

    core_version = coreversion.coreversion()
    server_long_id = f"Formalizer:System:Update:AMwizard v{version} (core v{core_version})"

    print(server_long_id)


def call_guide(subsection,snippet_idx):
    try:
        res = subprocess.check_output(f'fzguide.system -q -R -A -U {subsection} -x {snippet_idx} -F txt', shell=True)
    except subprocess.CalledProcessError as cpe:
        if config['verbose']:
            print('Error output: ',cpe.output)
            print('Error code  : ',cpe.returncode)
        exit(cpe.returncode)

    else:
        if config['verbose']:
            print(res)
        
    return res


def wakeup_shower_suggestion():
    snippet = call_guide('wakeup','1.0')
    print(snippet)
    stepdone = input("Please press ENTER...")

def chkbx2bool(chkbx_val) ->bool:
    if chkbx_val is None:
        return False
    return True

class WizData:
    def __init__(self, datakeys: list, form):
        self.keys = datakeys
        self.data = {}
        for k in datakeys:
            self.data[k] = form.getvalue(k)
    def size(self) ->int:
        return len(self.data)
    def csv_str(self) ->str:
        csvstr = ''
        k = list(self.data.keys())
        for i in range(len(k)):
            if i>0:
                csvstr += ','
            csvstr += str(chkbx2bool(self.data[k[i]]))
        return csvstr
    def sum(self) ->int:
        datasum = 0
        for k in self.data:
            datasum += int(chkbx2bool(self.data[k]))
        return datasum
    def ratio(self) ->float:
        return self.sum() / len(self.data)
    def percent(self) ->int:
        return int(self.ratio()*100.0)

class FastRiseData(WizData):
    def __init__(self, form):
        super().__init__(['supplements','nutriplan','coffee','logcatchup','exercise','shower','news','outside','num_pushups',], form)

class FastStartData(WizData):
    def __init__(self, form):
        super().__init__(['calendar','passedfixed','repeating','promises','priority','milestones','challenges','sumdoable','variable','realistic','backup',], form)

class CounterProductive(WizData):
    def __init__(self, form):
        super().__init__(['excess_online',], form)

def process_earlywiz_form(form):
    fastrisedata = FastRiseData(form)
    faststartdata = FastStartData(form)
    counterproductive = CounterProductive(form)
    # Let's start just by showing what we've got and calculating a sum.
    print('Fast Rise:')
    print(fastrisedata.csv_str())
    print(' => '+str(fastrisedata.sum())+'/'+str(fastrisedata.size())+' ['+str(fastrisedata.percent())+'%]')
    print('<br>')
    print('Fast Start:')
    print(faststartdata.csv_str())
    print(' => '+str(faststartdata.sum())+'/'+str(faststartdata.size())+' ['+str(faststartdata.percent())+'%]')
    print('<br>')
    print('Counter Productive:')
    print(counterproductive.csv_str())
    print(' => '+str(counterproductive.sum())+'/'+str(counterproductive.size())+' ['+str(counterproductive.percent())+'%]')
    print('<br>')

HEAD='''Content-type:text/html

<html>
<head>
<title>EarlyWiz Output</title>
</head>
<body>
<h1>EarlyWiz Output</h1>
'''

TAIL='''
</body>
</html>
'''

if __name__ == '__main__':

    if mode == 'formproc':
        print(HEAD)

    if mode == 'interactive':
        initialize()
        wakeup_shower_suggestion()
    elif mode == 'formproc':
        process_earlywiz_form(form)
    else:
        print('Unknown mode.')

    if mode == 'formproc':
        print(TAIL)
