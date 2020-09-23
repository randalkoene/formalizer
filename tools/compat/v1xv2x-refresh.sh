#!/bin/bash
#
# v1xv2x-refresh.sh
#
# Randal A. Koene, 20200921
#
# A shorthand for the steps involved in removing and replacing v2.x
# database tables for Graph and Log content, and to regenerate those
# with `dil2graph`.
#
# This script is primarily for use during the early transition phase
# where an up-to-date Graph and Log copy is already useful, but
# daily Formalizer activity is still being carried out with dil2al.
#

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
        dil2graph

        echo ""
        echo "v1xv2x-refresh.sh completed."
    fi
fi