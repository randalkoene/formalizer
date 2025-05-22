#!/usr/bin/python3
#
# Randal A. Koene, 20250513
#
# This script parses HTML that contains Log chunks and adds selection checkboxes with
# Log chunk IDs to easily select a subset of Log chunks for further processing, e.g. to
# extract related data and carry out processing.
#
# This script is a receiver script that can be used when calling fzloghtml
# with the -S (select and process) option.
#
# This script was made with the help of DeepSeek.

import cgi
import cgitb
import sys
from io import StringIO
from traceback import print_exc
from subprocess import Popen, PIPE

# Enable error reporting for debugging (remove in production)
cgitb.enable()

config = {
    'verbose': False,
    'logcmdcalls': False,
    'cmdlog': '/var/www/webdata/formalizer/selectchunks.log'
}

results = {}

def try_subprocess_check_output(thecmdstring, resstore):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
        print('<pre>')
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        p = Popen(thecmdstring,shell=True,stdin=PIPE,stdout=PIPE,close_fds=True, universal_newlines=True)
        (child_stdin,child_stdout) = (p.stdin, p.stdout)
        child_stdin.close()
        result = child_stdout.read()
        if resstore:
            results[resstore] = result
        child_stdout.close()
        if result and config['verbose']:
            print(result)
            print('</pre>')
        return 0

    except Exception as ex:
        print('<pre>')
        print(ex)
        f = StringIO()
        print_exc(file=f)
        a = f.getvalue().splitlines()
        for line in a:
            print(line)
        print('</pre>')
        return 1

def do_time_reporting(select_values:list):
    print("Content-Type: text/plain\n")

    csv_keys = ','.join(select_values)
    cmdstr = './fzlogdata -q -C '+csv_keys
    res = try_subprocess_check_output(cmdstr, 'data')
    if 'data' in results:
        data_list = results['data'].split('\n')
        days = {}
        for chunk_data_str in data_list:
            if len(chunk_data_str)>0:
                chunk_data = chunk_data_str.split(':')
                timestr = chunk_data[0]
                minsstr = chunk_data[1]
                daystr = timestr[0:8]
                if daystr not in days:
                    days[daystr] = []
                days[daystr].append(int(minsstr))
        print('Selected Log chunks:\n')
        print(str(select_values))
        print('\nMinutes in Log chunks each day and their sum:\n')
        for daystr in days:
            print('%s: sum(%s) = %d mins' % (daystr, str(days[daystr]), sum(days[daystr])))
        print('\nDays and sums of minutes:\n')
        total_minutes = 0
        for daystr in days:
            sum_minutes = sum(days[daystr])
            total_minutes += sum_minutes
            print('%s, %d' % (daystr, sum_minutes))
        print('\nTotal minutes:\n')
        print('%d' % total_minutes)
        print('\nTotal hours:\n')
        print('%.2f' % (float(total_minutes)/60.0))
    else:
        print('Missing data.')

