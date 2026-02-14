#!/usr/bin/python3
#
# Randal A. Koene, 20230326
#
# This CGI handler displays nodeboard output in an HTML page.
#

# Do this immediately, so that any errors are visible:
print("Content-type:text/html\n\n")

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
from os.path import exists
sys.stderr = sys.stdout
from time import strftime
import datetime
import traceback
from json import loads, load
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE
from pathlib import Path
from datetime import datetime, timedelta

from fzmodbase import *
from fzbgprogress import *

home = str(Path.home())

# *** THESE SHOULD BE SET BY SETUP CONFIGURATION
# *** Perhaps read the following from ~/.formalizer/webdata_path
webdata_path = "/var/www/webdata/formalizer"

intentionscategoriesfile = "/var/www/webdata/formalizer/categories_a2c.json"
workcategoriesfile = "/var/www/webdata/formalizer/categories_work.json"
main2023categoriesfile = "/var/www/webdata/formalizer/categories_main2023.json"
ccfresponsibilitiesfile = "/var/www/webdata/formalizer/ccf_responsibilities.json"

logfile = webdata_path + '/nodeboard-cgi.log'

# HTML_HEAD='''<?xml version="1.0" encoding="UTF-8" ?>
# <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
# <html xmlns="http://www.w3.org/1999/xhtml">
# <head>
# <meta http-equiv="Content-Type" content="application/xml+xhtml; charset=UTF-8"/>
# <title>System Metrics - Extract</title>
# </head>
# <body style="color:white; background-color:black">
# <pre>
# '''

# HTML_TAIL='''</pre>
# </body>
# </html>
# '''

# cgitb.enable()
# cgitb.disable()
# cgitb.enable(display=0, logdir="/tmp/test_python_cgiformget.log")

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
sysmet_file = form.getvalue('f')
node_dependencies = form.getvalue('n')
node_dependencies_tree = form.getvalue('G')
node_superiors_tree = form.getvalue('g')
filter_string = form.getvalue('F')
filter_topics = form.getvalue('i')
show_completed = form.getvalue('I')
show_zero_required = form.getvalue('Z')
subtree_list = form.getvalue('D')
threads = form.getvalue('T')
progress_analysis = form.getvalue('P')
calendar_file = form.getvalue('c')
header_arg = form.getvalue('H')
highlight_topic = form.getvalue('B')
vertical_multiplier = form.getvalue('M')
max_columns = form.getvalue('C')
max_rows = form.getvalue('r')
days_near_highlight = form.getvalue('N')
tdorder = form.getvalue('tdorder')
tdbad = form.getvalue('tdbad')
tdfar = form.getvalue('tdfar')
subtreesort = form.getvalue('K')
nnldepstotdate = form.getvalue('u')

nnlweeksahead = form.getvalue('nnlweeksahead')
if nnlweeksahead:
    current_time = datetime.now()
    weeks_later = timedelta(weeks=int(nnlweeksahead))
    future_time = current_time + weeks_later
    nnldepstotdate = future_time.strftime('%Y%m%d%H%M')

proposetdsolutions = form.getvalue('O')
norepeated = form.getvalue('U')
dodevelopmenttest = form.getvalue('R')
timeline = form.getvalue('z')
timeline_stretch = form.getvalue('zstretch')
importance_threshold = form.getvalue('w')

include_td_detect = ''
if tdorder:
    include_td_detect += 'tdorder,'
if tdbad:
    include_td_detect += 'tdbad,'
if tdfar:
    include_td_detect += 'tdfar,'
if include_td_detect != '':
    include_td_detect = ' -e "'+include_td_detect[:-1]+'"'
if filter_string:
    if filter_string != '':
        include_filter_string = ' -F %s' % filter_string
    else:
        include_filter_string = ''
else:
    include_filter_string = ''
if filter_topics:
    if filter_topics != '':
        include_filter_topics = ' -i %s' % filter_topics
    else:
        include_filter_topics = ''
else:
    include_filter_topics = ''
if show_completed == 'true':
    include_show_completed = ' -I'
else:
    include_show_completed = ''
if show_zero_required == 'true':
    include_show_zero_required = ' -Z'
else:
    include_show_zero_required = ''
if importance_threshold:
    include_importance_threshold = ' -w %s' % importance_threshold
else:
    include_importance_threshold = ''
if threads:
    include_threads = ' -T'
else:
    include_threads = ''
if progress_analysis:
    include_progress_analysis = ' -P'
else:
    include_progress_analysis = ''
if timeline:
    if not timeline_stretch:
        timeline_stretch = '1.0'
    include_timeline = ' -z '+timeline_stretch
else:
    include_timeline = ''
