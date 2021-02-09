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


def selected_Node_description(config: dict, excerpt_len = 0):
    thecmd = "fzgraphhtml -L 'selected' -F desc -N 1 -e -q"
    if (excerpt_len > 0):
        thecmd += f' -x {excerpt_len}'
    retcode = try_subprocess_check_output(thecmd,'selected_desc', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get description of selected Node failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        res_selected_desc = results['selected_desc']
        selected_desc_vec = res_selected_desc.decode().split("@@@")
        return selected_desc_vec[0]
    else:
        return ''


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


def get_node_data(node: str, param_labels: str, config: dict):
    # *** not yet implemented, decide if you use GET or FZ method
    # *** here's a less efficient, temporary version
    thecmd = f'fzgraphhtml -n {node} -o STDOUT -F node -e -q'
    retcode = try_subprocess_check_output(thecmd, 'nodedata', config)
    exit_error(retcode, f'Attempt to get Node data for Node {node} failed.', True)
    if (retcode != 0):
        return 'unknown', '20'
    else:
        # *** temporary version only works for 'tdproperty, required'
        nodedata = results['nodedata'].split(b'\n')
        required_mins = nodedata[2].decode()
        tdproperty = nodedata[5].decode()
        return tdproperty, required_mins


def get_main_topic(node: str, config: dict):
    # *** This can be made easier if there is a simple way to get just a a specific
    #     parameter of a node, for example through the direct TCP-port API.
    #     E.g. could call fzgraph -C or curl with the corresponding URL.
    customtemplate = '{{ topics }}'
    with open(config['customtemplate'],'w') as f:
        f.write(customtemplate)
    customtemplatefile = config['customtemplate']
    topicgettingcmd = f"fzgraphhtml -q -T 'Node={customtemplatefile}' -n {node}"
    retcode = try_subprocess_check_output(topicgettingcmd, 'topic', config)
    cmderrorreviewstr = config['cmderrorreviewstr']
    exit_error(retcode, f'Attempt to get Node topic failed.{cmderrorreviewstr}', True)
    if (retcode == 0):
        topic = results['topic'].split()[0]
        topic = topic.decode()
    else:
        topic = ''
    return topic
