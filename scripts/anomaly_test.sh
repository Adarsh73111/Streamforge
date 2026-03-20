#!/bin/bash
BASE="http://localhost:8080"
echo "=== StreamForge Live Anomaly Detection Test ==="

echo "--- Sending 100 normal events (value 45-55) ---"
for i in $(seq 1 100); do
    VALUE=$((45 + RANDOM % 10))
    curl -s -X POST $BASE/ingest \
        -H "Content-Type: application/json" \
        -d "{\"source\":\"test\",\"metric_name\":\"latency\",\"value\":$VALUE}" > /dev/null
done
echo "100 normal events sent"
sleep 1

echo "--- Injecting 5 spike anomalies (value 480-520) ---"
for i in $(seq 1 5); do
    VALUE=$((480 + RANDOM % 40))
    RESP=$(curl -s -X POST $BASE/ingest \
        -H "Content-Type: application/json" \
        -d "{\"source\":\"test\",\"metric_name\":\"latency\",\"value\":$VALUE}")
    echo "  Spike $i (value=$VALUE): $RESP"
done
sleep 1

echo "--- Final health check ---"
curl -s $BASE/health | python3 -m json.tool
echo "=== Check server tab for [ANOMALY] lines ==="