SELECTED_DISPLAY_HEAD='''<html>
<head>
<meta charset="utf-8" />
<link rel="stylesheet" href="/fz.css">
<link rel="icon" href="/favicon-logentry-32x32.png">
<link rel="stylesheet" href="/fzuistate.css">
<link rel="stylesheet" href="/clock.css">
<link rel="stylesheet" href="/score.css">
<link rel="stylesheet" href="/copiedalert.css">
<link rel="stylesheet" href="/htmltemplatestocopy.css">
<link rel="stylesheet" href="/tooltip.css">
<meta http-equiv="cache-control" content="no-cache" />
<title>fz: Selected Log chunks</title>
<style>
.logstate {
position: fixed;
top: 120px;
right: 0px;
font-size: 34px;
font-family: calibri;
}
#protocol_tab {
position: fixed;
top: 185px;
right: 0px;
width: 300px;
height: 500px;
display: block;
text-align: right;
}
.prot_tip {
width: 300px;
top: 100%;
right: 50%;
}
</style>
</head>
<body>
<script type="text/javascript" src="/clock.js"></script>
<script type="text/javascript" src="/score.js"></script>
<script type="text/javascript" src="/fzuistate.js"></script>
<button id="timerBarText" class="button button2 logstate">_____</button>

<div id="protocol_tab">
<button class="button button1" onclick="window.open('/cgi-bin/schedule-cgi.py?c=true&num_days=7&s=20', '_blank');">Calendar Schedule</button><br>
<span class="alt_tooltip">Distractions off <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Turn off video distractions.</div></span>
</span><br>
<span class="alt_tooltip">Brush away emotion <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Literally, brush away emotions, especially anxiety.</div></span>
</span><br>
<span class="alt_tooltip">Just get started <input type="checkbox">
<span class="alt_tooltiptext prot_tip"><div>Just get started on a Node, you don't need to complete it right away, you just need to figure out what to do and take notes about that to get started.</div></span>
</span><br>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py', '_blank');">OrderScore</button><br>
<button class="button button1" onclick="window.open('/cgi-bin/fzloghtml-cgi.py?review=today', '_blank');">Today Review</button><br>
<button class="button button2" onclick="window.open('/cgi-bin/nodeboard-cgi.py?D=week_main_goals&T=true&u=204512311159&r=100&U=true', '_blank');">Week Goals</button>
</div>

<script type="text/javascript" src="/stateoflog.js"></script>
<script type="text/javascript" src="/hoveropentab.js"></script>
<script>
const state_of_log = new logState(logstate_id="timerBarText");
const protocol_tab = new HoverOpenTab('timerBarText', 'protocol_tab');
</script>
'''

SELECTED_DISPLAY_TAIL='''
<p>Enter HTML text here to append a Log Entry:
<form action="/cgi-bin/logentry-form.py" method="post"><input type="hidden" name="showrecent" value="on">
<textarea id="entrytext" rows="10" cols="100" name="entrytext"></textarea><br>
Add entry for <input type="submit" name="makeentry" value="Log Chunk Node" /> or <input type="submit" name="makeentry" value="Other Node" /> | <input type="submit" name="makeentry" value="Templates" /> | [<a href="/cgi-bin/metrictags.py" target="_blank">Show Metric Tags</a>].
</form>
</p>

<div id="after_cptplt"></div>
<div id="templates_html" style="display:none;">
<button class="button2" onclick="copyInnerHTMLToClipboard('chkbx_tpl');">copy</button> <span id="chkbx_tpl"><input type="checkbox" ></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnkblank_tpl');">copy</button> <span id="lnkblank_tpl"><a href="URL" class="docpopupfunc" target="_blank">target</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocaldoc_tpl');">copy</button> <span id="lnklocaldoc_tpl"><a href="@FZSERVER@/doc/" target="_blank">~/doc/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('lnklocalwww_tpl');">copy</button> <span id="lnklocalwww_tpl"><a href="@FZSERVER@/" target="_blank">/var/www/html/path</a></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('img_tpl');">copy</button> <span id="img_tpl"><img class="hoverdelayfunc" src="/favicon.png" width=32px height=32px></span>
<button class="button2" onclick="copyInnerHTMLToClipboard('acttype_tpl');">copy</button> <span id="acttype_tpl">@ACTTYPE=A100@</span>
<p></p>
</div>
<script type="text/javascript" src="/htmltemplatestocopy.js"></script>
<script>const htmltemplates = new htmlTemplates('after_cptplt', 'cptplt', 'templates', 'templates_html');</script>
<script type="text/javascript" src="/copiedalert.js"></script>
<script type="text/javascript" src="/copyinnerhtml.js"></script>

<hr>

Select another part of the Log:
<form action="/cgi-bin/fzloghtml-cgi.py" method="post">
<input type="checkbox" name="alloflog"> all of the Log<br />
<input type="text" name="startfrom"> start at this YYYYmmddHHMM time stamp (default: 24 hours before end of interval)<br />
<input type="text" name="endbefore"> end before this YYYYmmddHHMM time stamp (default: 1 second after most recent Log entry)<br />
<input type="text" name="daysinterval"> interval size in days<br />
<input type="text" name="weeksinterval"> interval size in weeks<br />
<input type="text" name="hoursinterval"> interval size in hours<br />
<input type="text" name="numchunks"> number of Log chunks (interval takes precedence)<br />
<input type="checkbox" name="frommostrecent"> from most recent (interval takes precedence)<br />
<input type="text" name="node"> belongs to node, node history<br />
<input type="text" name="topic"> belongs to topic<br />
<input type="text" name="nnl"> belongs to NNL<br />
<input type="text" name="searchtext" size="48"> must contain search text
<input type="checkbox" name="caseinsensitive"> case insensitive
<input type="checkbox" name="andall"> must contain each<br />
<input type="text" name="regex" size="48"> must contain RegEx<br />
<input type="text" name="btf"> must belong to BTF category<br />
or<br />
<input type="checkbox" name="mostrecentdata"> show most recent Log data summary<br />
<input type="checkbox" name="selectchunks"> enable selecting of Log chunks<br />

<input type="submit" value="Submit" />
</form>

<hr>

<p>[<a href="/index.html">fz: Top</a>] <span id="logautoupdate">_</span></p>

<script type="text/javascript" src="/delayedpopup.js"></script>
<script>
set_hover_delayed_function('.hoverdelayfunc', enlargeImage, 1000);
set_hover_delayed_function('.docpopupfunc', openPopup, 1000);
pastePopupLink('docpopupfunc', 'entrytext');
</script>
<script type="text/javascript" src="/logautoupdate.js"></script>
<script>
const global_autologupdate = new autoLogUpdate('logautoupdate', true, 'entrytext');
</script>
<!-- <script type="text/javascript" src="/logcheckbox.js"></script> -->
'''

