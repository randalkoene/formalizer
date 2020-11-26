#!/bin/bash
USETERM=urxvt
USEBROWSER=w3m

$USETERM -title "fz: Next Nodes" -rv -geometry 108x51-0+0 -fade 20 -e $USEBROWSER 'http://localhost/cgi-bin/fzloghtml-cgi.py?frommostrecent=on&numchunks=100#END' 2>&1 > /dev/null &
