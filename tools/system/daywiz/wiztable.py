# wiztable.py
# Copyright 2023 Randal A. Koene
# License TBD
#
# WIZTABLE data.

# ====================== Table row content template(s):

# full: [ node, weight, id, time.time(), type, hr_ideal_from, hr_ideal_to, description, state, ]
#       weight is 1 to 5, weights are often given in accordance with discipline challenge or critical importance
# stored in JSON: [ id, time.time(), state, ]
# NOTE: Horizontal lines are drawn using the WIZLINE_VISIBLE_TOPBORDER code, demarkating transitions
#       to items with a different Node link.
WIZTABLE_LINES=[
	[ '20060914084328.1', 5, 'log', 0, 'checkbox', 6, 8, 'Update (or catch up) the Log.', '' ], # Log autodetectable.
	[ '20060914084328.1', 1, 'ritual', 0, 'checkbox', 6, 8, 'Ritual: Meditate and/or do yoga (preferably outside).', '' ],
	[ '20060914084328.1', 1, 'coffee', 0, 'checkbox', 6, 8, 'Start timer and make coffee.', '' ],
	[ '20060914084328.1', 2, 'vitamins', 0, 'checkbox', 6, 8, 'Take vitamins and supplements.', '' ],
	[ '20060914084328.1', 3, 'news', 0, 'checkbox', 6, 8, 'Use <a href="/formalizer/test_maketimer.html" target="_blank">a timer or skip news reading.', '' ],

	[ '20091115180507.1', 2, 'nextnutri', 0, 'checkbox', 6, 9, 'Plan nutrition for the next day.', '' ],
	[ '20091115180507.1', 5, 'calsync', 0, 'checkbox', 6, 9, 'Sync <a href="https://calendar.google.com/calendar/?authuser=randal.a.koene@gmail.com" target="_blank">Google Calendar</a> with Schedule.', '' ], # Automatable.
	[ '20091115180507.1', 3, 'promises', 0, 'checkbox', 6, 9, 'Review <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=promises&sort_by=targetdate" target="_blank"><b>Promises</b></a> (<a href="https://trello.com/b/KsIljqx3/promises">and Board</a>), update, revise and re-prioritize as needed. Make sure Scheduling is correct.', '' ],
	[ '20091115180507.1', 5, 'passedfixed', 0, 'checkbox', 6, 9, '<a href="/cgi-bin/fzupdate-cgi.py?update=passedfixed" target="_blank">Confirm passed non-repeating Fixed Nodes to convert to Variable</a> - or - manually update them.', '' ],
	[ '20091115180507.1', 5, 'passedrepeat', 0, 'checkbox', 6, 9, 'Auto-update passed repeating Nodes to their next instances.', '' ],
	[ '20091115180507.1', 3, 'priority', 0, 'checkbox', 6, 9, 'Ensure that highest priority <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=promises" target="_blank">Promises are represented in the Schedule</a>.', '' ], # Schedule autodetectable.
	[ '20091115180507.1', 4, 'milestones', 0, 'checkbox', 6, 9, 'Review upcoming Milestones (<a href="/cgi-bin/nodeboard-cgi.py?n=20101230042609.1" target="_blank">Top Milestones Kanban</a>, <a href="/cgi-bin/nodeboard-cgi.py?f=main2023" target="_blank">Active Main Categories</a>, <a href="/cgi-bin/nodeboard-cgi.py?n=20101230042609.1&F=%5B" target="_blank">Milestone Categories</a>, mapped to Milestones, Dependencies, Roadmap Path, Values and Urgency.) Ensure there are Nodes to complete upcoming Milestones. Also, review progress on current main Threads side-by-side (in <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=threads" target="_blank">Schedule</a>, in <a href="/cgi-bin/nodeboard-cgi.py?D=threads&I=true&T=true" target="_blank">Kanban</a>).', '' ],
	[ '20091115180507.1', 4, 'decisions', 0, 'checkbox', 6, 9, 'Identify <a href="/cgi-bin/fzgraphhtml-cgi.py?subtrees=challenges_open" target="_blank">Challenges and Decisions in the Schedule</a>, tag as such, add to <a href="/cgi-bin/fzgraphhtml-cgi.py?srclist=challenges_open&sort_by=targetdate" target="_blank">NNL</a>. <a href="https://trello.com/b/mnu1VKIn" target="_blank">Known Challenges and Decisions</a> must be represented by at least one Node.', '' ],
	[ '20091115180507.1', 5, 'schedfits', 0, 'checkbox', 6, 9, 'Ensure schedule is mathematically doable. (Sum of required times fits into available time.)', '' ],
	[ '20091115180507.1', 2, 'variable', 0, 'checkbox', 6, 9, 'Auto-update Variable Nodes. (Or replace this with different handling of Variable Nodes.)', '' ],
	[ '20091115180507.1', 5, 'realistic', 0, 'checkbox', 6, 9, 'Ensure Schedule is realistic. E.g. map to time intervals on calendar day. Account for mini-days, and Nodes best in morning/evening.', '' ], # Automatable.
	[ '20091115180507.1', 5, 'backup', 0, 'checkbox', 6, 9, 'Make sure that Formalizer database backups are mirrored to separate storage (USB drive, mirrored account, cloud storage). E.g., run <button class="button button1" onclick="window.open(\'/cgi-bin/fzbackup-from-web-cgi.py\',\'_blank\');">fzbackup-mirror-to-github.sh</button>.', '' ], # Automatable.
	[ '20091115180507.1', 5, 'relations', 0, 'checkbox', 6, 9, 'Prepare the list of people that I already know I should communicate with this day.', '' ],

	[ '20211017053846.1', 4, 'weight', 0, 'number', 6, 10, 'Measure <a href="/cgi-bin/metrics.py?cmd=show&selectors=wiztable" target="_blank">weight</a>.', '' ],
	[ '20211017053846.1', 4, 'pushup1', 0, 'checkbox', 6, 10, 'First push-ups or weights (e.g. before/after shower).', '' ],
	[ '20211017053846.1', 2, 'shower', 0, 'checkbox', 6, 10, 'Have a shower.', '' ],
	[ '20211017053846.1', 3, 'lotion', 0, 'checkbox', 6, 10, 'Put on lotion.', '' ],
	[ '20211017053846.1', 4, 'teeth', 0, 'checkbox', 6, 10, 'Brush teeth and/or chew gum.', '' ],

	[ '20140428114648.1', 2, 'neoprene', 0, 'checkbox', 6, 10, 'Wearing clothing where bloat/size is detectable (or neoprene).', '' ],
	[ '20140428114648.1', 4, 'pushup2', 0, 'checkbox', 6, 10, 'Second push-ups or weights (e.g. during lunch).', '' ],

	[ '20140921041431.1', 5, 'startedearly', 0, 'checkbox', 6, 10, 'Started early, at least an hour into priority intentions by 10 am.', ''],
	[ '20140921041431.1', 4, 'lessthan300', 0, 'checkbox', 6, 12, 'Had less than 300 calories in the morning.', ''],
	[ '20140921041431.1', 3, 'morningfast', 0, 'checkbox', 6, 12, 'Did not eat in the morning.', ''],

	[ '20040402045825.1', 5, 'firstwork', 0, 'checkbox', 13, 16, 'Managed at least 4 hours of actual work progress in the first miniday.', ''],

	[ '20081125102516.1', 5, 'emailparsed', 0, 'number', 8, 20, 'Emails parsed.', '' ],
	[ '20081125102516.1', 5, 'emailurgent', 0, 'checkbox', 8, 20, 'Urgent emails identified.', '' ],
	[ '20081125102516.1', 5, 'emailresp', 0, 'number', 8, 20, 'Emails responded to.', '' ],

	[ '20100403134619.1', 4, 'endurance', 0, 'checkbox', 9, 22, 'Endurance exercise (e.g rowing, dancing, rollerblading).', '' ],
	[ '20100403134619.1', 4, 'pushup3', 0, 'checkbox', 20, 24, 'Third push-ups or weights (e.g. during dinner).', '' ],
	[ '20100403134619.1', 4, 'weight2', 0, 'checkbox', 19, 24, 'Weighed myself (again) in the evening.', '' ],

	[ '20200601093905.1', 5, 'acctcomplete', 0, 'checkbox', 20, 24, 'Accounting of the day completed.', '' ],
	[ '20200601093905.1', 5, 'secondwork', 0, 'checkbox', 20, 24, 'Managed at least 4 hours of actual work progress in the second miniday.', '' ],

	[ '20230727204001.1', 5, 'lovedbody', 0, 'checkbox', 20, 24, 'Loved my body.', '' ],
	[ '20230727204001.1', 5, 'lovedmind', 0, 'checkbox', 20, 24, 'Loved my mind.', '' ],

	[ '20200601093905.1', 3, 'armodafinil', 0, 'number', 21, 24, 'Armodafinil taken (mg).', '' ],
	[ '20200601093905.1', 3, 'alcohol', 0, 'number', 21, 24, 'Alcohol consumed (ml).', '' ],
	[ '20200601093905.1', 4, 'daynutri', 0, 'checkbox', 22, 24, 'Finalize day nutrition.', '' ],
	[ '20200601093905.1', 4, 'fastafter8', 0, 'checkbox', 22, 24, 'Did not eat after 8pm.', '' ],
	[ '20200601093905.1', 5, 'challenge', 0, 'checkbox', 22, 24, 'Addressed at least 1 challenge.', '' ],
	[ '20200601093905.1', 4, 'timeuse', 0, 'checkbox', 22, 24, 'Review how I <a href="/cgi-bin/sysmet-extract-cgi.py?categories=new2023" target="_blank"><b>used my time</b></a>.', '' ],
]
WIZTABLE_LINES_NODE=0
WIZTABLE_LINES_WEIGHT=1
WIZTABLE_LINES_ITEM=2
WIZTABLE_LINES_TIME=3
WIZTABLE_LINES_TYPE=4
WIZTABLE_LINES_HRFROM=5
WIZTABLE_LINES_HRTO=6
WIZTABLE_LINES_DESC=7
WIZTABLE_LINES_STATE=8

WIZTABLE_LINE_LENGTH=len(WIZTABLE_LINES[0])