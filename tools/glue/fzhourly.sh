#!/bin/bash
#
# fzhourly.sh
#
# Randal A. Koene

fzhourly_prevrun=$HOME/.formalizer/fzhourly.stamp
fzhourly_do=$HOME/.formalizer/fzhourly_do.source

if [ "$1" = "-h" ]; then
    echo "Usage: fzhourly.sh"
    echo ""
    echo "Ensures that a set of actions is carried out every half hour."
    echo ""
    echo "This script should be called from the crontab, ideally, at a"
    echo "time of day where it is likely that cron is running."
    echo ""
    echo "For example, with this crontab line:"
    echo "16,46 * * * * $HOME/.formalizer/bin/fzhourly.sh > /tmp/fzhourly.log 2>&1"
    echo ""
    echo "The last time fzhourly.sh was run is stored as a date stamp in the"
    echo "file $fzhourly_prevrun."
    echo ""
    echo "The set of activities to carry out is sourced from the file"
    echo "$fzhourly_do. Add activities there."
    echo ""
    exit 
fi

timestamp=$(date +"%Y%m%d%H%M")

echo "$timestamp" > $fzhourly_prevrun

if [ -f $fzhourly_do ]; then

    source $fzhourly_do

fi