if highlight_topic:
    if highlight_topic != '':
        include_highlight_topic = ' -B %s' % highlight_topic
    else:
        include_highlight_topic = ''
else:
    include_highlight_topic = ''
if max_rows:
    include_max_rows = ' -r %s' % max_rows
else:
    include_max_rows = ''
if days_near_highlight:
    if days_near_highlight != '0.0':
        include_days_near_highlight = ' -N %s' % days_near_highlight
    else:
        include_days_near_highlight = ''
else:
    include_days_near_highlight = ''
if max_columns:
    include_max_columns = ' -C %s' % max_columns
else:
    include_max_columns = ''
if subtreesort:
    include_subtree_sort = ' -K'
else:
    include_subtree_sort = ''
if nnldepstotdate:
    include_nnldepstotdate = ' -u %s' % nnldepstotdate
else:
    include_nnldepstotdate = ''
if proposetdsolutions:
    if proposetdsolutions=='earlier' or proposetdsolutions=='later':
        include_proposetdsolutions = ' -O %s' % proposetdsolutions
    else:
        include_proposetdsolutions = ''
else:
    include_proposetdsolutions = ''
if norepeated:
    include_norepeated = ' -U'
else:
    include_norepeated = ''
if dodevelopmenttest:
    include_dodevelopmenttest = ' -R'
else:
    include_dodevelopmenttest = ''

# *** OBTAIN THIS SOMEHOW!
#with open('./server_address','r') as f:
#    fzserverpq_addrport = f.read()

def log(logstr):
    with open(logfile, 'w') as f:
        f.write(logstr)

# Returns (bool:success, str:result).
def try_command_call(thecmd, print_result=True)->tuple:
    try:
        log(thecmd)
    except:
        pass
    try:
        #print(thecmd, flush=True)
        p = Popen(thecmd,shell=True,stdin=PIPE,stdout=PIPE,stderr=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout,child_stderr) = (p.stdin, p.stdout, p.stderr)
        child_stdin.close()
        result = child_stdout.read()
        error = child_stderr.read()
        child_stdout.close()
        child_stderr.close()
        no_error = len(error)<1
        if not no_error:
            print(error)
        if print_result:
            print(result)
            return (no_error, '')
        #print(result.replace('\n', '<BR>'))
        return (no_error, result)

    except Exception as ex:                
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        return (False, '')

REDIRECT='''
<html>
<meta http-equiv="Refresh" content="0; url='/formalizer/data%s'" />
</html>
'''

TEST='''
<html>
<body>
Output was: %s
Click here: <a href="/formalizer/data%s">%s</a>
</body>
</html>
'''

NODE_DEPENDENCIES_BOARD_ERROR='''<html>
<body>
<b>nodeboard command returned error</b>
<P>
Command was: %s
</body>
</html>
'''

def show_sysmet_board(sysmet_json_path:str, sysmet_output_path:str):
    thecmd = f"./nodeboard -f {sysmet_json_path} {include_filter_string} {include_filter_topics} {include_show_completed} -q -o /var/www/webdata/formalizer{sysmet_output_path}"
    #thecmd = f"./nodeboard -f {sysmet_json_path} -o STDOUT"
    success, res = try_command_call(thecmd, print_result=False)
    #print(res)
    print(REDIRECT % sysmet_output_path)
    #print(TEST % (res, sysmet_output_path, sysmet_output_path))

def show_main2023_board():
    show_sysmet_board(main2023categoriesfile, '/main2023categories-kanban.html')

def show_ccfresponsibilities_board():
    show_sysmet_board(ccfresponsibilitiesfile, '/ccfresponsibilities-kanban.html')

def show_node_dependencies_board():
    # Note: We send the following to a static output page in case there are
    #       uses where that is efficient. Nevertheless, we return the contents
    #       on STDOUT for ease of regenerating by reloading.
    outpath = '/var/www/webdata/formalizer/node_dependencies_kanban.html'
    if node_dependencies_tree:
        thecmd = f"./nodeboard -G -n {node_dependencies} {include_dodevelopmenttest} {include_filter_string} {include_filter_topics} {include_show_completed} {include_show_zero_required} {include_importance_threshold} {include_threads} {include_norepeated} {include_progress_analysis} {include_proposetdsolutions} {include_td_detect} {include_subtree_sort} {include_highlight_topic} {include_max_columns} {include_max_rows} {include_days_near_highlight} {include_timeline} -q -o {outpath}"
    elif node_superiors_tree:
        thecmd = f"./nodeboard -g -n {node_dependencies} {include_dodevelopmenttest} {include_filter_string} {include_filter_topics} {include_show_completed} {include_show_zero_required} {include_importance_threshold} {include_threads} {include_norepeated} {include_progress_analysis} {include_proposetdsolutions} {include_td_detect} {include_subtree_sort} {include_highlight_topic} {include_max_columns} {include_max_rows} {include_days_near_highlight} {include_timeline} -q -o {outpath}"
    else:
        thecmd = f"./nodeboard -n {node_dependencies} {include_filter_string} {include_filter_topics} {include_show_completed} {include_show_zero_required} -q -o {outpath}"
    success, res = try_command_call(thecmd, print_result=False)
    #print(REDIRECT % "/node_dependencies_kanban.html")
    if success:
        try:
            with open(outpath, 'r') as f:
                outcontent = f.read()
            print(outcontent)
        except Exception as e:
            print(str(e))
    else:
        print(NODE_DEPENDENCIES_BOARD_ERROR % thecmd)

