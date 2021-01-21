#!/bin/bash
USETERM=urxvt
USEBROWSER=w3m

$USETERM -title "fz: Next Nodes" -rv -geometry 160x47+0+0 -fade 20 -e $USEBROWSER 'http://localhost/cgi-bin/fzgraphhtml-cgi.py' 2>&1 > /dev/null &
