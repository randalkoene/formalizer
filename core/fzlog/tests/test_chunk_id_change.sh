#! /bin/bash
# test_chunk_id_change.sh
#
# Tests `fzlog -m <chunk-ID> -1 <time>` (changing a Log chunk's open time, i.e. its ID)
# on a chunk WITH Log entries, in a separate throw-away database. The Formalizer
# database is never touched: the script creates the database fztest_chunkid, fills it
# with made-up chunks and entries, runs the fzlog you name on it, checks the results,
# and drops the database again at the end (also after an error).
#
# Usage: test_chunk_id_change.sh <path-to-fzlog>
#
# Written for Nodes 20261117053456.1 / 20240422072520.1 (2026-11-21).

FZLOG="$1"
DB=fztest_chunkid
S=fztest

if [ -z "$FZLOG" ] || [ ! -x "$FZLOG" ]; then
    echo "Usage: $0 <path-to-fzlog>"
    exit 1
fi
if [ "$DB" = "formalizer" ]; then echo "Refusing to use the Formalizer database."; exit 1; fi
if psql -lqt | cut -d'|' -f1 | grep -qw "$DB"; then
    echo "Database $DB already exists. Not touching it; drop it yourself if it is a leftover test (dropdb $DB)."
    exit 1
fi

createdb "$DB" || { echo "createdb failed."; exit 1; }
trap 'rm -f "$LASTOUT"; dropdb "$DB" && echo "" && echo "Dropped test database $DB."' EXIT

q() { psql -X -q -t -A -d "$DB" -c "$1"; }

psql -X -q -v ON_ERROR_STOP=1 -d "$DB" <<EOF
CREATE SCHEMA $S;
CREATE TABLE $S.Logchunks (id timestamp (0) PRIMARY KEY, nid char(16), tclose timestamp (0));
CREATE TABLE $S.Logentries (id char(16), nid char(16), text text);
CREATE TABLE $S.Breakpoints (id timestamp (0) PRIMARY KEY);
CREATE TABLE $S.histories (nid char(16) PRIMARY KEY, chunkids char(12)[], entryids char(16)[]);

-- A 09:00-10:00, B 10:00-11:00 (3 entries, a Breakpoint), C 11:00-12:20 (1 entry),
-- E 12:30-12:30 (zero length), F 12:40-14:00
INSERT INTO $S.Logchunks VALUES
  ('2026-11-19 09:00', '20200101000000.1', '2026-11-19 10:00'),
  ('2026-11-19 10:00', '20200101000000.2', '2026-11-19 11:00'),
  ('2026-11-19 11:00', '20200101000000.1', '2026-11-19 12:20'),
  ('2026-11-19 12:30', '20200101000000.2', '2026-11-19 12:30'),
  ('2026-11-19 12:40', '20200101000000.1', '2026-11-19 14:00');
