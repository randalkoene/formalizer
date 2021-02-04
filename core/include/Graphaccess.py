# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares functions used to obtain or select data from
a Graph.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import socket
from error import *
from fzcmdcalls import *
from tcpclient import serial_API_request

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


def clear_NNL(listname: str, config: dict):
    thecmd = f"fzgraph -C '/fz/graph/namedlists/{listname}?delete='"
    if config['verbose']:
        thecmd += ' -V'
    retcode = try_subprocess_check_output(thecmd, 'clearlist', config)
    exit_error(retcode, f'Attempt to clear Named Node List {listname} failed.', True)
    if (retcode != 0):
        return False
    return True


# See, for example, how this is used in fztask.py.
def select_to_NNL(filter: str, listname: str):
    num_str = serial_API_request(f'NNLadd_match({listname},{filter})')
    return int(num_str)


def NNLlen(listname: str):
    nnl_len_str = serial_API_request(f'NNLlen({listname})')
    return int(nnl_len_str)


def edit_nodes_in_NNL(listname: str, param_label: str, valstr: str):
    num_str = serial_API_request(f'NNLedit_nodes({listname},{param_label},{valstr})')
    return int(num_str)

