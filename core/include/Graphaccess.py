# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares functions used to obtain or select data from
a Graph.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

from error import *
from fzcmdcalls import *

def browse_for_Node(config: dict):
    print('Use the browser to select a node.')
    #retcode = pty.spawn([config['localbrowser'],'http://localhost/select.html'])
    thecmd = config['localbrowser'] + ' http://localhost/select.html'
    retcode = try_subprocess_check_output(thecmd, 'browsed', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to browse for Node selection failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        retcode = try_subprocess_check_output(f"fzgraphhtml -L 'selected' -F node -N 1 -e -q",'selected', config)
        exit_error(retcode, f'Attempt to get selected Node failed.{cmderrorreviewstr}', True)
        if (retcode == 0):
            node = (results['selected'][0:16]).decode()
            print(f'Selected: {node}')
            if results['selected']:
                return results['selected'][0:16]
            else:
                return ''
        else:
            return ''
    else:
        return ''