ANALYSIS_PROGRESS_PAGE = '''
<html>
<body>
<h3>Running progress analysis...</h3>
%s
<script>
%s

progressloop();

</script>
</body>
</html>
'''

SUBTREE_BOARD_ERROR='''<html>
<body>
<b>nodeboard command returned error</b>
<P>
Command was: %s
</body>
</html>
'''

# If I want this to show progress:
# 1. Launch it in the background with >logfile 2>&1 &.
# 2. Print the progress page (see test_progress_indicator FZ method).
# 3. In nodeboard, use the -p /var/www/webdata/formalizer/nodeboard-progress.state option
#    and have nodeboard update that file as it works through cards.
# 4. The done-page will be  /formalizer/data/subtree_list_kanban.html.
# Note: We send the following to a static output page in case there are
#       uses where that is efficient. Nevertheless, we return the contents
#       on STDOUT for ease of regenerating by reloading.
#       *** Right now, we do this only for the simple subtree list, not yet
#           for the progress analysis results.
def show_subtree_board():
    nodeboard_logfile = '/var/www/webdata/formalizer/nodeboard.log'
    progress_state_file = 'nodeboard_progress.state'
    progress_state_path = '/var/www/webdata/formalizer/%s' % progress_state_file
    result_file = 'subtree_list_kanban.html'
    result_page_path = '/var/www/webdata/formalizer/%s' % result_file
    result_page_url = '/formalizer/data/%s' % result_file
    if progress_analysis:
        thecmd = f"./nodeboard -D {subtree_list} {include_dodevelopmenttest} {include_threads} {include_norepeated} {include_nnldepstotdate} {include_max_rows} {include_days_near_highlight} {include_progress_analysis} {include_proposetdsolutions} {include_show_completed} {include_show_zero_required} {include_timeline} -p {progress_state_path} -q -o {result_page_path} >{nodeboard_logfile} 2>&1 &"
        success, res = try_command_call(thecmd, print_result=False)
        embed_in_html, embed_in_script = make_background_progress_monitor(progress_state_file, result_page_url)
        print(ANALYSIS_PROGRESS_PAGE % (embed_in_html, embed_in_script))
    else:
        thecmd = f"./nodeboard -D {subtree_list} {include_dodevelopmenttest} {include_threads} {include_norepeated} {include_nnldepstotdate} {include_max_rows} {include_days_near_highlight} {include_proposetdsolutions} {include_show_completed} {include_show_zero_required} -q -o {result_page_path}"
        success, res = try_command_call(thecmd, print_result=False)
        #print(REDIRECT % "/subtree_list_kanban.html")
        if success:
            try:
                with open(result_page_path, 'r') as f:
                    outcontent = f.read()
                print(outcontent)
            except Exception as e:
                print(str(e))
        else:
            print(SUBTREE_BOARD_ERROR % thecmd)

def show_calendar_schedule_board():
    if vertical_multiplier:
        vertical_multiplier_arg = ' -M %s' % vertical_multiplier
    else:
        vertical_multiplier_arg = ''
    if header_arg:
        header_arg_str = " -H '%s'" % header_arg
    thecmd = f"./nodeboard -c '{calendar_file}'{header_arg_str}{vertical_multiplier_arg} -q -o STDOUT"
    success, res = try_command_call(thecmd, print_result=True)

HELP='''
<html>
<body>
The nodeboard-cgi.py script received no recognized form data.
</body>
</html>
'''

def show_help():
    print(HELP)

if __name__ == '__main__':
    if (sysmet_file == 'main2023'):
        show_main2023_board()
        sys.exit(0)

    if (sysmet_file == 'ccfresponsibilities'):
        show_ccfresponsibilities_board()
        sys.exit(0)

    if subtree_list:
        show_subtree_board()
        sys.exit(0)

    if node_dependencies: #(node_dependencies != ''):
        show_node_dependencies_board()
        sys.exit(0)

    if calendar_file:
        show_calendar_schedule_board()
        sys.exit(0)

    show_help()
    sys.exit(0)
