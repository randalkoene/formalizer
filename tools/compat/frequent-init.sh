#! /bin/bash
# frequent-init.sh
# Randal A. Koene, 20201225
#
# Create an initial collection for the 'frequent' Named Node List.
#
# This is useful during the transition stage where Named Node Lists may be
# regularly cleared as the Graph and Log are refreshed via conversion from
# Formalizer v1.x format. The 'frequent' List is useful during Node selection.

sleep_and_nap="\
20110215103741.1,\
20110215104130.1,\
20110215104308.1,\
20110215104436.1,\
20110215104555.1,\
20110215104710.1,\
20110215104832.1,\
20190803231629.1\
"

meals="\
20140428114648.1,\
20140428114925.1,\
20110408181347.1\
"

self_care="\
20200601093905.1,\
20171130175219.1,\
20190412111605.1,\
20190622091434.1\
"

exercise="\
20100403134619.1,\
20110215130407.1,\
20130107120139.1\
"

social="\
20171130175518.1,\
20190513145341.1,\
20130603085836.1\
"

reward="\
20200513232732.1,\
20180603114625.1,\
20190306191404.1\
"

emergency="\
20200516103456.1\
"

travel="\
20110409195208.1,\
20150812175622.1,\
20191002093500.1,\
20191006084838.1\
"

catch_up="\
20120716170436.1,\
20190426103150.1,\
20200506064335.1\
"

chores="\
20150607104410.1,\
20120909012900.1\
"

family="\
20190110065308.1,\
20190307161113.1,\
20181002164240.1,\
20190628095100.1\
"

systematic="\
20190426102301.1,\
20101228040442.1,\
20130107114129.1,\
20130107113915.1,\
20130107115044.1\
"

frequent="$sleep_and_nap,$meals,$self_care,$exercise,$family,$social,$chores,$systematic,$travel,$reward,$catch_up,$emergency"

echo "Formalizer:Graph:NamedNodeList:Frequent:Init 0.1.0-0.1"
echo ""
echo "Calling fzgraph as follows:"
echo ""
echo "  fzgraph -L add -l frequent -S \"$frequent\""
echo ""

fzgraph -L add -l frequent -S "$frequent"
