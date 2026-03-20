<div align="center">
```
  ____  _                      _____
 / ___|| |_ _ __ ___  __ _ _ _|  ___|__  _ __ __ _  ___
 \___ \| __| '__/ _ \/ _` | '_| |_ / _ \| '__/ _` |/ _ \
  ___) | |_| | |  __/ (_| | | |  _| (_) | | | (_| |  __/
 |____/ \__|_|  \___|\__,_|_| |_|  \___/|_|  \__, |\___|
                                               |___/
```

### Intelligent Real-Time Data Analytics Pipeline

![CI](https://github.com/Adarsh73111/Streamforge/actions/workflows/ci.yml/badge.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00b4d8?style=flat&logo=cplusplus)
![AWS](https://img.shields.io/badge/AWS-Free%20Tier-FF9900?style=flat&logo=amazonaws)
![License](https://img.shields.io/badge/license-MIT-2ecc71?style=flat)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Docker-lightgrey?style=flat)
![Throughput](https://img.shields.io/badge/throughput-18%2C274%20req%2Fsec-brightgreen?style=flat)

**18,274 req/sec · 24.6x faster than Python · Pure C++ AI · Zero dependencies for local mode**

[Quickstart](#-quickstart) · [Local Mode](#-run-locally-no-aws-needed) · [Docker](#-docker) · [AWS Mode](#-aws-cloud-mode) · [API Reference](#-api-reference) · [Benchmark](#-benchmark)

</div>

---

## What is StreamForge?

StreamForge is a **production-grade, AI-powered streaming data pipeline** built entirely in C++17. It processes live event streams at high throughput, detects anomalies in real time using three algorithms running natively in C++, and — when configured for AWS — archives data to S3, logs to DynamoDB, and fires email alerts via SNS.

> Think of it as a busy highway tollbooth that in real time counts cars, measures speed, detects if a truck is overweight, and alerts authorities — without stopping traffic. StreamForge does the same for data flowing through your system.

**Two modes. One binary.**

| Mode | What runs | Who it's for |
|---|---|---|
| `--local` | HTTP server + AI pipeline + Query API | Anyone — no AWS account needed |
| AWS mode | Everything + S3 + DynamoDB + SNS alerts | Cloud deployments |

---

## ⚡ Quickstart

### Option A — Local mode (laptop / no AWS)
```bash
# 1. Clone
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge

# 2. Install deps (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev

# 3. Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make streamforge -j$(nproc)
cd ..

# 4. Run in local mode (no AWS required)
./build/streamforge --local

# 5. In a new terminal — run the live demo
bash scripts/demo.sh
```

### Option B — Docker (easiest)
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
docker compose up
```

### Option C — AWS EC2 (one command)
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
bash scripts/deploy_ec2.sh
```

---

## 🖥 Run Locally — No AWS Needed

StreamForge runs fully on your laptop in `--local` mode. The complete pipeline is active:
```
✅ HTTP ingestion server  (port 8080)
✅ Lock-free ring buffer
✅ AI anomaly detection   (Z-score + EWMA + Isolation Forest)
✅ REST Query API         (port 9090)
✅ Prometheus /metrics
❌ S3 archiving           (skipped — no AWS)
❌ DynamoDB logging       (skipped — no AWS)
❌ SNS email alerts       (skipped — no AWS)
```
```bash
# Start
./build/streamforge --local

# Send events
curl -X POST http://localhost:8080/ingest \
  -H "Content-Type: application/json" \
  -d '{"source":"my-app","metric_name":"latency","value":52,"timestamp":0}'

# Query live stats
curl http://localhost:9090/health
curl http://localhost:9090/metrics

# Run the full automated demo
bash scripts/demo.sh
```

**Expected demo output:**
```
▶ Step 1 — Health check
{ "status": "ok", "events_processed": 0, ... }

▶ Step 2 — Sending 60 normal events (building AI baseline)...
  60 normal events sent — baseline established

▶ Step 3 — Injecting 5 anomaly spikes (value=600)...
[LOCAL] Anomaly: metric=latency value=600 votes=[Z-F] score=0.609
[LOCAL] Anomaly: metric=latency value=600 votes=[Z-F] score=0.497
[LOCAL] Anomaly: metric=latency value=600 votes=[Z-F] score=0.443

▶ Step 4 — Query pipeline stats
{ "status": "ok", "events_processed": 65, "anomalies_detected": 3 }
```

---

## 🐳 Docker
```bash
# Build and run (local mode — no AWS)
docker compose up

# Or build manually
docker build -t streamforge .
docker run -p 8080:8080 -p 9090:9090 streamforge --local
```

---

## ☁ AWS Cloud Mode

Requires an AWS account (all services stay within Free Tier).

### AWS Setup
```bash
# 1. Launch EC2 t2.micro (Amazon Linux 2023) — attach IAM role with:
#    AmazonS3FullAccess + AmazonDynamoDBFullAccess + AmazonSNSFullAccess

# 2. Create AWS resources
aws s3 mb s3://streamforge-events-yourname
aws dynamodb create-table --table-name StreamMetrics \
  --attribute-definitions AttributeName=metric_name,AttributeType=S \
                           AttributeName=timestamp,AttributeType=S \
  --key-schema AttributeName=metric_name,KeyType=HASH \
               AttributeName=timestamp,KeyType=RANGE \
  --billing-mode PAY_PER_REQUEST
aws dynamodb create-table --table-name AnomalyLog \
  --attribute-definitions AttributeName=anomaly_id,AttributeType=S \
  --key-schema AttributeName=anomaly_id,KeyType=HASH \
  --billing-mode PAY_PER_REQUEST
aws sns create-topic --name StreamForgeAlerts

# 3. Edit src/main.cpp — update S3_BUCKET and SNS_ARN with your values

# 4. One-command deploy
bash scripts/deploy_ec2.sh
```

### AWS Free Tier Safety

| Service | StreamForge Usage | Free Tier Limit | Safe? |
|---|---|---|---|
| EC2 t2.micro | Runs entire pipeline | 750 hrs/month | ✅ |
| S3 | Raw event archives | 5 GB storage | ✅ |
| DynamoDB | Metrics + anomaly logs | 25 GB + 25 WCU | ✅ |
| SNS | Email alerts | 1M publishes/month | ✅ |

---

## 🏗 Architecture
```
                    POST /ingest
                         │
                         ▼
              ┌─────────────────────┐
              │   Crow HTTP Server  │  :8080
              │   + mutex guard     │
              └──────────┬──────────┘
                         │
                         ▼
              ┌─────────────────────┐
              │  RingBuffer<T,1024> │  lock-free SPSC
              │  std::atomic        │  zero contention
              └──────────┬──────────┘
                         │
                         ▼
              ┌─────────────────────┐
              │    ThreadPool       │  4 worker threads
              │    StreamProcessor  │  dispatcher loop
              └──────────┬──────────┘
                         │
              ┌──────────▼──────────┐
              │   AnomalyDetector   │
              │  ┌───────────────┐  │
              │  │ Z-score  O(1) │  │  spike detection
              │  │ EWMA     O(1) │  │  drift detection
              │  │ IsoForest    │  │  outlier detection
              │  │ Vote: 2 of 3 │  │
              │  └───────────────┘  │
              └──────────┬──────────┘
                         │
           ┌─────────────┼─────────────┐
           ▼             ▼             ▼
    ┌────────────┐ ┌──────────┐ ┌──────────┐
    │ S3Uploader │ │  Dynamo  │ │   SNS    │
    │ batch→S3   │ │  Writer  │ │ Notifier │
    │ 20 events  │ │ anomaly+ │ │  email   │
    └────────────┘ │ metrics  │ │ alerts   │
                   └──────────┘ └──────────┘

    GET /health | /metrics | /anomalies | /query
                         │
                         ▼
              ┌─────────────────────┐
              │    QueryServer      │  :9090
              │    Crow + DynamoDB  │
              └─────────────────────┘
```

---

## 🤖 AI Anomaly Detection

Three algorithms run natively in C++ on every single event — no Python, no subprocess:

| Algorithm | Detects | Complexity | File |
|---|---|---|---|
| **Z-score** | Sudden spikes — value deviates sharply from recent mean | O(1) | `ZScoreDetector.hpp` |
| **EWMA** | Slow drift — long-term mean shifts from baseline | O(1) | `EWMADetector.hpp` |
| **Isolation Forest** | Statistical outliers — learns what normal looks like | O(n log n) | `IsolationForest.hpp` |

**Voting policy:** All three vote independently. Anomaly fires only when **2 of 3 agree** — minimising false positives while maintaining sensitivity.
```
Event value=52  → Z:NO  E:NO  F:NO  → NORMAL
Event value=550 → Z:YES E:NO  F:YES → ANOMALY (2/3 agree)
Event value=55  → Z:NO  E:NO  F:NO  → NORMAL
```

---

## 📡 API Reference

### Ingestion Server — port 8080

| Endpoint | Method | Body | Response |
|---|---|---|---|
| `/ingest` | POST | `{"source":"app","metric_name":"latency","value":52,"timestamp":0}` | 202 Accepted |
| `/health` | GET | — | `{"status":"ok","processed":N,"dropped":N}` |

### Query API — port 9090

| Endpoint | Method | Returns |
|---|---|---|
| `/health` | GET | Pipeline counters — events processed, dropped, anomalies detected |
| `/metrics` | GET | Prometheus-format counters (compatible with Grafana) |
| `/anomalies` | GET | Last 20 anomalies from DynamoDB as JSON |
| `/query?metric=X` | GET | Rolling mean / stddev / min / max for a metric |

### Example calls
```bash
# Ingest an event
curl -X POST http://localhost:8080/ingest \
  -H "Content-Type: application/json" \
  -d '{"source":"api","metric_name":"response_time","value":143,"timestamp":0}'

# Check pipeline health
curl http://localhost:9090/health

# Prometheus metrics
curl http://localhost:9090/metrics

# Query rolling stats
curl "http://localhost:9090/query?metric=response_time"

# View recent anomalies
curl http://localhost:9090/anomalies
```

---

## 📊 Benchmark

Stress tested on EC2 t2.micro — 1,000 events at 10 concurrent connections.  
Both servers ran **identical AI anomaly detection logic** on every event.

| Implementation | Req/sec | Avg Latency | p99 Latency | Errors |
|---|---|---|---|---|
| **⚡ C++ StreamForge** | **18,274** | **0.5ms** | **3.1ms** | **0** |
| 🐍 Python FastAPI | 743 | 13.4ms | 19.3ms | 0 |

**StreamForge is 24.6x faster than Python FastAPI on identical hardware.**

- C++ p99 latency is **6x lower** (3.1ms vs 19.3ms)
- Zero dropped events at full load
- AI runs in-process — no IPC or language boundary overhead

Reproduce the benchmark yourself:
```bash
# Terminal 1 — C++ server
./build/streamforge --local

# Terminal 2 — Python server
pip install fastapi uvicorn
python3 benchmark/python_server.py &

# Run benchmark
bash benchmark/run_benchmark.sh
```

---

## 🧪 Tests

7 unit tests — all passing. CI runs automatically on every push.
```bash
# Build and run all tests
cd build
make test_rb test_pipeline test_anomaly -j$(nproc)
./test_rb        # RingBuffer: push/pop, concurrent safety
./test_pipeline  # Pipeline: 200 events, zero dropped
./test_anomaly   # AI: normal data, spike detection, drift, multi-metric
```

| Test | Scenario | Result |
|---|---|---|
| RingBuffer basic | Push 7 items, pop in order | ✅ PASS |
| RingBuffer concurrent | 500 concurrent pushes — no corruption | ✅ PASS |
| Pipeline end-to-end | 200 events ingested, 0 dropped | ✅ PASS |
| Normal data | 500 events ~50 — zero false positives | ✅ 0/500 |
| Spike detection | 5 injected spikes after baseline | ✅ 4/5 |
| Drift detection | Gradual increase 30→105 over 50 events | ✅ Detected |
| Multi-metric | Spike on metric A doesn't affect metric B | ✅ PASS |

---

## 💡 Real-World Use Cases

| Use Case | What StreamForge monitors | Alert fires when |
|---|---|---|
| **API Health** | Response time, error rate, throughput | p99 latency spikes at 2am |
| **E-commerce** | Order rate, payment failures | Order volume drops 40% suddenly |
| **IoT Sensors** | Temperature, pressure, humidity | Sensor reading leaves normal range |
| **Cybersecurity** | Login attempts, API hit patterns | Brute force spike detected |
| **Game Servers** | Player events, crash reports, errors | Error rate spikes after a deploy |
| **Finance** | Transaction volume, amounts | Unusual frequency or size pattern |

---

## 📁 Project Structure
```
streamforge/
├── src/
│   ├── ingestion/
│   │   ├── RingBuffer.hpp       ← Lock-free SPSC queue (std::atomic)
│   │   └── HttpServer.hpp       ← Crow HTTP server :8080
│   ├── pipeline/
│   │   ├── ThreadPool.hpp       ← Worker thread pool
│   │   └── StreamProcessor.hpp ← Pipeline orchestrator
│   ├── ai/
│   │   ├── ZScoreDetector.hpp   ← Spike detection O(1)
│   │   ├── EWMADetector.hpp     ← Drift detection O(1)
│   │   ├── IsolationForest.hpp  ← Outlier detection
│   │   └── AnomalyDetector.hpp  ← 2-of-3 voting orchestrator
│   ├── aws/
│   │   ├── S3Uploader.hpp       ← Batch event archiver
│   │   ├── DynamoWriter.hpp     ← DynamoDB metrics + anomaly logger
│   │   └── SNSNotifier.hpp      ← Email alert system
│   ├── api/
│   │   └── QueryServer.hpp      ← REST Query API :9090
│   └── main.cpp                 ← Entry point (--local flag support)
├── tests/                       ← 7 unit tests
├── benchmark/                   ← C++ vs Python stress test
├── scripts/
│   ├── deploy_ec2.sh            ← One-command AWS deploy
│   └── demo.sh                  ← Live demo script (local mode)
├── Dockerfile                   ← Multi-stage build
├── docker-compose.yml           ← Local Docker setup
└── .github/workflows/ci.yml     ← GitHub Actions CI
```

---

## 🔧 What Makes This Unique

- **Pure C++ AI** — Isolation Forest, EWMA, Z-score run natively in-process. No Python sidecar, no subprocess, no IPC overhead.
- **Lock-free hot path** — Custom `RingBuffer<T>` using `std::atomic` with zero mutex contention between ingestion and processing threads.
- **Local mode** — Full pipeline runs on any laptop with no AWS account required.
- **Self-benchmarking** — Built-in `/metrics` endpoint exposes throughput, anomaly rate, and drop rate in Prometheus format.
- **One-command deploy** — `bash scripts/deploy_ec2.sh` bootstraps everything from a fresh EC2 instance.
- **C++ on AWS** — Almost no open-source streaming pipelines use C++ with native AWS SDK integration.

---

## 📋 Requirements

**Local mode (no AWS):**
- GCC 12+ or Clang 14+
- CMake 3.20+
- OpenSSL dev headers
- libcurl dev headers

**AWS mode (additionally):**
- AWS account (Free Tier sufficient)
- IAM role with S3 + DynamoDB + SNS permissions
- aws-sdk-cpp (built by deploy_ec2.sh automatically)

---

## 🚀 Contributing
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./test_rb && ./test_pipeline && ./test_anomaly
```

All PRs must pass the existing 7 tests. Add tests for new components.

---

<div align="center">

Built with C++17 · Deployed on AWS Free Tier · ap-south-1 Mumbai · 2026

**[⭐ Star this repo](https://github.com/Adarsh73111/Streamforge)** if it helped you learn C++ systems programming or AWS integration.

</div>
