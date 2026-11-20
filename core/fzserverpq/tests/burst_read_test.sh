#! /bin/bash
# burst_read_test.sh
#
# Send many read-only requests to a running fzserverpq at the same time, check that
# every one is answered, and report how long they took. Nothing is changed in the
# Graph or the database: only status, address, time zone and Node parameter reads
# are requested.
#
# Usage: burst_read_test.sh [number-of-requests] [port] [node-id] [node-id]
#   defaults: 50 requests, port 8090, Nodes 20261102153502.1 and 20260606095139.1
#
# Written for the fzserverpq thread-safety work (Node 20261102153502.1).

N=${1:-50}
PORT=${2:-8090}
NODE_A=${3:-20261102153502.1}
NODE_B=${4:-20260606095139.1}
HOST="http://127.0.0.1:$PORT"

paths=(
    "/fz/status"
    "/fz/ipport"
    "/fz/tzadjust"
    "/fz/graph/nodes/$NODE_A/targetdate.raw"
    "/fz/graph/nodes/$NODE_A/valuation.raw"
    "/fz/graph/nodes/$NODE_A/required.raw"
    "/fz/graph/nodes/$NODE_B/completion.raw"
    "/fz/graph/nodes/$NODE_B/topics.raw"
    "/fz/graph/nodes/$NODE_B/text.raw"
)

results=$(mktemp)
trap 'rm -f "$results"' EXIT

echo "Checking that the server answers on port $PORT..."
code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 "$HOST/fz/status")
if [ "$code" != "200" ]; then
    echo "The server did not answer /fz/status with 200 (got: '$code'). Stopping."
    exit 1
fi

echo "Sending $N read-only requests at the same time..."
start=$(date +%s.%N)
for i in $(seq 1 "$N"); do
    p=${paths[$((i % ${#paths[@]}))]}
    (
        out=$(curl -s -o /dev/null -w '%{http_code} %{time_total}' --max-time 30 "$HOST$p")
        echo "$i $out $p" >> "$results"
    ) &
done
wait
end=$(date +%s.%N)

echo ""
awk -v n="$N" -v wall="$(echo "$end - $start" | bc)" '
{
    total++
    if ($2 == "200") ok++; else { failed++; if (failed <= 5) print "  not answered with 200: " $4 " (code " $2 ")" }
    t[total] = $3
    if ($3 >= 1.0) slow++
}
END {
    asort(t)
    printf "Requests sent:          %d\n", n
    printf "Results recorded:       %d\n", total
    printf "Answered with 200:      %d\n", ok
    printf "Failed or no answer:    %d\n", failed + (n - total)
    printf "Whole burst (seconds):  %.2f\n", wall
    printf "Per request (seconds):  min %.3f, median %.3f, max %.3f\n", t[1], t[int((total+1)/2)], t[total]
    printf "Requests taking >= 1 s: %d\n", slow
}' "$results"

echo ""
echo "Checking that the server still answers..."
code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 10 "$HOST/fz/status")
if [ "$code" = "200" ]; then echo "Server still answers: OK"; else echo "Server did not answer /fz/status afterwards (got: '$code')."; fi
