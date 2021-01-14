#!/bin/bash
#
# fzlist_backups.sh
#
# Randal A. Koene

fzconfigdir=$HOME/.formalizer/config
fzbasedir=$HOME/src/formalizer
fzlibdir=$fzbasedir/core/lib
fzbackupconfig=$fzconfigdir/fzbackup.py/config.json

if [ "$1" = "-h" ]; then
    echo "Usage: fzlist_backups.sh [-V]"
    echo ""
    echo "List database backups."
    echo ""
    echo "Options:"
    echo "  -V Be verbose."
    echo ""
    exit
fi

# import JSON-lite processing functions
source $fzlibdir/jsonlite.sh

# find the backup root directory, sets 'backuproot'
if [ -f $fzbackupconfig ]; then
    json_get_label_value_pairs_from_file $fzbackupconfig
fi

# default in case it was not configured
if [ "$backuproot" = "" ]; then
    backuproot="$HOME"
fi

if [ "$1" = "-V" ]; then
    echo "Formalizer database backups in $backuproot:"
fi

ls -1 -g -o $backuproot
