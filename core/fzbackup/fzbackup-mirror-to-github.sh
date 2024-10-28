#! /bin/bash
# fzbackup-mirror-to-github.sh
# Randal A. Koene, 20210526
#
# A convenience script to git push database backups to Github.
#
# Note: At present, this script may only work when run as user randalk.

now_timestamp=`date +%Y%m%d%H%M%S`
echo "$now_timestamp" > /dev/shm/fzbackup-mirror-to-github.launched

signalfile=""
if [ "$1" = "-S" ]; then
	signalfile="$2"
fi

echo "==================================================="
echo "Pushing database backups and DayWiz JSON to GitHub."
echo "==================================================="
echo ""
echo "Note: Your username for this Github account is 'koenera'."
echo "      Your password is: S17...@4..."
echo ""

# echo "JUST TESTING!"
# if [ "$signalfile" != "" ]; then
# 	now_timestamp=`date +%Y%m%d%H%M%S`
# 	printf $now_timestamp > $signalfile
# fi
# exit 0

todaydate=`date +%F`
Ymddate=`date +%Y%m%d`

# 20241028 RK - The DayWiz data is now stored within the database,
# so the following steps are now commented out.
# daywizjsonpath="$HOME/.formalizer/archive/daywiz_data-$Ymddate.json"
# cp /var/www/webdata/formalizer/.daywiz_data.json $daywizjsonpath

# echo "Copied DayWiz JSON data to $daywizjsonpath."
# echo ""

fzdaily.sh

echo "Checked conditional running of daily ops to prepare a backup."
echo ""

cd $HOME/.formalizer
git add .
git commit -m "Update for $todaydate."

git push

echo "Formalizer database backups mirrored to GitHub."
echo ""
echo "Please remember to prune excess backups from time to time."
echo ""

num_daywiz=$(ls $HOME/.formalizer/archive/daywiz_data-* | wc -l)
num_db=$(ls $HOME/.formalizer/archive/postgres | wc -l)

echo "There are $num_daywiz DayWiz JSON data backup files."
echo "There are $num_db Formalizer database backup files."
echo ""

echo "List of Database backup files:"
echo "-----------------------------"
ls $HOME/.formalizer/archive/postgres

echo ""
echo "List of DayWiz JSON data backup files:"
echo "-------------------------------------"
cd $HOME/.formalizer/archive
ls daywiz_data-*

if [ "$signalfile" != "" ]; then
	now_timestamp=`date +%Y%m%d%H%M%S`
	printf $now_timestamp > $signalfile
fi
