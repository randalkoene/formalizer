#!/bin/bash
#
# fzdaily.sh
#
# Randal A. Koene

fzdaily_prevrun=$HOME/.formalizer/fzdaily.stamp
fzdaily_do=$HOME/.formalizer/fzdaily_do.source

if [ "$1" = "-h" ]; then
    echo "Usage: fzdaily.sh"
    echo ""
    echo "Ensures that a set of actions is carried out if they have not"
    echo "already been carried out this day."
    echo ""
    echo "This script should be called from the crontab, ideally, at a"
    echo "time of day where it is likely that cron is running."
    echo ""
    echo "For example, with this crontab line:"
    echo "31 19 * * * $HOME/.formalizer/bin/fzdaily.sh > /tmp/fzdaily.log 2>&1"
    echo ""
    echo "If you want to be even more certain that the activities will be"
    echo "carried out then simply call fzdaily.sh multiple times per day. The"
    echo "datestamp test will ensure that the activities are carried out at"
    echo "most once per day."
    echo ""
    echo "For example, specify:"
    echo "31 7,13,19 * * * $HOME/.formalizer/bin/fzdaily.sh > /tmp/fzdaily.log 2>&1"
    echo ""
    echo "The last time fzdaily.sh was run is stored as a date stamp in the"
    echo "file $fzdaily_prevrun."
    echo ""
    echo "The set of activities to carry out is sourced from the file"
    echo "$fzdaily_do. Add activities there."
    echo ""
    exit 
fi

datestamp=$(date +"%Y%m%d")
prevstamp="??"
if [ -f $fzdaily_prevrun ]; then
    prevstamp=$(cat $fzdaily_prevrun)
fi

if [ "$datestamp" != "$prevstamp" ]; then

    echo "$datestamp" > $fzdaily_prevrun

    if [ -f $fzdaily_do ]; then

        source $fzdaily_do

    fi
fi
