#!/bin/bash
#
# sysmet-extract-term.sh
#
# Randal A. Koene, 2021-02-13

USETERM=urxvt

$USETERM -title "SysMet Extract" -rv -geometry 248x76+0+0 -e sysmet-extract-show.sh &
