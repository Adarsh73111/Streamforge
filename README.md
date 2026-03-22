<div align="center">

<img src="https://img.shields.io/badge/StreamForge-v2.1-0077b6?style=for-the-badge&logoColor=white" />

# StreamForge ⚡

### Intelligent Real-Time Data Analytics Pipeline

[![CI](https://github.com/Adarsh73111/Streamforge/actions/workflows/ci.yml/badge.svg)](https://github.com/Adarsh73111/Streamforge/actions)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00b4d8?style=flat&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![AWS](https://img.shields.io/badge/AWS-Free%20Tier-FF9900?style=flat&logo=amazonaws&logoColor=white)](https://aws.amazon.com/free/)
[![License](https://img.shields.io/badge/license-MIT-2ecc71?style=flat)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Docker-lightgrey?style=flat)](https://www.docker.com/)
[![Throughput](https://img.shields.io/badge/throughput-18%2C274%20req%2Fsec-brightgreen?style=flat)](#-benchmark)

**18,274 req/sec &nbsp;·&nbsp; 24.6x faster than Python &nbsp;·&nbsp; Pure C++ AI &nbsp;·&nbsp; Runs locally with zero AWS**

[**⚡ Quickstart**](#-quickstart) &nbsp;·&nbsp;
[**💻 Local Mode**](#-run-locally--no-aws-needed) &nbsp;·&nbsp;
[**🐳 Docker**](#-docker) &nbsp;·&nbsp;
[**☁️ AWS Mode**](#️-aws-cloud-mode) &nbsp;·&nbsp;
[**📡 API**](#-api-reference) &nbsp;·&nbsp;
[**📊 Benchmark**](#-benchmark)

</div>

---

## 📖 What is StreamForge?

StreamForge is a **production-grade, AI-powered streaming data pipeline** built entirely in **C++17**.  
It ingests live event streams at high throughput, runs three anomaly detection algorithms **natively in C++** (no Python sidecar), and — when deployed on AWS — archives to S3, logs to DynamoDB, and fires real-time email alerts via SNS.

> 💡 Think of it as a highway tollbooth that in real time counts cars, measures speed, detects overweight trucks, and alerts authorities — without stopping traffic. StreamForge does the same for data flowing through your system.

### Two Modes. One Binary.

| Mode | What Runs | Who It's For |
|------|-----------|-------------|
| `--local` | HTTP server + AI pipeline + Query API | **Anyone** — no AWS account needed |
| AWS mode | Everything + S3 + DynamoDB + SNS alerts | Cloud deployments |

---

## ⚡ Quickstart

### 🖥️ Option A — Local Mode (Laptop / No AWS)
```bash
# Clone
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge

# Install deps (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev

# Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make streamforge -j$(nproc)
cd ..

# Run — no AWS required
./build/streamforge --local

# In a new terminal — run the live demo
bash scripts/demo.sh
```

### 🐳 Option B — Docker (Easiest)
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
docker compose up
```

### ☁️ Option C — AWS EC2 (One Command)
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
bash scripts/deploy_ec2.sh
```

---

## 💻 Run Locally — No AWS Needed

StreamForge runs fully on your laptop in `--local` mode:
```
✅ HTTP ingestion server    (port 8080)
✅ Lock-free ring buffer
✅ AI anomaly detection     (Z-score + EWMA + Isolation Forest)
✅ REST Query API           (port 9090)
✅ Prometheus /metrics
❌ S3 archiving             (skipped — no AWS)
❌ DynamoDB logging         (skipped — no AWS)
❌ SNS email alerts         (skipped — no AWS)
```
```bash
# Start
./build/streamforge --local

# Send an event
curl -X POST http://localhost:8080/ingest \
  -H "Content-Type: application/json" \
  -d '{"source":"my-app","metric_name":"latency","value":52,"timestamp":0}'

# Check health
curl http://localhost:9090/health

# Prometheus metrics
curl http://localhost:9090/metrics

# Run automated demo (builds baseline + injects anomalies)
bash scripts/demo.sh
```

**Expected demo output:**
```
▶ Step 1 — Health check
{ "status": "ok", "events_processed": 0 }

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
# Easiest — local mode, no AWS
docker compose up

# Manual build
docker build -t streamforge .
docker run -p 8080:8080 -p 9090:9090 streamforge --local
```

---

## ☁️ AWS Cloud Mode

Requires an AWS account. All services stay within Free Tier.

### Setup
```bash
# 1. Launch EC2 t2.micro (Amazon Linux 2023)
#    Attach IAM role: AmazonS3FullAccess + AmazonDynamoDBFullAccess + AmazonSNSFullAccess

# 2. Create AWS resources
aws s3 mb s3://streamforge-events-yourname
aws dynamodb create-table --table-name StreamMetrics \
  --attribute-definitions \
    AttributeName=metric_name,AttributeType=S \
    AttributeName=timestamp,AttributeType=S \
  --key-schema \
    AttributeName=metric_name,KeyType=HASH \
    AttributeName=timestamp,KeyType=RANGE \
  --billing-mode PAY_PER_REQUEST

aws dynamodb create-table --table-name AnomalyLog \
  --attribute-definitions AttributeName=anomaly_id,AttributeType=S \
  --key-schema AttributeName=anomaly_id,KeyType=HASH \
  --billing-mode PAY_PER_REQUEST

aws sns create-topic --name StreamForgeAlerts

# 3. Edit src/main.cpp — update S3_BUCKET and SNS_ARN

# 4. One-command deploy
bash scripts/deploy_ec2.sh
```

### AWS Free Tier Safety

| Service | StreamForge Usage | Free Tier Limit | Safe? |
|---------|------------------|----------------|-------|
| EC2 t2.micro | Runs entire pipeline | 750 hrs/month | ✅ |
| S3 | Raw event archives | 5 GB storage | ✅ |
| DynamoDB | Metrics + anomaly logs | 25 GB + 25 WCU | ✅ |
| SNS | Email alerts | 1M publishes/month | ✅ |

---

## 🏗️ Architecture
```
POST /ingest
      │
      ▼
┌─────────────────┐
│  HTTP Server    │  Crow :8080  + mutex guard
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  RingBuffer<T>  │  lock-free SPSC · std::atomic
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  ThreadPool     │  4 worker threads
│  StreamProc     │  dispatcher loop
└────────┬────────┘
         │
┌────────▼────────┐
│ AnomalyDetector │
│  Z-score  O(1)  │  spike detection
│  EWMA     O(1)  │  drift detection
│  IsoForest      │  outlier detection
│  Vote: 2 of 3   │
└────────┬────────┘
         │
   ┌─────┼─────┐
   ▼     ▼     ▼
  S3  Dynamo  SNS

GET /health | /metrics | /anomalies | /query
      │
      ▼
┌─────────────────┐
│  QueryServer    │  Crow :9090
└─────────────────┘
```

---

## 🤖 AI Anomaly Detection

Three algorithms run natively in C++ on **every single event** — no Python, no subprocess:

| Algorithm | Detects | Complexity | File |
|-----------|---------|------------|------|
| **Z-score** | Sudden spikes | O(1) | `ZScoreDetector.hpp` |
| **EWMA** | Slow drift | O(1) | `EWMADetector.hpp` |
| **Isolation Forest** | Statistical outliers | O(n log n) | `IsolationForest.hpp` |

**Voting policy:** Anomaly fires only when **2 of 3 detectors agree** — minimising false positives.
```
value=52  →  Z:NO   E:NO   F:NO   →  NORMAL
value=550 →  Z:YES  E:NO   F:YES  →  ANOMALY  (2/3 agree)
value=55  →  Z:NO   E:NO   F:NO   →  NORMAL
```

---

## 📡 API Reference

### Ingestion — port 8080

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/ingest` | POST | Ingest a JSON event → 202 Accepted |
| `/health` | GET | Server health + counters |

**Event schema:**
```json
{
  "source":      "my-app",
  "metric_name": "latency",
  "value":       52.3,
  "timestamp":   1700000000
}
```

### Query API — port 9090

| Endpoint | Method | Returns |
|----------|--------|---------|
| `/health` | GET | Events processed, dropped, anomalies detected |
| `/metrics` | GET | Prometheus-format counters (Grafana compatible) |
| `/anomalies` | GET | Last 20 anomalies from DynamoDB |
| `/query?metric=X` | GET | Rolling mean / stddev / min / max |

---

## 📊 Benchmark

Stress tested on **EC2 t2.micro** — 1,000 events, 10 concurrent connections.  
Both servers ran **identical AI anomaly detection logic** on every event.

| Implementation | Req/sec | Avg Latency | p99 Latency | Errors |
|----------------|---------|-------------|-------------|--------|
| **⚡ C++ StreamForge** | **18,274** | **0.5ms** | **3.1ms** | **0** |
| 🐍 Python FastAPI | 743 | 13.4ms | 19.3ms | 0 |

> **StreamForge is 24.6x faster than Python on identical hardware.**

Reproduce it yourself:
```bash
# Terminal 1
./build/streamforge --local

# Terminal 2
pip install fastapi uvicorn
python3 benchmark/python_server.py &
bash benchmark/run_benchmark.sh
```

---

## 🧪 Tests

7 unit tests — all passing. CI runs on every push.
```bash
cd build
make test_rb test_pipeline test_anomaly -j$(nproc)
./test_rb && ./test_pipeline && ./test_anomaly
```

| Test | Scenario | Result |
|------|----------|--------|
| RingBuffer basic | Push/pop ordering | ✅ PASS |
| RingBuffer concurrent | 500 concurrent pushes — no corruption | ✅ PASS |
| Pipeline end-to-end | 200 events, 0 dropped | ✅ PASS |
| Normal data | 500 events — zero false positives | ✅ 0/500 |
| Spike detection | 5 injected spikes after baseline | ✅ 4/5 |
| Drift detection | Gradual increase 30→105 over 50 events | ✅ Detected |
| Multi-metric | Spike on A doesn't affect B | ✅ PASS |

---

## 💡 Real-World Use Cases

| Use Case | Monitors | Alerts When |
|----------|----------|-------------|
| **API Health** | Response time, error rate | p99 latency spikes at 2am |
| **E-commerce** | Order rate, payment failures | Volume drops 40% suddenly |
| **IoT Sensors** | Temperature, pressure | Reading leaves normal range |
| **Cybersecurity** | Login attempts, API hits | Brute force spike detected |
| **Game Servers** | Player events, crash reports | Error rate spikes after deploy |
| **Finance** | Transaction volume, amounts | Unusual frequency or size |

---

## 📁 Project Structure
```
streamforge/
├── src/
│   ├── ingestion/
│   │   ├── RingBuffer.hpp        ← Lock-free SPSC queue
│   │   └── HttpServer.hpp        ← Crow HTTP server :8080
│   ├── pipeline/
│   │   ├── ThreadPool.hpp        ← Worker thread pool
│   │   └── StreamProcessor.hpp  ← Pipeline orchestrator
│   ├── ai/
│   │   ├── ZScoreDetector.hpp    ← Spike detection O(1)
│   │   ├── EWMADetector.hpp      ← Drift detection O(1)
│   │   ├── IsolationForest.hpp   ← Outlier detection
│   │   └── AnomalyDetector.hpp   ← 2-of-3 voting
│   ├── aws/
│   │   ├── S3Uploader.hpp        ← Batch event archiver
│   │   ├── DynamoWriter.hpp      ← DynamoDB logger
│   │   └── SNSNotifier.hpp       ← Email alerts
│   ├── api/
│   │   └── QueryServer.hpp       ← REST Query API :9090
│   └── main.cpp                  ← Entry point (--local flag)
├── tests/                        ← 7 unit tests
├── benchmark/                    ← C++ vs Python stress test
├── scripts/
│   ├── deploy_ec2.sh             ← One-command AWS deploy
│   └── demo.sh                   ← Live demo (local mode)
├── Dockerfile
├── docker-compose.yml
└── .github/workflows/ci.yml      ← GitHub Actions CI
```

---

## 🔑 What Makes This Unique

| Feature | Detail |
|---------|--------|
| **Pure C++ AI** | Isolation Forest + EWMA + Z-score in-process — no Python sidecar |
| **Lock-free pipeline** | `RingBuffer<T>` with `std::atomic` — zero mutex contention |
| **Local mode** | Full pipeline on any laptop — no cloud account needed |
| **Self-benchmarking** | `/metrics` exposes throughput + anomaly rate in Prometheus format |
| **One-command deploy** | `bash scripts/deploy_ec2.sh` bootstraps everything on EC2 |
| **Rare stack** | C++ + AWS SDK — almost no open-source pipelines do this |

---

## 📋 Requirements

**Local mode:**
- GCC 12+ or Clang 14+
- CMake 3.20+
- `libssl-dev`, `libcurl4-openssl-dev`

**AWS mode (additionally):**
- AWS account (Free Tier sufficient)
- IAM role: S3 + DynamoDB + SNS permissions
- `aws-sdk-cpp` (installed automatically by `deploy_ec2.sh`)

---

## 🤝 Contributing
```bash
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./test_rb && ./test_pipeline && ./test_anomaly
```

All PRs must pass the 7 existing tests. Add tests for new components.

---

<div align="center">

Built in C++17 &nbsp;·&nbsp; Deployed on AWS Free Tier &nbsp;·&nbsp; Mumbai ap-south-1 &nbsp;·&nbsp; 2026

**If this helped you learn C++ systems programming or AWS integration, consider giving it a ⭐**

[![Clone](https://img.shields.io/badge/Clone%20this%20repo-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/Adarsh73111/Streamforge)

</div>
