#!/usr/bin/python3
#
# Randal A. Koene, 20260110
#
# Generate a grid with available textarea cells for sub-hierarchy Node entries.

start_CGI_output = '''Content-type:text/html
'''

# This helps spot errors by printing to the browser.
print(start_CGI_output)

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
#from time import strftime
#import traceback
#from io import StringIO
#from traceback import print_exc
import subprocess
import json
import copy

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

action = form.getvalue('action')
if not action:
    action='Redraw'
node = form.getvalue('node')

if action=='Redraw':
    depth = form.getvalue('depth')
    if depth:
        depth = int(depth)
    branches = form.getvalue('branches')
    if branches:
        branches = int(branches)

    max_width = branches**(depth-1)

    cell_width = form.getvalue('cell_width')
    if not cell_width:
        cell_width = 20
    else:
        cell_width = int(cell_width)
    cell_height = form.getvalue('cell_height')
    if not cell_height:
        cell_height = 10
    else:
        cell_height = int(cell_height)

hsep = 2
vsep = 3

req_default = 2.0

TESTING=False

ENTRY_DIV = '''<div style="position:absolute; top: %s; left: %s; width: %s; height: %s;">
<textarea style="width:100%%; resize: none;" rows="%s" name="%s" id="%s">%s</textarea><br>
Hours Required: <input id="%s" name="%s" type="text" size=8 value="%s">
Targetdate: <input id="%s" name="%s" type="text" size=15 value="%s">
</div>
'''

PAGE_HTML = '''<html>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Specify Sub-Hierarchy</title>
<style>
body {
overflow-x: auto;
}
</style>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h1>Specify Sub-Hierarchy</h1>
<form action="/cgi-bin/subhierarchy.py" method="post">
<a class="nnl" href="/cgi-bin/fzlink.py?id=%s" target="_blank">%s</a>
<input type="submit" name="action" value="Redraw">
<input type="hidden" name="node" value="%s">
Depth: <input type="text" size=8 name="depth" value="%s">
Branches: <input type="text" size=8 name="branches" value="%s">
Min. cell width: <input type="text" size=8 name="cell_width" value="%s">
Cell height: <input type="text" size=8 name="cell_height" value="%s">
</form>
<form action="/cgi-bin/subhierarchy.py" method="post">
<input type="hidden" name="node" value="%s">
<p>
<input type="submit" name="action" value="Generate"> (Take care, this cannot be undone!)
</p>
<div style="position:relative;">
%s
</div>
</form>
</body>
</html>
'''

# Copied from fzcmdcalls.py to simplify using this as CGI script.
results = {}
def try_subprocess_check_output(
    thecmdstring:str,
    resstore:str=None,
    config:dict={
        'verbose': False,
        'logcmdcalls': False,
        'cmdlog': '',
        'logcmderrors': False,
        'cmderrlog': '',
    })->tuple:

    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        try:
            with open(config['cmdlog'],'a') as f:
                f.write(thecmdstring+'\n')
        except:
            pass
    results['thecmd'] = thecmdstring
    results['error'] = ''

    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except subprocess.CalledProcessError as cpe:
        errorstr = f'Error output: {str(cpe.output)}\nError code: {cpe.returncode}'
        results['error'] = errorstr
        errorstr += f'Subprocess call: {str(cpe.cmd)}\n'+errorstr+'\n'
        if config['logcmderrors']:
            try:
                with open(config['cmderrlog'],'a') as f:
                    f.write(errorstr)
            except:
                pass
        if config['verbose']:
            print(errorstr)
        return cpe.returncode, results['error']

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            if isinstance(res, bytes):
                print(res.decode(), flush=True)
            else:
                print(res, flush=True)
        if isinstance(res, bytes):
            return 0, res.decode()
        return 0, res

def get_node_content()->tuple:
    thecmd = f'./fzgraphhtml -q -o STDOUT -F json -n {node}'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        return f'Error: Failed to execute {thecmd}', 0.0, ''
    try:
        data = json.loads(res)
        return data['node-text'], float(data['required'])/60.0, data['targetdate']
    except Exception as e:
        return f'Error: {str(e)}', 0.0, ''

'''
depth=4
d = depth - 1 = 3
maxw = 2^d = 8
0th level: y=0, 1                [prevx=0]
1st level: y=1, 2, 0 & 4         [prevx=b*maxw/(2^y),                                                                 prevx=b*maxw/(2^y)=4]
2nd level: y=2, 4, 0 & 2 & 4 & 6 [prevx=b*maxw/(2^y)=0,                   prevx=b*maxw/(2^y)=2,                       prevx=prevx+b*maxw/(2^y)=4,                 prevx=prevx+b*maxw/(2^y)=6]
3rd level: y=3, 8,               [prevx+b*maxw/(2^y)=0, prevx+b*maxw/8=1, prevx+b*maxw/(2^y)=2, prevx+b*maxw/(2^y)=3, prevx+b*maxw/(2^y)=4, prevx+b*maxw/(2^y)=5, ...]
'''

