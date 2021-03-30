#! /bin/bash
# get_log_entry.sh - Extract the content of a specific Log entry
# Randal A. Koene, 20210329
#
# Please note: Although this script is called BY CGI scripts, it should never
# be turned into a CGI script, because the alt_fzloghtmlpath replacement argument
# would be a gaping security hole that allows someone to run an arbitrary program.

fzloghtmlpath=fzloghtml

gettext=1
getnode=1
if [ "$1" = "-t" ]; then
    getnode=0
    shift
else
    if [ "$1" = "-n" ]; then
        gettext=0
        shift
    fi
fi

# The following enables operation when we need to use ./fzloghtml when called by a CGI script
alt_fzloghtmlpath=$2
if [ "$alt_fzloghtmlpath" != "" ]; then
    fzloghtmlpath="$alt_fzloghtmlpath"
fi

logentry_id=$1
logchunk_id=${logentry_id%%.*}
logentry_minor_id=${logentry_id##*.}
logchunk_raw=$(${fzloghtmlpath} -1 $logchunk_id -2 $logchunk_id -c 1 -N -o STDOUT -F raw)
errcode=$?

if [ $errcode -ne 0 ]; then
    1>&2 echo "Unable to extract Log chunk data for Log chunk with ID ${logchunk_id}."
    exit $errcode
fi

logentrycontent=$(printf "$logchunk_raw" | sed -n "/ENTRY:${logentry_minor_id}:/,/__END__/ p")
logentrynode=$(printf "$logentrycontent" | sed -n "s/^ENTRY:${logentry_minor_id}:\(.*\)$/\1/p")
logentrytext=$(printf "$logentrycontent" | sed '1,/__BEGIN__/ d; /__END__/,$ d')

if [ $getnode -eq 1 -a $gettext -eq 1 ]; then
    echo "{$logentrynode}"
    printf "$logentrytext"
    exit 0
fi

if [ $getnode -eq 1 ]; then
    printf "$logentrynode"
    exit 0
fi

if [ $gettext -eq 1 ]; then
    printf "$logentrytext"
    exit 0
fi
