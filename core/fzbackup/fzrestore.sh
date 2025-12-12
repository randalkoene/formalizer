#! /bin/bash
# fzrestore.sh
#
# Randal A. Koene, 2021-01-03
#
# A simple script to restore a backed up database

if [ $# -lt 1 -o "$1" = "-h" ]; then
	echo "Usage: fzrestore.sh [-n] [-s <existing-schema>] <dbbackup-path>"
	echo ""
	echo "Requires path to an archived database as argument."
	echo "See, for example, in ~/.formalizer/archive/postgres/."
	echo ""
	echo "Options:"
	echo "  -n non-interactive, use defaults instead of waiting for input."
	echo "  -s rename an existing schema (usually USER) if it exists before restoring"
	echo "     Use this if you are restoring to an existing database, and"
	echo "     if you want to test a different version of the Formalizer"
	echo "     Graph and Log data. If you want to delete the old schema"
	echo "     then use 'psql' with the 'DROP SCHEMA name CASCADE;' command."
	echo ""
	exit
fi

if [ "$1" = "-n" ]; then
	interactive='n'
	shift
else
	interactive='y'
fi

if [ "$1" = "-s" ]; then
	schema="$2"
	timestamp=$(date +%Y%m%d%H%M)
	psql -d formalizer -c "ALTER SCHEMA ${schema} RENAME TO ${schema}${timestamp};"
	shift
	shift
fi

dbbackuppath="$1"

echo "Commencing restore of archived database $dbbackuppath"
if [ "$interactive" = "y" ]; then
	printf "Press Enter to continue..."
	read n
fi

echo "Creating database (createdb -T template0 formalizer)"

createdb -T template0 formalizer

echo "Setting up database users (fzsetup -1 fzuser)"
echo "Please ignore the 'schema' error for now."
if [ "$interactive" = "y" ]; then
	printf "Press Enter to continue..."
	read n
fi

fzsetup --defaults --ignoreschema -1 fzuser

echo "Importing backup of database tables"
if [ "$interactive" = "y" ]; then
	printf "Press Enter to continue..."
	read n
fi

gzip -d -c $dbbackuppath | psql --set ON_ERROR_STOP=on formalizer

echo "Running user setup again to associate with schema as needed (fzsetup -1 fzuser)"
if [ "$interactive" = "y" ]; then
	printf "Press Enter to continue..."
	read n
fi

fzsetup --defaults -1 fzuser

echo "Give fzuser login and access permissions (fzsetup -p)"
if [ "$interactive" = "y" ]; then
	printf "Press Enter to continue..."
	read n
fi

fzsetup --defaults -p

echo "Restore process completed. Please check for errors!"
