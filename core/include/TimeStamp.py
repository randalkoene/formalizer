# Copyright 2021 Randal A. Koene
# License TBD

"""
This header file declares functons and classes used with Formalizer
standardized Time Stamps.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import time
from datetime import datetime

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
    t_current_str = time.strftime("%Y%m%d%H%M", time.localtime())
    if (int(s) > int(t_current_str)):
        return True
    else:
        return False

