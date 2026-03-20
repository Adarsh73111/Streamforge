#!/bin/bash
# StreamForge Live Demo — works in LOCAL mode (no AWS needed)
# Usage: bash scripts/demo.sh

set -e
BINARY="./build/streamforge"
BASE_URL="http://localhost:8080"
API_URL="http://localhost:9090"

if [ ! -f "$BINARY" ]; then
  echo "Binary not found. Run: mkdir -p build && cd build && cmake .. && make streamforge -j$(nproc)"
  exit 1
fi

# Start server in local mode (background)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  StreamForge Live Demo"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
$BINARY --local &
SERVER_PID=$!
sleep 2

echo ""
echo "▶ Step 1 — Health check"
curl -s $API_URL/health | python3 -m json.tool

echo ""
echo "▶ Step 2 — Sending 60 normal events (building AI baseline)..."
for i in $(seq 1 60); do
  curl -s -X POST $BASE_URL/ingest \
    -H "Content-Type: application/json" \
    -d "{\"source\":\"demo\",\"metric_name\":\"latency\",\"value\":$((48 + RANDOM % 8)),\"timestamp\":$(date +%s%3N)}" > /dev/null
done
echo "  60 normal events sent — baseline established"

echo ""
echo "▶ Step 3 — Injecting 5 anomaly spikes (value=600, 12x normal)..."
for i in 1 2 3 4 5; do
  curl -s -X POST $BASE_URL/ingest \
    -H "Content-Type: application/json" \
    -d "{\"source\":\"demo\",\"metric_name\":\"latency\",\"value\":600,\"timestamp\":$(date +%s%3N)}" > /dev/null
  sleep 0.3
done
echo "  5 spikes injected — check server output above for [LOCAL] Anomaly lines"

sleep 1
echo ""
echo "▶ Step 4 — Query pipeline stats"
curl -s $API_URL/health | python3 -m json.tool

echo ""
echo "▶ Step 5 — Prometheus metrics"
curl -s $API_URL/metrics

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Demo complete — stopping server"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
kill $SERVER_PID 2>/dev/null || true
