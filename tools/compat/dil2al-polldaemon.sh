#!/bin/bash
#
# dil2al-polldaemon.sh
#
# Randal A. Koene, 20200903
#
# A simple polling daemon for collabortive requests to dil2al. This poses
# the lowest barriers for interprocess collaboration with dil2al and can
# work for requests from CGI handlers as well. In this manner, this
# ultra simple daemon script also provides a level of compatibility
# between new Formalizer code the dil2al program.
#
# How it works:
#
#   * The script is run in an X terminal, e.g. `dil2al-polldaemon.sh`. It
#     will enter a wait-loop and will very occasionally print to the
#     terminal, just to announce that it is still there.
#   * A client will write the file $DPDDIR/dil2al_poller_request. That
#     file will contain a request, e.g. `makenote`.
#   * As the daemon script polls and detects the request file, it takes
#     the request, deletes the request file, processes the request, and
#     writes a response to $DPDDIR/dil2al_poller_response.
#   * The client can read the response to know that the request has been
#     handled and what the outcome was.

DPD_DIR=/var/www/webdata/dil2al-polldaemon

echo "The dil2al poll daemon is starting. Entering poll loop..."

seconds=0

while true; do

    sleep 1

    (( seconds++ ))
    if [ $seconds -ge 300 ]; then
        printf "...still polling: "
        date
        seconds=0
    fi

    if [ -f $DPD_DIR/dil2al_poller_request ]; then

        request=$( cat $DPD_DIR/dil2al_poller_request )
        rm -f $DPD_DIR/dil2al_poller_request

        if [ "$request" = "makenote" ]; then
            printf "Request received: makenote. \nHandling request: $( date )" > $DPD_DIR/dil2al_poller_response
            makenote $DPD_DIR/dil2al_poller_data
        else
            printf "Unknown request: $request. \nNo handler available." > $DPD_DIR/dil2al_poller_response
        fi

	printf "\n\n\n\n\n...back to polling: "
	date
	
    fi

done
