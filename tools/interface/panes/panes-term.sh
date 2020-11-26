#!/bin/bash

fzlogtime-term.sh
fzloghtml-term.sh
fzgraphhtml-term.sh
urxvt -title "Help" -rv -geometry 108x28-0-48 -fade 30 -e w3m /home/randalk/doc/html/lists/System/system-help.html 2>&1 > /dev/null &
