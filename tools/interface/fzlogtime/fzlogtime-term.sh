#!/bin/bash
USETERM=urxvt
USEBROWSER=w3m

$USETERM -title "fz: Log Time" -rv -geometry 79x28+0-48 -fade 20 -e $USEBROWSER 'http://localhost/cgi-bin/fzlogtime.cgi?quiet=true&cgivar=wrap' 2>&1 > /dev/null &