INSERT INTO $S.Logentries VALUES
  ('202611190900.001', '20200101000000.1', \$txt\$Entry in A\$txt\$),
  ('202611191000.001', '20200101000000.2', \$txt\$First entry in B\$txt\$),
  ('202611191000.002', '20200101000000.3', \$txt\$Second entry in B, own Node, it's quoted\$txt\$),
  ('202611191000.003', '20200101000000.2', \$txt\$Third entry in B\$txt\$),
  ('202611191100.001', '20200101000000.1', \$txt\$Entry in C\$txt\$);
INSERT INTO $S.Breakpoints VALUES ('2026-11-19 10:00');
INSERT INTO $S.histories VALUES
  ('20200101000000.1', ARRAY['202611190900','202611191100'], ARRAY['202611190900.001','202611191100.001']),
  ('20200101000000.2', ARRAY['202611191000'], ARRAY['202611191000.001','202611191000.003']),
  ('20200101000000.3', '{}', ARRAY['202611191000.002']);
-- Makes the Logentries part of moving C to 11:05 fail, to test that nothing changes then.
ALTER TABLE $S.Logentries ADD CONSTRAINT test_block_1105 CHECK (substr(id, 1, 12) <> '202611191105');
EOF
[ $? -eq 0 ] || { echo "Setting up the test tables failed."; exit 1; }

# One query (psql -c shows only the result of the last statement in a string).
snapshot() { q "SELECT line FROM (
                  SELECT 1 AS k, 'chunk '||to_char(id,'YYYYMMDDHH24MI')||' '||nid||' '||to_char(tclose,'YYYYMMDDHH24MI') AS line FROM $S.Logchunks
                  UNION ALL SELECT 2, 'entry '||id||' '||nid||' '||text FROM $S.Logentries
                  UNION ALL SELECT 3, 'break '||to_char(id,'YYYYMMDDHH24MI') FROM $S.Breakpoints
                  UNION ALL SELECT 4, 'hist  '||nid||' '||chunkids::text||' '||entryids::text FROM $S.histories
                ) AS t ORDER BY k, line;"; }

fails=0
check() { # description, expected, actual
    if [ "$2" = "$3" ]; then echo "  ok:   $1"; else echo "  FAIL: $1"; echo "        expected: $2"; echo "        got:      $3"; fails=$((fails+1)); fi
}
# Runs fzlog as the modify page does (without -q), shows its output, and keeps it in $LASTOUT.
LASTOUT=$(mktemp)
run_fzlog() { "$FZLOG" -d "$DB" -s "$S" -E STDOUT -W STDOUT -m "$1" -1 "$2" > "$LASTOUT" 2>&1; local r=$?; sed 's/^/        | /' "$LASTOUT" | head -8; return $r; }

echo "=== Before"
snapshot | sed 's/^/  /'
before="$(snapshot)"

echo ""
echo "=== 1. Refused: B open 10:00 -> 09:58 (before A's close 10:00)"
run_fzlog 202611191000 202611190958; check "fzlog exits with an error" "1" "$([ $? -ne 0 ] && echo 1 || echo 0)"
check "nothing changed" "$before" "$(snapshot)"

echo ""
echo "=== 2. Refused: F open 12:40 -> 12:30 (not after E's open 12:30, zero-length E)"
run_fzlog 202611191240 202611191230; check "fzlog exits with an error" "1" "$([ $? -ne 0 ] && echo 1 || echo 0)"
check "nothing changed" "$before" "$(snapshot)"

echo ""
echo "=== 3. Refused: B open -> 11:30 (after its close 11:00)"
run_fzlog 202611191000 202611191130; check "fzlog exits with an error" "1" "$([ $? -ne 0 ] && echo 1 || echo 0)"
check "nothing changed" "$before" "$(snapshot)"

echo ""
echo "=== 4. Refused by the database: C open 11:00 -> 11:05, with the test constraint blocking the entry ID change"
run_fzlog 202611191100 202611191105; check "fzlog exits with an error" "1" "$([ $? -ne 0 ] && echo 1 || echo 0)"
check "nothing changed (chunk ID change rolled back too)" "$before" "$(snapshot)"

echo ""
echo "=== 5. Allowed: B open 10:00 -> 10:05 (B has 3 entries)"
run_fzlog 202611191000 202611191005; check "fzlog succeeds" "0" "$?"
check "B's chunk ID is 10:05, close and Node unchanged" "202611191005|20200101000000.2|202611191100" \
      "$(q "SELECT to_char(id,'YYYYMMDDHH24MI')||'|'||nid||'|'||to_char(tclose,'YYYYMMDDHH24MI') FROM $S.Logchunks WHERE id='2026-11-19 10:05'")"
check "no chunk left at 10:00" "0" "$(q "SELECT count(*) FROM $S.Logchunks WHERE id='2026-11-19 10:00'")"
check "B's entries have the new IDs, same Nodes, texts and order" \
      "202611191005.001|20200101000000.2|First entry in B
202611191005.002|20200101000000.3|Second entry in B, own Node, it's quoted
202611191005.003|20200101000000.2|Third entry in B" \
      "$(q "SELECT id||'|'||nid||'|'||text FROM $S.Logentries WHERE id LIKE '202611191005.%' ORDER BY id")"
check "no entries left with the old chunk ID" "0" "$(q "SELECT count(*) FROM $S.Logentries WHERE id LIKE '202611191000.%'")"
check "Breakpoint moved to 10:05" "202611191005" "$(q "SELECT to_char(id,'YYYYMMDDHH24MI') FROM $S.Breakpoints")"
check "histories cache not changed (it is rebuilt with fzquerypq -R histories)" \
      "$(echo "$before" | grep '^hist ')" "$(snapshot | grep '^hist ')"
check "fzlog's output mentions refreshing Node histories" "1" "$(grep -c 'Refresh Node histories' "$LASTOUT")"
check "chunks A, C, E, F and their entries unchanged" \
      "$(echo "$before" | grep -E '^(chunk 2026111909|chunk 2026111911|chunk 2026111912|entry 202611190900|entry 202611191100|hist  20200101000000.1)')" \
      "$(snapshot | grep -E '^(chunk 2026111909|chunk 2026111911|chunk 2026111912|entry 202611190900|entry 202611191100|hist  20200101000000.1)')"

echo ""
echo "=== 6. Allowed: B open 10:05 -> 10:00 (back)"
run_fzlog 202611191005 202611191000; check "fzlog succeeds" "0" "$?"
check "everything is as before the tests" "$before" "$(snapshot)"

echo ""
echo "=== After"
snapshot | sed 's/^/  /'

echo ""
if [ $fails -eq 0 ]; then echo "All checks passed."; else echo "$fails check(s) FAILED."; fi
exit $fails
