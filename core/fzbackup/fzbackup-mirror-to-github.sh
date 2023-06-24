#! /bin/bash
# fzbackup-mirror-to-github.sh
# Randal A. Koene, 20210526
#
# A convenience script to git push database backups to Github.
#
# Note: At present, this script may only work when run as user randalk.

echo "==================================================="
echo "Pushing database backups and DayWiz JSON to GitHub."
echo "==================================================="
echo ""
echo "Note: Your username for this Github account is 'koenera'."
echo "      Your password is: S17...@4..."
echo ""

todaydate=`date +%F`
Ymddate=`date +%Y%m%d`

daywizjsonpath="$HOME/.formalizer/archive/daywiz_data-$Ymddate.json"
cp /var/www/webdata/formalizer/.daywiz_data.json $daywizjsonpath

echo "Copied DayWiz JSON data to $daywizjsonpath."
echo ""

cd $HOME/.formalizer
git add .
git commit -m "Update for $todaydate."

git push

echo "Formalizer database backups mirrored to GitHub."
echo ""
echo "Please remember to prune excess backups from time to time."
echo ""
