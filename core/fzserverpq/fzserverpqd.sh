#! /bin/bash
# fzserverpqd.sh
# 
# Randal A. Koene 2021-01-03
#
# Daemonize the Graph server.

fzserverpq_error_log="/var/www/html/formalizer/formalizer.core.server.error.ErrQ.log"

echo "This looping script is designed to automatically restart a resident"
echo "fzserverpq if it exited with an error status."
echo ""
echo "(Note that this script has only been tested with the"
echo "default_to_localhost configuration option.)"
echo ""

while true
do

	echo "This is not yet a full daemonization. That is yet to be implemented."
	echo "This is really just a handy start script."
	echo ""
	echo "Errors are logged to $fzserverpq_error_log."
	echo ""

	fzserverpq -G -E $fzserverpq_error_log

	if [ $? -eq 0 ]; then
		echo "Server exited normally."
		exit 0
	fi

	echo "Server returned with error code."
	echo "There is probably a dangling lock file."
	echo ""
	# printf "Remove dangling lock file? (Y/n)"
	# read n

	# if [ "$n" != "n" ]; then
	# 	rm -f $HOME/.formalizer/.fzserverpq.lock
	# 	echo "If there was a lock file, it has been removed."
	# fi

	echo "Automatically removing dangling lock file."
	rm -f $HOME/.formalizer/.fzserverpq.lock
	echo "If there was a lock file, it has been removed."

done
