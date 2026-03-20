#!/bin/bash
set -e

EVENTS=1000
CONCURRENCY=10
CPP_URL="http://localhost:8080/ingest"
PY_URL="http://localhost:8081/ingest"
PAYLOAD='{"source":"bench","metric_name":"latency","value":52,"timestamp":0}'

echo "================================================"
echo "  StreamForge Benchmark — C++ vs Python"
echo "  Events: $EVENTS  Concurrency: $CONCURRENCY"
echo "================================================"

# Install hey (HTTP load tool) if not present
if ! command -v hey &>/dev/null; then
    echo "Installing hey load tester..."
    wget -q https://hey-release.s3.us-east-2.amazonaws.com/hey_linux_amd64 -O /tmp/hey
    chmod +x /tmp/hey
    sudo mv /tmp/hey /usr/local/bin/hey
fi

# ── C++ benchmark ──────────────────────────────────
echo ""
echo ">>> Benchmarking C++ StreamForge on port 8080..."
CPP_RESULT=$(hey -n $EVENTS -c $CONCURRENCY \
    -m POST \
    -H "Content-Type: application/json" \
    -d "$PAYLOAD" \
    "$CPP_URL" 2>&1)
echo "$CPP_RESULT"

CPP_RPS=$(echo "$CPP_RESULT"   | grep "Requests/sec" | awk '{print $2}')
CPP_P99=$(echo "$CPP_RESULT"   | grep "99%" | awk '{print $2}')
CPP_MEAN=$(echo "$CPP_RESULT"  | grep "Average" | awk '{print $2}')

# ── Python benchmark ───────────────────────────────
echo ""
echo ">>> Benchmarking Python FastAPI on port 8081..."
PY_RESULT=$(hey -n $EVENTS -c $CONCURRENCY \
    -m POST \
    -H "Content-Type: application/json" \
    -d "$PAYLOAD" \
    "$PY_URL" 2>&1)
echo "$PY_RESULT"

PY_RPS=$(echo "$PY_RESULT"   | grep "Requests/sec" | awk '{print $2}')
PY_P99=$(echo "$PY_RESULT"   | grep "99%" | awk '{print $2}')
PY_MEAN=$(echo "$PY_RESULT"  | grep "Average" | awk '{print $2}')

# ── Summary ────────────────────────────────────────
echo ""
echo "================================================"
echo "  BENCHMARK RESULTS — $EVENTS events, $CONCURRENCY concurrent"
echo "================================================"
printf "%-20s %-15s %-15s %-15s\n" "Implementation" "Req/sec" "Avg latency" "p99 latency"
printf "%-20s %-15s %-15s %-15s\n" "--------------" "-------" "-----------" "-----------"
printf "%-20s %-15s %-15s %-15s\n" "C++ StreamForge" "$CPP_RPS" "${CPP_MEAN}s" "${CPP_P99}s"
printf "%-20s %-15s %-15s %-15s\n" "Python FastAPI"  "$PY_RPS"  "${PY_MEAN}s" "${PY_P99}s"
echo "================================================"
echo ""

# Speedup calculation
if command -v python3 &>/dev/null && [ -n "$CPP_RPS" ] && [ -n "$PY_RPS" ]; then
    SPEEDUP=$(python3 -c "print(f'{float('$CPP_RPS')/float('$PY_RPS'):.1f}x faster')" 2>/dev/null || echo "see above")
    echo "  C++ is $SPEEDUP than Python"
fi
echo "================================================"
