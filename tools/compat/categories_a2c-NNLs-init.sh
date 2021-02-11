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

#frequent="$sleep_and_nap,$meals,$self_care,$exercise,$family,$social,$chores,$systematic,$travel,$reward,$catch_up,$emergency"

echo "Formalizer:Graph:NamedNodeList:Categories:Init 0.1.0-0.1"
echo ""
echo "Calling fzgraph as follows:"
echo ""
echo "  fzgraph -L add -l <NNL> -S <Node-IDs-list>"
echo ""

fzgraph -L add -l "group_wellbeing" -S "$group_wellbeing"

fzgraph -L add -l "group_sleep" -S "$group_sleep"

fzgraph -L add -l "group_ipab" -S "$group_ipab"

fzgraph -L add -l "group_dayint" -S "$group_dayint"

fzgraph -L add -l "group_buildsystem" -S "$group_buildsystem"

fzgraph -L add -l "group_system" -S "$group_system"

fzgraph -L add -l "group_hobbies" -S "$group_hobbies"

fzgraph -L add -l "group_meals" -S "$group_meals"

fzgraph -L add -l "group_solo" -S "$group_solo"

fzgraph -L add -l "group_social" -S "$group_social"

fzgraph -L add -l "group_chores" -S "$group_chores"

fzgraph -L add -l "group_travel" -S "$group_travel"
