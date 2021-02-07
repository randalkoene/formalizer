#!/bin/bash
#
# panes-term.sh
#
# Randal A. Koene, 2021-01-18

FORMALIZERDOTDIR=$HOME/.formalizer
FORMALIZERBASEDIR=$HOME/src/formalizer


if [ "$1" = "-h" ]; then
	echo "Usage: panes-term.sh [-h|-R]"
	echo ""
	echo "Uses w3m to open 4 panes in X terminals displaying the most"
	echo "commonly used interactive and informational web pages for"
	echo "daily Formalizer use: Next Nodes in the Schedule, most"
	echo "Recent Log chunks and entries, a day's time Logged and"
	echo "upcoming in 5 minute intervals, and an Index to System"
	echo "and Formalizer pages."
	echo ""
	echo "Options:"
	echo ""
	echo "  -R  Refresh panes."
	echo "  -h  Show this help page."
	echo ""
	echo "This script auto-detects desktop size and calls the"
	echo "monitor.sh script to adjust font sizes accordingly."
	echo "This script also uses the graph-resident tool to detect"
	echo "if the Graph is memory-resident and fzserverpq is listening."
	echo ""
	exit
fi

notifycmd='zenity'
notifyarg='--notification --text'
which zenity > /dev/null
if [ $? -ne 0 ]; then
	notifycmd='gxmessage'
	notifyarg=''
	which gxmessage > /dev/null
	if [ $? -ne 0 ]; then
		notifycmd='xmessage'
		notifyarg=''
		which xmessage > /dev/null
		if [ $? -ne 0 ]; then
			notifycmd='echo'
			notifyarg=''
		fi
	fi
fi

if [ "$1" = "-R" ]; then

	which xdotool > /dev/null
	if [ $? -ne 0 ]; then
		${notifycmd} ${notifyarg} "Please install xdotool to refresh panes."
		exit 1
	fi
	xdotool search --name 'fz: Next Nodes' key "R"
	xdotool search --name 'fz: Recent Log' key "R"
	xdotool search --name 'fz: Log Time' key "R"
	exit 0

fi

which thisdesktop-geometry.sh > /dev/null
if [ $? -ne 0 ]; then
	${notifycmd} ${notifyarg} "This requires 'thisdesktop-geometry.sh'."
	exit 1
fi
which monitor.sh > /dev/null
if [ $? -ne 0 ]; then
	${notifycmd} ${notifyarg} "This requires 'monitor.sh'."
	exit 1
fi
which graph-resident > /dev/null
if [ $? -ne 0 ]; then
	${notifycmd} ${notifyarg} "This requires 'graph-resident'."
	exit 1
fi

if [ ! -d $FORMALIZERDOTIR/bin ]; then
	${notifycmd} ${notifyarg} "Missing Formalizer executables directory $FORMALIZERDOTDIR/bin."
	exit 1
fi
if [ ! -d $FORMALIZERBASEDIR ]; then
        ${notifycmd} ${notifyarg} "Missing Formalizer base directory $FORMALIZERBASEDIR."
        exit 1
fi


xy=$(thisdesktop-geometry.sh)
x=${xy%%x*}

cd $FORMALIZERDOTDIR/bin

if [ "$x" = "1920" ]; then
	monitor.sh 1920
	ln -s -f $FORMALIZERBASEDIR/tools/interface/fzlogtime/fzlogtime-term-1920.sh fzlogtime-term.sh
	ln -s -f $FORMALIZERBASEDIR/tools/interface/fzgraphhtml/fzgraphhtml-term-1920.sh fzgraphhtml-term.sh
	ln -s -f $FORMALIZERBASEDIR/tools/interface/fzloghtml/fzloghtml-term-1920.sh fzloghtml-term.sh
	ln -s -f $FORMALIZERBASEDIR/tools/system/top/index-term-1920.sh index-term.sh
else
	monitor.sh
        ln -s -f $FORMALIZERBASEDIR/tools/interface/fzlogtime/fzlogtime-term.sh fzlogtime-term.sh
        ln -s -f $FORMALIZERBASEDIR/tools/interface/fzgraphhtml/fzgraphhtml-term.sh fzgraphhtml-term.sh
        ln -s -f $FORMALIZERBASEDIR/tools/interface/fzloghtml/fzloghtml-term.sh fzloghtml-term.sh
        ln -s -f $FORMALIZERBASEDIR/tools/system/top/index-term.sh index-term.sh
fi

cd $HOME

graph-resident
if [ $? -ne 0 ]; then
	${notifycmd} ${notifyarg} "Please activate the memory-resident Graph server first (fzserverpq)."
	exit 1
fi

fzlogtime-term.sh
fzloghtml-term.sh
fzgraphhtml-term.sh
index-term.sh
