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

