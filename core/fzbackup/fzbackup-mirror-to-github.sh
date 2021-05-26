#! /bin/bash
# fzbackup-mirror-to-github.sh
# Randal A. Koene, 20210526
#
# A convenience script to git push database backups to Github.
#
# Note: At present, this script may only work when run as user randalk.

todaydate=`date +%F`

cd $HOME/.formalizer
git add .
git commit -m "Update for $todaydate."

echo "Pushing database backups to GitHub."
echo "Note: Your username for this Github account is 'koenera'."
echo "      Your password is: S17...@4..."
echo ""

git push

echo "Formalizer database backups mirrored to GitHub."
