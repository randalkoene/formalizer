#! /bin/bash
#
# fzbackup-restore-test.sh
#
# Randal A. Koene, 20261127
#
# Test fzbackup-restore.sh and its undo from start to finish, on a throwaway
# test database. The live database and fzserverpq are not touched.
#
#   1. Make a test database from the newest local backup ("today").
#   2. Restore an older backup into it with fzbackup-restore.sh -d.
#   3. Check that the test database now matches the older backup exactly.
#   4. Undo the restore with fzbackup-restore.sh -d ... --undo.
#   5. Check that the test database matches the newest backup again.
#   6. Delete the test database.
#
# Takes several minutes: each check restores a backup into a scratch database.

set -o pipefail

backuproot=$(python3 -c "import json;print(json.load(open('$HOME/.formalizer/config/fzbackup.py/config.json'))['backuproot'])" 2>/dev/null)
[ -n "$backuproot" ] || backuproot="$HOME/.formalizer/archive/postgres/"
backuproot="${backuproot%/}"

backups=$(ls "$backuproot" | grep -E '^formalizer-postgres-backup-[0-9]{12}\.gz$' | sort)
newest=$(echo "$backups" | tail -1)
# The older backup: the newest one from an earlier month than the newest backup.
newest_month=${newest:27:6}
older=$(echo "$backups" | awk -v m="$newest_month" 'substr($0,28,6) < m' | tail -1)
testdb="fzrestoretest_$(date +%Y%m%d%H%M%S)"

results=()
fail=""
record() {
	results+=("$1")
	[[ "$1" == FAIL* ]] && fail="yes"
}

finish() {
	echo ""
	echo "--- Deleting the test database $testdb ---"
	dropdb --if-exists "$testdb" && echo "Deleted."
	echo ""
	echo "==================== Restore test summary ===================="
	for r in "${results[@]}"; do echo "  $r"; done
	if [ -z "$fail" ]; then
		echo "PASS: restore and undo work."
	else
		echo "FAIL: see the steps above."
	fi
	echo "=============================================================="
	[ -z "$fail" ]
	exit $?
}

if [ -z "$newest" ] || [ -z "$older" ]; then
	echo "Need at least two local backups from different months in $backuproot."
	exit 1
fi

echo "=============================================================="
echo "Restore test on a throwaway database. The live database and"
echo "fzserverpq are not touched."
echo ""
echo "  test database: $testdb"
echo "  'today':       $newest"
echo "  older backup:  $older"
echo "=============================================================="

echo ""
echo "--- 1. Make the test database from $newest ---"
if createdb -T template0 "$testdb" && \
   gzip -d -c "$backuproot/$newest" | psql -d "$testdb" -q -v ON_ERROR_STOP=1 --single-transaction -o /dev/null; then
	record "OK    1. test database made from the newest backup"
else
	record "FAIL  1. could not make the test database"
	finish
fi

echo ""
echo "--- 2. Restore the older backup into the test database ---"
restore_out=$(echo y | fzbackup-restore.sh -d "$testdb" "$older" 2>&1)
echo "$restore_out"
ts=$(echo "$restore_out" | grep -oE -- '--undo [0-9]{14}' | head -1 | cut -d' ' -f2)
if echo "$restore_out" | grep -q 'RESTORE DONE: every table matches the backup' && [ -n "$ts" ]; then
	record "OK    2. restore reported that every table matches the older backup"
else
	record "FAIL  2. restore did not report success"
	finish
fi

echo ""
echo "--- 3. Independent check: test database equals the older backup ---"
fzbackup-verify.py -d "$testdb" "$backuproot/$older"
if [ $? -eq 0 ]; then
	record "OK    3. test database matches the older backup"
else
	record "FAIL  3. test database does not match the older backup"
fi

echo ""
echo "--- 4. Undo the restore (time stamp $ts) ---"
echo y | fzbackup-restore.sh -d "$testdb" --undo "$ts"
if [ $? -eq 0 ]; then
	record "OK    4. undo ran"
else
	record "FAIL  4. undo failed"
	finish
fi

echo ""
echo "--- 5. Check: test database equals the newest backup again ---"
fzbackup-verify.py -d "$testdb" "$backuproot/$newest"
if [ $? -eq 0 ]; then
	record "OK    5. after the undo the test database matches the newest backup again"
else
	record "FAIL  5. after the undo the test database does not match the newest backup"
fi

finish