top_targetdate = ''

def generate_cell(y:int, b:int, prev_x:int, name:str):
    global top_targetdate
    divisor = branches**y
    multi = max_width/divisor
    x = prev_x + (b*multi)
    i = (cell_width+hsep)*x
    j = (cell_height+vsep)*y
    if b==0 and y==0:
        cellvalue, req_hrs, targetdate = get_node_content()
        top_targetdate = targetdate
    else:
        if TESTING:
            cellvalue = name
        else:
            cellvalue = ''
        req_hrs = '%.2f' % req_default
        targetdate = ''
        
    cell_html = ENTRY_DIV % (
        '%dem' % j,
        '%dem' % i,
        '%dem' % (cell_width*multi+1),
        '%dem' % cell_height,
        str(cell_height),
        name,
        name,
        cellvalue,
        'req_'+name,
        'req_'+name,
        req_hrs,
        'td_'+name,
        'td_'+name,
        targetdate)
    y += 1
    if y < depth:
        for b in range(branches):
            cell_html += generate_cell(y, b, x, name+'_%d' % b)
    return cell_html

def generate_grid():
    grid_content = generate_cell(0, 0, 0, '0')
    return grid_content

def show_page(grid_content:str):
    print(PAGE_HTML % (
        node,
        node,
        node,
        str(depth),
        str(branches),
        str(cell_width),
        str(cell_height),
        node,
        grid_content))

GEN_PAGE_HTML = '''<html>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Generating Sub-Hierarchy</title>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h1>Generating Sub-Hierarchy</h1>
</body>
%s
</html>
'''

ERROR_PAGE_HTML='''<html>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>Sub-Hierarchy Error</title>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h1>Sub-Hierarchy Error</h1>
</body>
%s
</html>
'''

def show_error_page(msg:str):
    print(ERROR_PAGE_HTML % msg)
    sys.exit(0)

content_count = 0

def add_content_to_data_map(vlist:list, content:str, data:dict):
    global content_count
    idx = vlist.pop(0)
    i = int(idx)
    if i not in data:
        data[i] = {}
    if len(vlist)==0:
        data[i]['content'] = content
        content_count += 1
    else:
        add_content_to_data_map(vlist, content, data[i])

def add_req_to_data_map(vlist:list, req:str, data:dict):
    idx = vlist.pop(0)
    i = int(idx)
    if i not in data:
        data[i] = {}
    if len(vlist)==0:
        data[i]['req'] = req
    else:
        add_req_to_data_map(vlist, req, data[i])

def add_td_to_data_map(vlist:list, td:str, data:dict):
    idx = vlist.pop(0)
    i = int(idx)
    if i not in data:
        data[i] = {}
    if len(vlist)==0:
        data[i]['td'] = td
    else:
        add_td_to_data_map(vlist, td, data[i])

def add_to_data_map(name:str, value:str, data:dict):
    try:
        vlist = name.split('_')
        if vlist[0]=='req':
            add_req_to_data_map(vlist[1:], value, data)
        elif vlist[0]=='td':
            add_td_to_data_map(vlist[1:], value, data)
        else:
            add_content_to_data_map(vlist, value, data)
    except Exception as e:
        show_error_page('Error in add_to_data_map: '+str(e))

# branches with only a req need to be pruned
# branches with only content are invalid
# branches with no content and no req are invalid
def validate_data(i:int, data:dict, parse_copy:dict)->int:
    if i not in data:
        show_error_page(f'Error missing key {i} in data: '+'<pre>'+json.dumps(data, indent=4)+'</pre>')
    data_branch = data[i]
    parse_copy_branch = parse_copy[i]
    has_content = True
    if 'content' not in data_branch:
        has_content = False
    else:
        has_content = data_branch['content'].strip()!=''
    if not has_content:
        if 'req' in data_branch:
            # prune this
            del data[i]
            return 0
        else:
            # invalid, unconnected branch
            show_error_page(f'Error unconnected branch at key {i} in data: '+'<pre>'+json.dumps(data, indent=4)+'</pre>')
    else:
        if 'req' not in data_branch:
            # invalid, missing time required
            show_error_page(f'Error missing time required at key {i} in data: '+'<pre>'+json.dumps(data, indent=4)+'</pre>')
        else:
            # valid branch
            count = 1
            # validate index keys below this
            for idx in parse_copy_branch.keys():
                if idx not in ['content', 'req', 'td']:
                    count += validate_data(idx, data_branch, parse_copy_branch)
            return count

