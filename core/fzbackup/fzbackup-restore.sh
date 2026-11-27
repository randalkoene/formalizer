#! /bin/bash
#
# fzbackup-restore.sh
#
# Randal A. Koene, 20261127
#
# Replace the live Formalizer data with a database backup, in a way that can
# be undone. The Backups page (fzbackup-cgi.py) shows this command for the
# backup chosen there.
#
#   1. Make a fresh backup (and off-site copy) of the current state.
#   2. Verify the chosen backup in a scratch database.
#   3. Stop fzserverpq (it keeps the Graph in memory).
#   4. Rename the current schema aside (nothing is deleted).
#   5. Load the backup and give the usual roles access (as fzrestore.sh does).
#   6. Check that the restored data matches the backup exactly.
#   7. Tell how to start fzserverpq again, and how to undo.
#
# Undo: fzbackup-restore.sh --undo <timestamp>  (puts the renamed schema back)
#
# Plan B package 4 (Node 20261102153503.1).

set -o pipefail

schema="$USER"
database="formalizer"
testmode=""

usage() {
	cat <<EOF
Usage: fzbackup-restore.sh [-d <database>] <backup>
       fzbackup-restore.sh [-d <database>] --undo <timestamp>
       fzbackup-restore.sh [-d <database>] --drop-old <timestamp>

  <backup>   a backup file, or its name in the backup directory
             (formalizer-postgres-backup-YYYYmmddHHMM.gz)

  -d <database>
             restore into this database instead of 'formalizer'. For testing:
             fzserverpq is not stopped, and no fresh backup is made. Create a
             test database first, e.g. with: fzbackup-verify.py --keep

  --undo <timestamp>
             put back the schema that a restore renamed to ${schema}_before_restore_<timestamp>
             (the restored data is renamed to ${schema}_undone_<timestamp>)

  --drop-old <timestamp>
             delete the renamed schema ${schema}_before_restore_<timestamp> for good,
             once you are sure the restore is what you want
EOF
}

die() {
	echo ""
	echo "STOPPED: $1"
	exit 1
}

confirm() {
	printf "%s (y/N) " "$1"
	read -r answer
	[ "$answer" = "y" ] || [ "$answer" = "Y" ] || die "Nothing more was changed."
}

psql_cmd() {
	psql -d "$database" -v ON_ERROR_STOP=1 -q -c "$1"
}

schema_exists() {
	[ "$(psql -d "$database" -At -c "SELECT 1 FROM information_schema.schemata WHERE schema_name = '$1';")" = "1" ]
}

stop_server() {
	if [ -n "$testmode" ]; then
		return
	fi
	echo "Stopping fzserverpq..."
	fzgraph -z > /dev/null 2>&1
	for i in $(seq 1 20); do
		pgrep -x fzserverpq > /dev/null || return 0
		sleep 1
	done
	die "fzserverpq is still running. Stop it, then run this command again."
}

server_note() {
	if [ -n "$testmode" ]; then
		echo "(Test mode: fzserverpq was not stopped and does not need a restart.)"
	else
		echo "Start fzserverpq again, in a terminal:"
		echo "  fzserverpqd.sh"
	fi
}

if [ "$1" = "-d" ]; then
	database="$2"
	testmode="yes"
	shift 2
	[ "$database" = "formalizer" ] && testmode=""
fi

