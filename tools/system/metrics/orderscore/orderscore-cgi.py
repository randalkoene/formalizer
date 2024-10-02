#!/usr/bin/python3
#
# Randal A. Koene, 20241001
#
# This CGI handler helps to test the new scoring for doing things
# in order or starting a timer, as documented on the System page.

# Import modules for CGI handling 
try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi
sys.stderr = sys.stdout
import json
from datetime import datetime

# Create instance of FieldStorage 
form = cgi.FieldStorage()

cgi_keys = list(form.keys())

print("Content-type:text/html\n")

todayscorefile = '/var/www/webdata/orderscore-today.json'
totalscorerecord = '/var/www/webdata/orderscore-record.json'

doing_in_order = form.getvalue('inorder')
timer_started = form.getvalue('timerstarted')
clear = form.getvalue('clear')

FAILEDTOSAVETEMPLATE = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Order Score - Failed to Save</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>fz: Order Score - Failed to Save</h3>

<p class="fail">Failed to save order score.</p>

</body>
</html>
'''

FAILEDTOSAVERECORDTEMPLATE = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Order Score - Failed to Save Record</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>fz: Order Score - Failed to Save Record</h3>

<p class="fail">Failed to save order score record.</p>

</body>
</html>
'''

SHOWTEMPLATE = '''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: Order Score</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>fz: Order Score</h3>

<table>
<tr>
<td><button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?inorder=on','_self');">Add Doing In Order</button></td><td>Score for Node activity done in order:</td><td>%s</td>
</tr>

<tr>
<td><button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?timerstarted=on','_self');">Add Setting Timer</button></td><td>Score for timers started:</td><td>%s</td>
</tr>

<tr>
<td><button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py','_self');">Refresh</button></td><td>Total score:</td><td>%s</td>
</tr>
</table>

<p>
<button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?clear=on','_self');">Clear</button>
</p>

<hr>

Formatted to copy into the Log:
<table><tbody><tr><td style="border: 1px solid;">
<p>The day's order of action score:<br>&lt;pre&gt;<br>%s<br>&lt;\\pre&gt;<br></p>
</td></tr></tbody></table>

<hr>

</body>
</html>
'''

def make_new_orderscore()->dict:
	return {
		'date': datetime.now().strftime("%Y%m%d"),
		'inorder': 0,
		'timerstarted': 0,
	}

def get_today_orderscore()->dict:
	try:
		with open(todayscorefile, 'r') as f:
			data = json.load(f)
		if data['date'] != datetime.now().strftime("%Y%m%d"):
			data = make_new_orderscore()
	except:
		data = make_new_orderscore()
	return data

def save_today_orderscore(data:dict):
	try:
		with open(todayscorefile, 'w') as f:
			json.dump(data, f)
	except:
		print(FAILEDTOSAVETEMPLATE)
		sys.exit(0)

def get_orderscore_record()->dict:
	try:
		with open(totalscorerecord, 'r') as f:
			record = json.load(f)
	except:
		record = {}
	return record

def save_orderscore_record(record:dict):
	try:
		with open(totalscorerecord, 'w') as f:
			json.dump(record, f)
	except:
		print(FAILEDTOSAVERECORDTEMPLATE)
		sys.exit(0)

def show_today_orderscore(data:dict):
	total = data['inorder'] + data['timerstarted']
	print(SHOWTEMPLATE % (data['inorder'], data['timerstarted'], total, total))

def copy_today_orderscore_to_record(data:dict):
	record = get_orderscore_record()
	record[data['date']] = ( data['inorder'], data['timerstarted'] )
	save_orderscore_record(record)

def save_and_show(data:dict):
	save_today_orderscore(data)
	copy_today_orderscore_to_record(data)
	show_today_orderscore(data)

def score_doing_in_order():
	data = get_today_orderscore()
	data['inorder'] += 10
	save_and_show(data)

def score_timer_started():
	data = get_today_orderscore()
	data['timerstarted'] += 5
	save_and_show(data)

def clear_today_orderscore():
	data = make_new_orderscore()
	save_and_show(data)

def today_orderscore():
	data = get_today_orderscore()
	show_today_orderscore(data)

if __name__ == '__main__':
	if doing_in_order:
		score_doing_in_order()
		sys.exit(0)
	if timer_started:
		score_timer_started()
		sys.exit(0)
	if clear:
		clear_today_orderscore();
		sys.exit(0)

	today_orderscore()
	sys.exit(0)
