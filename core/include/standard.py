# Copyright 2021 Randal A. Koene
# License TBD

"""
This header file declares functons and classes used by Formalizer standardized
programs and components.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import sys
import time

def wait_exit(exitcode = 0, wait_seconds: float = 0.0, verbose = True):
    if verbose:
        print('Done.', end='', flush=True)
    if (wait_seconds > 0.0):
        interval_seconds = wait_seconds / 3.0
        for i in range(3):
            time.sleep(interval_seconds)
            if verbose:
                print('.', end='', flush=True)
    if verbose:
        print('')
    sys.exit(exitcode)
