#!/bin/bash
#
# v1xv2x-refresh.sh
#
# Randal A. Koene, 20200921
#
# Updates:
#   20201027 Added `fzquery -R histories` and another `fzsetup.py -1 fzuser` option.
#
# A shorthand for the steps involved in removing and replacing v2.x
# database tables for Graph and Log content, and to regenerate those
# with `dil2graph`.
#
# This script is primarily for use during the early transition phase
# where an up-to-date Graph and Log copy is already useful, but
# daily Formalizer activity is still being carried out with dil2al.
#

if [ "$1" = "-h" ]; then
    echo "Usage: v1xv2x-refresh.sh [-V|-q]"
    echo ""
    echo "This script removes existing Graph Log tables, and other Formalizer 2.x"
    echo "data from the database, then generates replacement data from files"
    echo "containing Formalizer 1.x data. This process uses the dil2graph"
    echo "conversion tool. Additionally, necessary post-conversion setup steps"
    echo "are carried out with the fzsetup tool."
    echo ""
    echo "Options:"
    echo ""
    echo "  -V be as verbose as possible."
    echo "  -q be as quiet as possible."
    echo ""
    exit
fi

v_or_q=""

if [ $# -gt 0 ]; then
    if [ "$1" = "-V" ]; then
        v_or_q="-V"
    else
        v_or_q="-q"
    fi
fi

echo "This script removes existing Graph and Log tables from the"
echo "database and then regenreates them using dil2graph."
echo ""
echo "Do you wish to proceed? (y/N) "
read proceed

if [ "$proceed" = "y" ]; then
    echo ""
    echo "Removing Graph table..."
    fzsetup.py -R graph
    echo ""
    echo "Removing Log table..."
    fzsetup.py -R log

    echo ""
    echo "Still proceed with regeneration? (y/N) "
    read proceed
    if [ "$proceed" = "y" ]; then
        echo "Regenerating Graph and Log tables..."
        dil2graph $v_or_q

        echo ""
        echo "Refreshing fzuser access and permissions..."
        fzsetup.py -1 fzuser

        echo "We can do the recommended fzquerypq -R histories and fzquerypq -R namedlists now if you wish? (Y/n) "
        read refreshhistories
        if [ "$refreshhistories" != "n" ]; then

            echo "Refreshing Node histories cache..."
            fzquerypq -R histories $v_or_q

            echo "Refreshing Named Node Lists cache..."
            fzquerypq -R namedlists $v_or_q

            echo ""
            echo "Refreshing fzuser access and permissions for the cache..."
            fzsetup.py -1 fzuser


        fi

        echo "We can initialize the 'frequent' Named Node List now if you wish? (Y/n) "
        read initfrequent
        if [ "$initfrequent" != "n" ]; then
            frequent-init.sh
        fi

        echo ""
        echo "v1xv2x-refresh.sh completed."
    fi
fi
