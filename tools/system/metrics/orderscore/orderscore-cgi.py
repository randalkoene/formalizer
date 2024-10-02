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
videoviewingrecord = '/var/www/webdata/videoviewing-record.json'

doing_in_order = form.getvalue('inorder')
timer_started = form.getvalue('timerstarted')
clear = form.getvalue('clear')
recordgraph = form.getvalue('recordgraph')
videostart = form.getvalue('videostart')
videoend = form.getvalue('videoend')
videoclear = form.getvalue('videoclear')
videorecordgraph = form.getvalue('videorecordgraph')

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

<p>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py','_self');">Refresh</button>
</p>

<table>
<tr>
<td><button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?inorder=on','_self');">Add Doing In Order</button></td><td>Score for Node activity done in order:</td><td>%s</td>
</tr>

<tr>
<td><button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?timerstarted=on','_self');">Add Setting Timer</button></td><td>Score for timers started:</td><td>%s</td>
</tr>

<tr>
<td></td><td>Total score:</td><td>%s</td>
</tr>
</table>

<p>
<button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?clear=on','_self');">Clear</button>
</p>

<p>
<button class="button button1" onclick="window.open('/cgi-bin/orderscore-cgi.py?recordgraph=on','_blank');">Record Graph</button>
</p>

<hr>

Formatted to copy into the Log:
<table><tbody><tr><td style="border: 1px solid;">
<p>The day's order of action score:<br>&lt;pre&gt;<br>%s<br>&lt;\\pre&gt;<br></p>
</td></tr></tbody></table>

<hr>

<p>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py?videostart=on','_self');">Start (Non-Music) Video Viewing</button>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py?videoend=on','_self');">End (Non-Music) Video Viewing</button>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py?videoclear=on','_self');">Clear Tracking of Video Viewing</button>
</p>
<table><tbody>
<tr>
<td>Video Tracking Start:</td><td>%s</td>
</tr>
<tr>
<td>Video Hours Today:</td><td>%s</td>
</tr>
</tbody></table>

<p>
<button class="button button2" onclick="window.open('/cgi-bin/orderscore-cgi.py?videorecordgraph=on','_blank');">Video Tracking Record Graph</button>
</p>

</body>
</html>
'''

RECORDGRAPHTEMPLATE='''<html>
<head>
<meta charset="utf-8">
<link rel="icon" href="/favicon-nodes-32x32.png">
<link rel="stylesheet" href="/fz.css">
<link rel="stylesheet" href="/fzuistate.css">
<title>fz: %s</title>
</head>
<body>
<script type="text/javascript" src="/fzuistate.js"></script>
<h3>fz: %s</h3>

%s

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

def get_videoviewing_record()->dict:
	try:
		with open(videoviewingrecord, 'r') as f:
			record = json.load(f)
	except:
		record = {}
	return record

def save_videoviewing_record(record:dict):
	try:
		with open(videoviewingrecord, 'w') as f:
			json.dump(record, f)
	except:
		print(FAILEDTOSAVERECORDTEMPLATE)
		sys.exit(0)

def get_video_startstamp(data:dict)->str:
	if 'videostart' not in data:
		return ''
	return data['videostart']

def get_video_minutes(datestamp:str)->int:
	videorecord = get_videoviewing_record()
	if datestamp in videorecord:
		return videorecord[datestamp]
	return 0

def video_minutes_tracked(data:dict)->int:
	videostartstamp = get_video_startstamp(data)
	if videostartstamp == '':
		return 0
	d_start = datetime.strptime(videostartstamp, "%Y%m%d%H%M")
	d_end = datetime.now()
	time_difference = d_end - d_start
	return int(time_difference.total_seconds()/60.0)

def today_video_minutes_including_current_tracking(data:dict)->int:
	videotodayrecorded = get_video_minutes(data['date'])
	videotrackingminutes = video_minutes_tracked(data)
	return videotodayrecorded + videotrackingminutes

def show_today_orderscore(data:dict):
	total = data['inorder'] + data['timerstarted']
	videostartstamp = get_video_startstamp(data)
	videohourstoday = f"{float(today_video_minutes_including_current_tracking(data))/60.0:.2f}"
	print(SHOWTEMPLATE % (data['inorder'], data['timerstarted'], total, total, videostartstamp, videohourstoday))

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

def show_graph_of_record():
	import pandas as pd
	import plotly.graph_objects as go
	import plotly.io as pxio

	record = get_orderscore_record()

	data = {
		'Date': list(record.keys()),
		'InOrder': [ y_tuple[0] for key, y_tuple in record.items()],
		'TimerStarted': [ y_tuple[1] for key, y_tuple in record.items()],
	}

	df = pd.DataFrame(data)

	fig = go.Figure()
	fig.add_trace(go.Bar(x=df['Date'], y=df['InOrder'], name='In Order'))
	fig.add_trace(go.Bar(x=df['Date'], y=df['TimerStarted'], name='Timer Started'))

	fig.update_layout(
		barmode='stack',
	    title='Order Score Record',
	    xaxis_title='Date',
	    yaxis_title='Scores'
	)

	print(RECORDGRAPHTEMPLATE % ('Order Score Record', 'Order Score Record', pxio.to_html(fig, full_html=False)))

def record_video_viewing_interval(minutes:int):
	record = get_videoviewing_record()
	date = datetime.now().strftime("%Y%m%d")
	if date in record:
		record[date] += minutes
	else:
		record[date] = minutes
	save_videoviewing_record(record)

def track_video_viewing_start():
	data = get_today_orderscore()
	data['videostart'] = datetime.now().strftime("%Y%m%d%H%M")
	save_and_show(data)

def track_video_viewing_end():
	data = get_today_orderscore()
	minutes_tracked = video_minutes_tracked(data)
	if minutes_tracked > 0:
		record_video_viewing_interval(minutes_tracked)
		data['videostart'] = ''
	save_and_show(data)

def track_video_viewing_clear():
	data = get_today_orderscore()
	if 'videostart' in data:
		data['videostart'] = ''
	save_and_show(data)

def show_graph_of_video_tracking():
	import pandas as pd
	import plotly.graph_objects as go
	import plotly.io as pxio

	record = get_videoviewing_record()

	data = {
		'Date': list(record.keys()),
		'VideoHours': [ float(y)/60.0 for key, y in record.items()],
	}

	df = pd.DataFrame(data)

	fig = go.Figure()
	fig.add_trace(go.Bar(x=df['Date'], y=df['VideoHours'], name='Video Hours'))

	fig.update_layout(
	    title='Video Tracking Record',
	    xaxis_title='Date',
	    yaxis_title='Hours'
	)

	print(RECORDGRAPHTEMPLATE % ('Video Tracking Record', 'Video Tracking Record', pxio.to_html(fig, full_html=False)))

if __name__ == '__main__':
	if doing_in_order:
		score_doing_in_order()
		sys.exit(0)
	if timer_started:
		score_timer_started()
		sys.exit(0)
	if clear:
		clear_today_orderscore()
		sys.exit(0)
	if recordgraph:
		show_graph_of_record()
		sys.exit(0)
	if videostart:
		track_video_viewing_start()
		sys.exit(0)
	if videoend:
		track_video_viewing_end()
		sys.exit(0)
	if videoclear:
		track_video_viewing_clear()
		sys.exit(0)
	if videorecordgraph:
		show_graph_of_video_tracking()
		sys.exit(0)

	today_orderscore()
	sys.exit(0)
