#!/bin/bash
# This switches to the directories of various C++ programs that are
# part of the Formalizer and calls make clean && make to recompile
# them. E.g. use this when fundamental classes have been updated,
# such as Node or Graph.

topdir=`pwd`
cpp_dirs="core/fzedit core/fzgraph core/fzgraphsearch core/fzlog core/fzquerypq core/fzserverpq core/fzupdate tools/interface/fzdashboard tools/interface/fzgraphhtml tools/interface/fzloghtml tools/interface/fzlogmap tools/interface/fzlogtime tools/interface/fzvismilestones tools/interface/nodeboard tools/system/schedule"

for cppdir in $cpp_dirs; do

	cd $topdir/$cppdir

	make clean && make

done

cd $topdir
echo "Done"
