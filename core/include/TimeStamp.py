# Copyright 2021 Randal A. Koene
# License TBD

"""
This header file declares functons and classes used with Formalizer
standardized Time Stamps.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import time
from datetime import datetime, timedelta


def ActualTime():
    return time.localtime()


def TimeStamp(t):
    return time.strftime("%Y%m%d%H%M%S", t)


def NowTimeStamp():
    return time.strftime("%Y%m%d%H%M", time.localtime())


def is_int(n):
    try:
        int(n)
    except ValueError:
        return False
    return True


def is_float(n):
    try:
        float(n)
    except ValueError:
        return False
    return True


def is_TimeStamp(s):
    if not is_int(s):
        return False
    if (len(s) != 12):
        return False
    try:
        datetime.strptime(s, '%Y%m%d%H%M')
    except ValueError:
        return False
    return True

def is_Future(s):
    if (s == 'TODAY'):
        return True
    t_current_str = NowTimeStamp()
    if (int(s) > int(t_current_str)):
        return True
    else:
        return False

def add_days_to_TimeStamp(tstamp:str, numdays:int)->str:
    if len(tstamp)==12:
        d = datetime.strptime(tstamp, '%Y%m%d%H%M')
    else:
        d = datetime.strptime(tstamp[0:8], '%Y%m%d')
    delta = timedelta(days=numdays)
    return (d + delta).strftime("%Y%m%d%H%M")

def add_weeks_to_TimeStamp(tstamp:str, numweeks:int)->str:
    if len(tstamp)==12:
        d = datetime.strptime(tstamp, '%Y%m%d%H%M')
    else:
        d = datetime.strptime(tstamp[0:8], '%Y%m%d')
    delta = timedelta(weeks=numweeks)
    return (d + delta).strftime("%Y%m%d%H%M")
