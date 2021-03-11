#! /usr/bin/python3
#
# fztask-test.py, Randal A. Koene, 2021-03-11
# 
# This script tests Task Chunk events. Before running this task carry out
# the preceding SSE test described in the README.md, which uses the fztaskAPI.py
# module directly for a ping-pong test.

import time
from datetime import datetime
import fztaskAPI

if __name__ == '__main__':

    while True:
        starttask = input('Next, signal the start of a task (press ENTER).')
        now = datetime.now()
        t_stamp = now.strftime(r'%Y%m%d%H%M')
        mins = 20
        print(f'Starting TC at {t_stamp} for {mins} minutes.')
        fztaskAPI.start_task_chunk('localhost:5000', t_stamp, mins)
        endtask = input('Next, signal the end of the task (press ENTER).')
        now = datetime.now()
        t_stamp = now.strftime(r'%Y%m%d%H%M')
        print(f'Ending TC at {t_stamp}.')
        fztaskAPI.end_task_chunk('localhost:5000', t_stamp)
