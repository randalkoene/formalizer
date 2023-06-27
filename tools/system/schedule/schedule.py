#!/usr/bin/env python3
# schedule.py
# Randal A. Koene, 20230626

import os
from json import loads

# Step 1: Get Schedule data.

os.system('fzgraphhtml -I -r -o /dev/shm/fzschedule.json -E STDOUT -N 40 -D 1 -F json -q -e')

with open('/dev/shm/fzschedule.json','r') as f:
    schedjsontxt = f.read()

schedjsontxt = '[\n'+schedjsontxt[:-2]+'\n]'

scheduledata = loads(schedjsontxt)

print(str(scheduledata))

# Step 2: Convert to data-by-day.

# Step 3: Initialize days map.

# Step 4: Map exact target date entries first.

# Step 5: Then map fixed target date entries.

# Step 6: Then fill in gaps with data from variable target date entries.

# Step 7: Print the resulting schedule.
