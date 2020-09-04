#!/bin/bash

printf "Content-type: text/html\n\n"

printf "Formalizer Test Prototypes: Bash Wrapper\n"

bash -c '/usr/bin/emacs --version'

date > /var/www/webdata/dil2al-polldaemon/cgidatetest.txt
