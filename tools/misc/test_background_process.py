#!/usr/bin/python3
#
# Randal A. Koene, 20240106
#
# This is a background process used by the test_progress_indicator.py test script.
#

print('Log: The background process is running...', flush=True)

# Do this immediately, so that any errors are visible:
import sys, os
#from os.path import exists
#sys.stderr = sys.stdout
#from time import strftime
#import datetime
#import traceback
#from json import loads, load
#from io import StringIO
#from traceback import print_exc
#from subprocess import Popen, PIPE
#from pathlib import Path
from time import sleep
#home = str(Path.home())

progress_file = '/var/www/webdata/formalizer/test_progress.state'

def write_progress(progress:int):
    try:
        with open(progress_file, 'w') as f:
            f.write(str(progress))
    except:
        pass

def run_background_process():
    progress = 0
    while progress <= 100:
        write_progress(progress)
        sleep(1)
        progress += 10

# This STDOUT output will go to the log file indicated in the launch
# of this script. See test_progress_indicator.py.
def info_output():
    pass
    # user = os.environ['USER']
    # print('User: %s' % user)

if __name__ == '__main__':

    info_output()

    run_background_process()

    sys.exit(0)
