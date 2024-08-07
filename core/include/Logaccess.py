# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares functions used to obtain or select data from
a Log, which is sometimes combined with data from the Graph.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import re
from error import *
from fzcmdcalls import *


def get_most_recent(config: dict):
    retcode = try_subprocess_check_output(f"fzloghtml -R -o STDOUT -N -F raw -q", 'mostrecent', config)
    exit_error(retcode, 'Attempt to get most recent Log Chunk parameters failed.', True)
    if (retcode != 0):
        return False
    return True


def get_updated_shortlist(config: dict):
    retcode = try_subprocess_check_output(f"fzgraphhtml -u -L 'shortlist' -F node -e -q", 'shortlistnode', config)
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List node data failed.', True)
    if (retcode != 0):
        return False
    retcode = try_subprocess_check_output(f"fzgraphhtml -L 'shortlist' -F desc -x 60 -e -q", 'shortlistdesc', config)
    exit_error(retcode, 'Attempt to get "shortlist" Named Node List description data failed.', True)
    if (retcode != 0):
        return False
    return True


class ShortList:
    def __init__(self, title_msg: str, config: dict):
        self.nodes = ''
        self.desc = ''
        self.vec = [ ]
        self.title = title_msg
        self.gotshortlist = get_updated_shortlist(config)
        if self.gotshortlist:
            self.nodes = results['shortlistnode']
            self.desc = results['shortlistdesc']
            #self.vec = [s for s in self.desc.decode().splitlines() if s.strip()]
            self.vec = [s for s in self.desc.decode().split("@@@") if s.strip()]
            self.size = len(self.vec)
    def show(self):
        print(self.title)
        if self.gotshortlist:
            pattern = re.compile('[\W_]+')
            for (number, line) in enumerate(self.vec):
                printableline = pattern.sub(' ',line).strip()
                print(f' {number}: {printableline}')


def get_Log_days_data(numdays: int, config: dict):
    thecmd = f"fzloghtml -D {numdays} -r -T 'STR:{{{{ t_chunkopen }}}} {{{{ t_chunkclose }}}} {{{{ t_diff_mins }}}} {{{{ node_id }}}}\n' -o STDOUT -N -q"
    retcode = try_subprocess_check_output(thecmd, 'recentlogdata', config)
    exit_error(retcode, 'Attempt to get recent Log data failed.', True)
    if (retcode != 0):
        return False
    return True

def get_Log_interval_data(from_date: str, to_date: str, config: dict):
    thecmd = f"fzloghtml -1 {from_date} -2 {to_date} -T 'STR:{{{{ t_chunkopen }}}} {{{{ t_chunkclose }}}} {{{{ t_diff_mins }}}} {{{{ node_id }}}}\n' -o STDOUT -N -q"
    retcode = try_subprocess_check_output(thecmd, 'intervallogdata', config)
    exit_error(retcode, 'Attempt to get Log interval data failed.', True)
    if (retcode != 0):
        return False
    return True