if [ $# -lt 1 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
	usage
	exit 0
fi

# ---------- Undo ----------
if [ "$1" = "--undo" ]; then
	ts="$2"
	[[ "$ts" =~ ^[0-9]{14}$ ]] || die "Give the 14-digit time stamp of the restore to undo."
	old="${schema}_before_restore_${ts}"
	undone="${schema}_undone_${ts}"
	schema_exists "$old" || die "There is no schema $old in database $database."
	echo "This puts back the data from before the restore of $ts:"
	echo "  schema $schema     -> $undone   (the restored data, kept)"
	echo "  schema $old -> $schema"
	confirm "Undo the restore?"
	stop_server
	psql_cmd "BEGIN; ALTER SCHEMA \"$schema\" RENAME TO \"$undone\"; ALTER SCHEMA \"$old\" RENAME TO \"$schema\"; COMMIT;" \
		|| die "Renaming failed; nothing was changed."
	echo ""
	echo "Done: the data from before the restore is back."
	echo "The restored data is kept as schema $undone. Delete it when no longer needed:"
	echo "  psql -d $database -c 'DROP SCHEMA \"$undone\" CASCADE;'"
	server_note
	exit 0
fi

# ---------- Drop the renamed old schema ----------
if [ "$1" = "--drop-old" ]; then
	ts="$2"
	[[ "$ts" =~ ^[0-9]{14}$ ]] || die "Give the 14-digit time stamp of the restore."
	old="${schema}_before_restore_${ts}"
	schema_exists "$old" || die "There is no schema $old in database $database."
	confirm "Delete schema $old for good? After this, the restore of $ts cannot be undone."
	psql_cmd "DROP SCHEMA \"$old\" CASCADE;" || die "Deleting failed."
	echo "Deleted schema $old."
	exit 0
fi

# ---------- Restore ----------
backup="$1"
backuproot=$(python3 -c "import json;print(json.load(open('$HOME/.formalizer/config/fzbackup.py/config.json'))['backuproot'])" 2>/dev/null)
[ -n "$backuproot" ] || backuproot="$HOME/.formalizer/archive/postgres/"
if [ ! -f "$backup" ]; then
	backup="${backuproot%/}/$backup"
fi
[ -f "$backup" ] || die "Backup not found: $1. (Fetch it first if it is only off-site.)"

ts=$(date +%Y%m%d%H%M%S)
old="${schema}_before_restore_${ts}"

echo "==================================================================="
echo "Restore the Formalizer database from:"
echo "  $backup"
echo "into database '$database', schema '$schema'."
echo ""
if [ -z "$testmode" ]; then
	echo "Steps: fresh backup of the current state, verify the chosen backup,"
	echo "stop fzserverpq, rename the current schema to $old,"
	echo "load the backup, check it, and tell how to restart fzserverpq."
else
	echo "TEST MODE (database '$database'): no fresh backup, fzserverpq keeps running."
	echo "Steps: verify the chosen backup, rename the current schema to $old,"
	echo "load the backup, check it."
fi
echo ""
echo "Nothing is deleted. To undo afterwards:"
echo "  fzbackup-restore.sh$( [ -n "$testmode" ] && echo " -d $database") --undo $ts"
echo "==================================================================="
confirm "Go ahead?"

schema_exists "$schema" || die "There is no schema $schema in database $database to replace."

if [ -z "$testmode" ]; then
	echo ""
	echo "--- 1. Fresh backup of the current state ---"
	fzbackup || die "The fresh backup failed. Nothing was changed."
	fzbackup-offsite.py --no-forget || echo "(The off-site copy failed; the local fresh backup exists. Continuing.)"
fi

echo ""
echo "--- 2. Verify the chosen backup ---"
fzbackup-verify.py -d "$database" "$backup"
[ $? -eq 1 ] && die "The chosen backup failed verification. Nothing was changed."

echo ""
echo "--- 3. Stop fzserverpq ---"
stop_server

echo ""
echo "--- 4. Rename the current schema aside ---"
psql_cmd "ALTER SCHEMA \"$schema\" RENAME TO \"$old\";" || die "Renaming the schema failed. Nothing was changed."
echo "The current data is now schema $old."

echo ""
echo "--- 5. Load the backup ---"
if ! gzip -d -c "$backup" | psql -d "$database" -q -v ON_ERROR_STOP=1 --single-transaction -o /dev/null; then
	echo "Loading failed. Putting the old data back..."
	psql_cmd "DROP SCHEMA IF EXISTS \"$schema\" CASCADE; ALTER SCHEMA \"$old\" RENAME TO \"$schema\";" \
		&& die "Loading the backup failed; the old data is back as schema $schema." \
		|| die "Loading the backup failed, and putting the old data back failed too. It is in schema $old: rename it to $schema with psql."
fi
fzsetup --defaults -d "$database" -1 fzuser > /dev/null 2>&1
fzsetup --defaults -d "$database" -p > /dev/null 2>&1
echo "Loaded."

echo ""
echo "--- 6. Check the restored data against the backup ---"
fzbackup-verify.py -d "$database" "$backup"
check=$?

echo ""
echo "==================================================================="
if [ $check -eq 0 ]; then
	echo "RESTORE DONE: every table matches the backup."
else
	echo "RESTORE DONE, BUT THE CHECK FOUND DIFFERENCES (see above)."
	echo "Consider undoing it."
fi
echo ""
server_note
echo ""
echo "If something is wrong, undo it with:"
echo "  fzbackup-restore.sh$( [ -n "$testmode" ] && echo " -d $database") --undo $ts"
echo "When you are sure, the old data can be deleted with:"
echo "  fzbackup-restore.sh$( [ -n "$testmode" ] && echo " -d $database") --drop-old $ts"
echo "==================================================================="
