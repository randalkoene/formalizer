#! /bin/bash
# categories_hourly-NNLs-init.sh
# Randal A. Koene, 20210211
#
# Create an initial collection of Named Node Lists for hourly metrics.
#
# This is useful during the transition stage where Named Node Lists may be
# regularly cleared as the Graph and Log are refreshed via conversion from
# Formalizer v1.x format.

# 
group_actions="\
20160315065614.1,\
20081125102516.1,\
20141206180801.1\
"

# 
group_infrastructure="\
20110219143845.1,\
20100403134619.1,\
20100825135651.1\
"

# 
group_sleep="\
20110215103741.1,\
20110215104130.1,\
20110215104308.1,\
20110215104436.1,\
20110215104555.1,\
20110215104710.1,\
20110215104832.1,\
20190803231629.1\
"

# 
group_other="\
20120912131158.1,\
20120329124446.1\
"

numdo=1

function rebuild_category_NNL() {
    printf "[$numdo/4] Let's rebuild the $1 NNL (press ENTER)"
    read n

    echo "Deleting $1 if it existed."
    fzgraph -L delete -l "$1"
    echo "Creating $1"
    fzgraph -L add -l "$1" -S "$2"
    
    numdo=$((numdo + 1))
}

echo "Formalizer:Graph:NamedNodeList:Categories:Init 0.1.0-0.1"
echo ""
echo "Calling fzgraph as follows:"
echo ""
echo "  fzgraph -L add -l <NNL> -S <Node-IDs-list>"
echo ""

rebuild_category_NNL "group_actions" "$group_actions"

rebuild_category_NNL "group_infrastructure" "$group_infrastructure"

rebuild_category_NNL "group_sleep" "$group_sleep"

rebuild_category_NNL "group_other" "$group_other"

echo "Done."