def collect_data():
    data = {}
    #collected_data = '<table><tbody>\n'
    for field_name in form.keys():
        if field_name not in ['action', 'node']:
            field_value = form.getvalue(field_name)
            add_to_data_map(field_name, field_value, data)
            #collected_data += '<tr><td>' + field_name + '=' + field_value + '</td></tr>\n'
    #collected_data += '</tbody></table>\n'
    parse_copy = copy.deepcopy(data)
    count = validate_data(0, data, parse_copy)
    if count != content_count:
        show_error_page(f'Error differing content counts {content_count} and {count} indicate unconnected branch(es).')
    if TESTING:
        collected_data = '<pre>'+json.dumps(data, indent=4)+'</pre>'
    else:
        collected_data = data
    return collected_data

def get_node_id_from_result(result_str:str)->str:
    identifier = 'New Node: '
    id_start = result_str.find(identifier)
    if id_start < 0:
        return ''
    id_len =  len(identifier)
    id_start += id_len
    id_end = result_str.find('\n', id_start)
    if id_end < 0:
        return ''
    return result_str[id_start:id_end].strip()

nodes_created = []
validity_error = ''

def valid_superior_id(superior_id:str)->bool:
    global validity_error
    try:
        if not superior_id:
            return False
        if superior_id == '':
            return False
        if len(superior_id) < 16:
            return False
        if superior_id[14] != '.':
            return False
    except Exception as e:
        validity_error = 'Exception at superior_id tests: '+str(e)
        return False
    try:
        s = float(superior_id)
        return True
    except:
        validity_error = 'The superior_id does not have Float appearance: '+str(superior_id)
        return False

# Recursively make the Dependency Nodes of the indicated node in the
# subhierarchy.
def make_node_and_dependencies(superior_id:str, i:int, data:dict, is_top:bool)->str:
    global top_targetdate
    global nodes_created
    textfile = '/var/www/webdata/formalizer/node-text.html'
    resstr = ''
    atstep = 'prep data'
    if not is_top: # Make this node
        try:
            # *** Clear superiors NNL?
            # Specify arguments
            req_hrs = data[i]['req']
            val = 3.0
            if 'td' not in data[i]:
                targetdate = top_targetdate
            else:
                targetdate = data[i]['td']
            prop = 'unspecified'
            with open(textfile,'w') as f:
                f.write(data[i]['content'])
            # Run fzgraph to make Node
            atstep = 'call fzgraph'
            thecmd= f'./fzgraph -E STDOUT -M node -S {superior_id} -f {textfile} -H {req_hrs} -a {val:.5f} -t {targetdate} -p {prop}'
            retcode, resstr = try_subprocess_check_output(thecmd)
            if retcode != 0:
                return show_error_page(f'Error: Failed to execute {thecmd}')
            atstep = 'get ID'
            superior_id = get_node_id_from_result(resstr)
            atstep = 'validate ID'
            if not valid_superior_id(superior_id):
                show_error_page(f'Error Node creation failed or did not return Node ID. Check the dependencies hierarchy that may have been created. Validity error: '+validity_error)
            else:
                nodes_created.append(superior_id)
            resstr += thecmd + '\n'
        except Exception as e:
            show_error_page(f'Error during Node creation at index {i} with superior {superior_id} during step "{atstep}":\n{str(e)}\nThe command {thecmd} had returned:\n{str(resstr)}')
    # Recurse through dependency making
    data_branch = data[i]
    for idx in data_branch.keys():
        if idx not in ['content', 'req', 'td']:
            resstr += make_node_and_dependencies(superior_id, idx, data_branch, False)
    return resstr

def generate_subhierarchy():
    global top_targetdate
    global nodes_created
    collected_data = collect_data()
    top_targetdate = collected_data[0]['td']
    #collected_data_str = '<pre>'+json.dumps(collected_data, indent=4)+'</pre>'
    collected_data_str = '<pre>'+make_node_and_dependencies(node, 0, collected_data, is_top=True)+'</pre>'
    collected_data_str += '\n<p>Nodes created:\n<p>\n'
    for new_node in nodes_created:
        collected_data_str += f'<a class="nnl" href="/cgi-bin/fzlink.py?id={new_node}" target="_blank">'+new_node+'</a><br>\n'
    print(GEN_PAGE_HTML % collected_data_str)

if __name__ == '__main__':
    if not node:
        show_error_page('Error missing Node ID.')
    if action=='Generate':
        generate_subhierarchy()
        sys.exit(0)
    grid_content = generate_grid()
    show_page(grid_content)
    sys.exit(0)
