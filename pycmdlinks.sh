#!/bin/bash

cd $1
for n in `ls *.py`; do
    ln -s -f $n ${n%.py}
done
