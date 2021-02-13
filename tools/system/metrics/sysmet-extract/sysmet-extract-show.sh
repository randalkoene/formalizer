#!/bin/bash
#
# sysmet-extract-show.sh
#
# Randal A. Koene, 2021-02-13

fzlogmap -D 7 -r -o STDOUT -q -C -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_a2c.json
fzlogmap -D 7 -r -o STDOUT -q -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_work.json
