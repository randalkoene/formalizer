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

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

node = form.getvalue('node')
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

ENTRY_DIV = '''<div style="position:absolute; top: %s; left: %s; width: %s; height: %s;">
<textarea style='width:100%%; resize: none;' rows='%s' name='%s' id='%s'>
%s
</textarea>
</div>
'''

PAGE_HTML = '''<html>
<body>
<div style='position:relative;'>
%s
</div>
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
        errorstr = f'Subprocess call: {str(cpe.cmd)}\n'+errorstr+'\n'
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
        return 0, res

def get_node_content()->str:
    thecmd = f'./fzgraphhtml -q -o STDOUT -F json -n {node}'
    retcode, res = try_subprocess_check_output(thecmd)
    if retcode != 0:
        return f'Error: Failed to execute {thecmd}'
    try:
        data = json.loads(res)
        return data['node-text']
    except Exception as e:
        return f'Error: {str(e)}'

'''
depth=4
d = depth - 1 = 3
maxw = 2^d = 8
0th level: y=0, 1                [prevx=0]
1st level: y=1, 2, 0 & 4         [prevx=b*maxw/(2^y),                                                                 prevx=b*maxw/(2^y)=4]
2nd level: y=2, 4, 0 & 2 & 4 & 6 [prevx=b*maxw/(2^y)=0,                   prevx=b*maxw/(2^y)=2,                       prevx=prevx+b*maxw/(2^y)=4,                 prevx=prevx+b*maxw/(2^y)=6]
3rd level: y=3, 8,               [prevx+b*maxw/(2^y)=0, prevx+b*maxw/8=1, prevx+b*maxw/(2^y)=2, prevx+b*maxw/(2^y)=3, prevx+b*maxw/(2^y)=4, prevx+b*maxw/(2^y)=5, ...]
'''

def generate_cell(y:int, b:int, prev_x:int, name:str):
    divisor = branches**y
    multi = max_width/divisor
    x = prev_x + (b*multi)
    i = (cell_width+2)*x
    j = (cell_height+1)*y
    if b==0 and y==0:
        cellvalue = get_node_content()
    else:
        #cellvalue = ''
        cellvalue = name # *** Temporary for testing
    cell_html = ENTRY_DIV % ('%dem' % j, '%dem' % i, '%dem' % (cell_width*multi+1), '%dem' % cell_height, str(cell_height), name, name, cellvalue)
    y += 1
    if y < depth:
        for b in range(branches):
            cell_html += generate_cell(y, b, x, name+'_%d' % b)
    return cell_html

def generate_grid():
    grid_content = generate_cell(0, 0, 0, '0')
    return grid_content

def show_page(grid_content:str):
    print(PAGE_HTML % grid_content)

if __name__ == '__main__':
    grid_content = generate_grid()
    show_page(grid_content)
    sys.exit(0)
