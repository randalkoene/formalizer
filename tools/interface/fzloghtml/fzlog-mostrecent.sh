#!/bin/bash
# fzlog-mostrecent.sh
# Randal A. Koene, 20210113

if [ "$1" = "-h" ]; then
cat <<- EOF
Usage: fzlog-mostrecent.sh

Get information about the most recent Log chunk and Log entry,
i.e. the tail of the Log.

Returns to STDOUT on one line:
  1. The opening datetime stamp of the Log chunk.
  2. The OPEN or CLOSED status of the Log chunk.
  3. The Node ID of the Node to which the Log chunk belongs.
  4. The minor ID (index number) of the most recent Log entry. 
  5. The most recent Log entry ID.
  6. The closing datetime stamp of the Log chunk or -1 (if OPEN).

EOF
exit
fi

fzloghtml -R -o STDOUT -N -F raw -q
