#!/bin/bash
USETERM=urxvt
USEBROWSER=w3m

# Note that localcgi=0 requires TCP port-API support for launching specific local programs.
localcgi=1

if [ $localcgi -eq 1 ]; then
    $USETERM -title "fz: Log Time" -rv -geometry 79x28+0-48 -fade 20 -e $USEBROWSER '/cgi-bin/fzlogtime.cgi?quiet=true&cgivar=wrap' 2>&1 > /dev/null &
else
    $USETERM -title "fz: Log Time" -rv -geometry 79x28+0-48 -fade 20 -e $USEBROWSER 'http://localhost/cgi-bin/fzlogtime.cgi?quiet=true&cgivar=wrap' 2>&1 > /dev/null &
fi
