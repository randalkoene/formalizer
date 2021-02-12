#! /bin/bash
# categories_a2c-NNLs-init.sh
# Randal A. Koene, 20210211
#
# Create an initial collection of Named Node Lists for a2c metrics.
#
# This is useful during the transition stage where Named Node Lists may be
# regularly cleared as the Graph and Log are refreshed via conversion from
# Formalizer v1.x format.

# 
group_sleep="\
20110215103741.1\
20110215104130.1\
20110215104308.1\
20110215104436.1\
20110215104555.1\
20110215104710.1\
20110215104832.1\
20190803231629.1\
"

# Open Eros starting
group_ipab="\
20181210124234.1\
"

# Corrective tuning to achieve LEARNED-LVL1, Daily communication, Evening writing, Debts process, Discuss rearrangements, crucial Personal Roadmap travel, (temporary) Daily communications check, Facing Urgent or Emergency Challenges
group_dayint="\
20081125102516.1\
20180503204605.1\
20190113171753.1\
20190113171219.1\
20200101080724.1\
20200101080623.1\
20200117124517.1\
20200516103456.1\
20200407144927.1\
"

# 
#group_buildsystem="\
#"

# AL Update & Daily Self-Evaluation, AL Update, Take stock, Regular P.O.Box mail pick-up, Extra P.O.Box mail pick-up
group_system="\
20091115180507.1\
20040402045825.1\
20171201125029.1\
20200226144258.1\
20200427064612.1\
"

# Burning Man, 3D Blender skills, Composing and music skills, Write papers
group_hobbies="\
20120329124446.1\
20130107115044.1\
20130107114129.1\
20101228040442.1\
"

# 
group_meals="\
20140428114648.1\
20140428114925.1\
20110408181347.1\
"

# 
group_solo="\
20171130175219.1\
20110408052952.1\
20200601093905.1\
"

# 
group_social="\
20171130175518.1\
20140428115038.1\
20130603085836.1\
"

# Morning readiness, Driving family, Run bus, (20181023101453.1 Fire wood - moved to well-being and health, see TL#201903100640.2)
group_chores="\
20060914084328.1\
20190110065308.1\
20120909012900.1\
20180708143245.1\
"

# 
group_travel="\
20110409195208.1\
20150812175622.1\
"

# Death Guild, Getting out, Fire wood (see TL#201903100640.2), A specific version of Getting out
group_wellbeing="\
20110215130407.1\
20110218000634.1\
20181023101453.1\
20200530070528.1\
"

numdo=1

function rebuild_category_NNL() {
    printf "[$numdo/12] Let's rebuild the $1 NNL (press ENTER)"
    read n

    fzgraph -L delete -l "$1"
    fzgraph -L add -l "$1" -S "$2"
    
    numdo=$((numdo + 1))
}

echo "Formalizer:Graph:NamedNodeList:Categories:Init 0.1.0-0.1"
echo ""
echo "Calling fzgraph as follows:"
echo ""
echo "  fzgraph -L add -l <NNL> -S <Node-IDs-list>"
echo ""

rebuild_category_NNL "group_wellbeing" "$group_wellbeing"

rebuild_category_NNL "group_sleep" "$group_sleep"

rebuild_category_NNL "group_ipab" "$group_ipab"

rebuild_category_NNL "group_dayint" "$group_dayint"

rebuild_category_NNL "group_buildsystem" "$group_buildsystem"

rebuild_category_NNL "group_system" "$group_system"

rebuild_category_NNL "group_hobbies" "$group_hobbies"

rebuild_category_NNL "group_meals" "$group_meals"

rebuild_category_NNL "group_solo" "$group_solo"

rebuild_category_NNL "group_social" "$group_social"

rebuild_category_NNL "group_chores" "$group_chores"

rebuild_category_NNL "group_travel" "$group_travel"

echo "Done."
