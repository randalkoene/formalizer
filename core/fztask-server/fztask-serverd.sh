#! /bin/bash
# fztask-serverd.sh
# 
# Randal A. Koene 2021-03-13
#
# Launch the Task server.

echo "This is not yet a full daemonization. That is yet to be implemented."
echo "This is really just a handy start script."
echo ""

FZBASEDIR="$HOME/src/formalizer"
FZCOREDIR="$FZBASEDIR/core"
FZTASKSERVERDIR="$FZCOREDIR/fztask-server"

cd $FZTASKSERVERDIR
source env/bin/activate
python3 -u fztask-server.py

if [ $? -ne 0 ]; then

	echo "Server returned with error code."
	echo "There is probably a dangling lock file."
	echo ""
	printf "Remove dangling lock file? (Y/n)"
	read n

	if [ "$n" != "n" ]; then
		rm -f $HOME/.formalizer/.fztask-server.lock
		echo "If there was a lock file, it has been removed."
	fi

fi
