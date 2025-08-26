#!/usr/bin/python3
#
# selected_to_nth.py
#
# Randal A. Koene, 20250826
#
# Add a Node reference as per the selected Node to the nth
# list item under the Randal actions header within a Node.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
#import traceback
#from io import StringIO
#from traceback import print_exc
import subprocess

# Print the HTTP header
print("Content-Type: text/plain\n")

form = cgi.FieldStorage()

data = form.getvalue('data')

# Copied from fzcmdcalls.py to simplify using this as CGI script.
results = {}
def try_subprocess_check_output(thecmdstring, resstore, config: dict):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except subprocess.CalledProcessError as cpe:
        try:
            config['cmderrorreviewstr'] = cpe.output.decode()
        except:
            pass
        if config['logcmderrors']:
            with open(config['cmderrlog'],'a') as f:
                f.write(f'Subprocess call ({thecmdstring}) caused exception.\n')
                f.write(f'Error output: {cpe.output.decode()}\n')
                f.write(f'Error code  : {cpe.returncode}\n')
        if config['verbose']:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
        return cpe.returncode

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

def chars2str(buf, maxlen:int)->str:
    if isinstance(buf, str):
        return buf[:maxlen]
    return buf.decode()[:maxlen]

# Copied from Graphaccess.py to simplify using this as CGI script.
def get_node_data(node: str, config:dict=None, running_as_cgi=True):
    if config is None:
        config= {
            'verbose': False,
            'logcmdcalls': False,
            'cmdlog': '',
            'logcmderrors': False,
            'cmderrlog': '',
        }
    if running_as_cgi:
        thecmd = f'./fzgraphhtml -n {node} -o STDOUT -F json -e -q'
    else:
        thecmd = f'fzgraphhtml -n {node} -o STDOUT -F json -e -q'
    retcode = try_subprocess_check_output(thecmd, 'nodedata', config)
    if (retcode != 0):
        return None
    else:
        from json import loads
        return loads(results['nodedata'])

forbidden_set = set('\r\n ')

def is_non_empty(input_string:str)->bool:
    input_set = set(input_string)
    return bool(input_set.difference(forbidden_set))

def extract_list_data(header:str, content:str)->list:
    h_pos = content.find(header)
    if h_pos < 0:
        return None
    ls_pos = content.find('<UL>', h_pos)
    if ls_pos < 0:
        return None
    ls_pos += 4
    le_pos = content.find('</UL>', ls_pos)
    if le_pos < 0:
        return None
    extracted = content[ls_pos:le_pos]
    extracted_list = extracted.split('<LI>')
    return [ line for line in extracted_list if is_non_empty(line) ]

def clean_list(list_data:list):
    for i in range(len(list_data)):
        list_data[i] = list_data[i].strip()

def has_node(line:str)->tuple:
    closing_pos = line.find(']')
    if closing_pos < 0:
        return False, closing_pos
    line_pre = line[:closing_pos]
    opening_pos = line_pre.find('[')
    if opening_pos < 0:
        return False, closing_pos
    node_id = line_pre[opening_pos+1:].strip()
    if len(node_id) != 16:
        return False, closing_pos
    if node_id[-2]=='.' and node_id[-1].isdigit() and node_id[:-2].isdigit():
        return True, closing_pos
    return False, closing_pos

def change_node(selected:str, list_data:list, idx:int, closing_pos:int):
    new_line = '[ '+selected+' ]'+list_data[idx][closing_pos+1:]
    list_data[idx] = new_line

def add_node(selected, list_data, idx):
    new_line = '[ '+selected+' ] '+list_data[idx]
    list_data[idx] = new_line

def replace_list_data(header:str, content:str, list_data:list)->str:
    h_pos = content.find(header)
    if h_pos < 0:
        return None
    ls_pos = content.find('<UL>', h_pos)
    if ls_pos < 0:
        return None
    ls_pos += 4
    le_pos = content.find('</UL>', ls_pos)
    if le_pos < 0:
        return None
    new_content = content[:ls_pos] + '\n<LI>' + '\n<LI>'.join(list_data) + '\n' + content[le_pos:]
    return new_content

textfile = '/var/www/webdata/formalizer/node-text.html'

config= {
        'verbose': False,
        'logcmdcalls': False,
        'cmdlog': '',
        'logcmderrors': False,
        'cmderrlog': '',
    }

thecmd = "./fzgraphhtml -L 'selected' -F node -e -q"
retcode = 0
try:
    retcode = try_subprocess_check_output(thecmd,'selected_node', config)
    if (retcode == 0):
        selected = chars2str(results['selected_node'], 16)
        idx, node_id = data.split(':')
        idx = int(idx)
        # Get Node content
        node_data = get_node_data(node_id)
        # Get list lines
        list_data = extract_list_data('Randal', node_data['node-text'])
        # Detect node-link at line
        if idx >= len(list_data):
            print('Index is beyond list length')
        else:
            clean_list(list_data)
            # Modify or add
            hasnode, closing_pos = has_node(list_data[idx])
            if hasnode:
                change_node(selected, list_data, idx, closing_pos)
            else:
                add_node(selected, list_data, idx)
            new_content = replace_list_data('Randal', node_data['node-text'], list_data)
            # Request modification of Node content
            with open(textfile,'w') as f:
                f.write(new_content)
            thecmd = f"./fzedit -q -E STDOUT -M {node_id} -f {textfile}"
            retcode = try_subprocess_check_output(thecmd,'edit_node', config)
            # Report
            if retcode == 0:
                print('Associated Node %s' % selected)
            else:
                print(f'Attempt to edit Node failed.{config["cmderrorreviewstr"]}')

    else:
        print(f'Attempt to get description of selected Node failed.{config["cmderrorreviewstr"]}')
except Exception as e:
    print('Exception: '+str(e))
