#!/bin/bash
BASE="http://localhost:8080"
echo "=== StreamForge Smoke Test ==="

echo "1. Health check..."
curl -s $BASE/health | python3 -m json.tool

echo "2. Sending 10 test events..."
for i in $(seq 1 10); do
    curl -s -X POST $BASE/ingest \
        -H "Content-Type: application/json" \
        -d "{\"source\":\"smoke_test\",\"metric_name\":\"latency\",\"value\":$((RANDOM % 100))}" \
        && echo " [event $i sent]"
done

echo "3. Metrics check..."
curl -s $BASE/metrics

echo "4. Final health check..."
curl -s $BASE/health | python3 -m json.tool

echo "=== Smoke test complete ==="