def do_selected_display(select_values:list):
    print("Content-Type: text/html\n")
    print(SELECTED_DISPLAY_HEAD)
    print('<h1>Formalizer: Selected Log chunks</h1>\n<p></p>\n<table id="LogInterval"><tbody>')
    for selected_chunk in select_values:
        cmdstr = './fzloghtml -q -d formalizer -s randalk -e '+selected_chunk+' -N -F html -o STDOUT -E STDOUT'
        res = try_subprocess_check_output(cmdstr, selected_chunk)
        if selected_chunk in results:
            print(str(results[selected_chunk]))
        else:
            print('Missing content for Log chunk '+str(selected_chunk))
    print('</tbody></table>')
    print('<a name="END">&nbsp;</a>')
    print(SELECTED_DISPLAY_TAIL)
    print("</body>\n</html>")

def main():
    try:
        # Parse the form data
        form = cgi.FieldStorage()
        
        time_reporting = form.getvalue('time_reporting')
        selected_display = form.getvalue('selected_display')

        # Initialize list to store select values
        select_values = []
        
        # Check if the form was submitted with POST
        if sys.stdin.isatty():
            print("Content-Type: text/plain\n")
            print("Error: This script should be called with POST method.")
            return
        
        # Get all 'select' parameters (handles multiple values for the same name)
        if 'select' in form:
            # FieldStorage returns all values for a parameter as a list
            select_params = form.getlist('select')
            select_values.extend(select_params)
        else:
            print("Content-Type: text/plain\n")
            print('No selected Log chunks.')
            return
        
        # Print the collected values for demonstration
        #print("Collected 'select' values:")
        #for value in select_values:
        #    print(f"- {value}")

        if time_reporting:
            do_time_reporting(select_values)

        elif selected_display:
            do_selected_display(select_values)
        
    except Exception as e:
        print("Content-Type: text/plain\n")
        print(f"An error occurred: {str(e)}")

if __name__ == "__main__":
    main()
