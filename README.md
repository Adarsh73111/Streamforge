<div align="center">

<img src="https://img.shields.io/badge/StreamForge-v3.0-0077b6?style=for-the-badge&logoColor=white" />

# StreamForge ⚡

### Production-Grade Real-Time Data Analytics Pipeline

[![CI](https://github.com/Adarsh73111/Streamforge/actions/workflows/ci.yml/badge.svg)](https://github.com/Adarsh73111/Streamforge/actions)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00b4d8?style=flat&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![AWS](https://img.shields.io/badge/AWS-Free%20Tier-FF9900?style=flat&logo=amazonaws&logoColor=white)](https://aws.amazon.com/free/)
[![License](https://img.shields.io/badge/license-MIT-2ecc71?style=flat)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Docker-lightgrey?style=flat)](https://www.docker.com/)
[![Throughput](https://img.shields.io/badge/throughput-18%2C274%20req%2Fsec-brightgreen?style=flat)](#-benchmark)

**18,274 req/sec &nbsp;·&nbsp; 24.6x faster than Python &nbsp;·&nbsp; Pure C++ AI &nbsp;·&nbsp; Live Cyberpunk Dashboard &nbsp;·&nbsp; Zero AWS required to run**

[**⚡ Quickstart**](#-quickstart) &nbsp;·&nbsp;
[**💻 Local Mode**](#-local-mode--no-aws-needed) &nbsp;·&nbsp;
[**🐳 Docker**](#-docker) &nbsp;·&nbsp;
[**☁️ AWS Mode**](#️-aws-cloud-mode) &nbsp;·&nbsp;
[**📡 API**](#-api-reference) &nbsp;·&nbsp;
[**📊 Benchmark**](#-benchmark) &nbsp;·&nbsp;
[**🗺️ Versions**](#️-version-history)

</div>

---

## 📖 What is StreamForge?

StreamForge is a **production-grade, AI-powered streaming data pipeline** built entirely in **C++17**.

It ingests live event streams at extreme throughput, runs **three anomaly detection algorithms natively in C++** — no Python, no sidecar — and supports two modes: fully local on any laptop, or fully deployed on AWS with S3 archiving, DynamoDB logging, and real-time SNS email alerts.

> 💡 Think of it as a highway tollbooth that in real time counts cars, measures speed, detects overweight trucks, and alerts authorities — without stopping traffic. StreamForge does the same for data flowing through your system.

### Two Modes. One Binary.

| Mode | What Runs | Who It's For |
|------|-----------|-------------|
| `--local` | Full pipeline + AI + Dashboard + Query API | **Anyone** — no AWS, no account needed |
| AWS mode | Everything + S3 + DynamoDB + SNS email alerts | Cloud deployments |

### What You Get

```
⚡ 18,274 req/sec sustained throughput
🤖 3 AI detectors: Z-score + EWMA + Isolation Forest (2-of-3 voting)
📊 Live cyberpunk dashboard — auto-refreshes every 5 seconds
🔔 Real AWS email alerts when anomaly detected
📦 Unlimited metrics tracked simultaneously with full isolation
⚙️  Fully configurable via config.json — no recompiling needed
🐳 Docker support — one command to run
🔁 GitHub Actions CI — always green
```

---

## ⚡ Quickstart

Choose the option that works best for you:

### Option A — Local Mode on Linux/Ubuntu (Recommended for first run)

```bash
# 1. Clone the repo
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge

# 2. Install build dependencies
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake \
  libssl-dev libcurl4-openssl-dev \
  nlohmann-json3-dev

# 3. Install AWS SDK C++ (needed even for local mode — takes ~10 mins)
sudo apt-get install -y libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev
git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git /tmp/aws-sdk-cpp
mkdir -p /tmp/aws-sdk-build && cd /tmp/aws-sdk-build
cmake /tmp/aws-sdk-cpp \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_ONLY="s3;dynamodb;sns" \
  -DENABLE_TESTING=OFF \
  -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
cd -

# 4. Install Crow HTTP library
git clone https://github.com/CrowCpp/Crow.git /tmp/crow
sudo cp -r /tmp/crow/include/crow* /usr/local/include/
sudo apt-get install -y libasio-dev
sudo chmod -R 755 /usr/local/include/crow
sudo chmod 644 /usr/local/include/crow.h

# 5. Build StreamForge
mkdir -p build && cd build
cmake ..
make streamforge -j$(nproc)
cd ..

# 6. Run in local mode — no AWS needed!
./build/streamforge --local
```

Open a second terminal and test:
```bash
# Send an event
curl -X POST http://localhost:8080/ingest \
  -d '{"source":"my-app","metric_name":"latency","value":52}'

# Check health
curl http://localhost:9090/health

# Open dashboard
# http://localhost:8090
```

---

### Option B — Docker (Easiest — One Command)

```bash
# Clone
git clone https://github.com/Adarsh73111/Streamforge.git
cd Streamforge

# Run with Docker Compose (local mode, no AWS needed)
docker compose up
```

That's it. StreamForge starts on:
- `http://localhost:8080` — event ingestion
- `http://localhost:9090` — query API
- `http://localhost:8090` — live dashboard

Manual Docker build:
```bash
docker build -t streamforge .
docker run -p 8080:8080 -p 9090:9090 -p 8090:8090 streamforge --local
```

---

### Option C — GitHub Codespaces (Zero local setup)

1. Click the green **Code** button on this repo
2. Click **Codespaces** → **Create codespace on main**
3. Wait ~2 minutes for the container to build
4. In the terminal, build and run:

```bash
# Install AWS SDK (one time, ~10 mins)
sudo apt-get install -y libcurl4-openssl-dev libssl-dev uuid-dev zlib1g-dev libasio-dev nlohmann-json3-dev
git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git /tmp/aws-sdk-cpp
mkdir -p /tmp/aws-sdk-build && cd /tmp/aws-sdk-build
cmake /tmp/aws-sdk-cpp -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="s3;dynamodb;sns" -DENABLE_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc) && sudo make install
cd /workspaces/Streamforge

# Install Crow
git clone https://github.com/CrowCpp/Crow.git /tmp/crow
sudo cp -r /tmp/crow/include/crow* /usr/local/include/
sudo chmod -R 755 /usr/local/include/crow && sudo chmod 644 /usr/local/include/crow.h

# Build
mkdir -p build && cd build && cmake .. && make streamforge -j$(nproc)

# Run
./streamforge --local
```

Open **Ports tab** → click port **8090** link to see live dashboard.

---

## 💻 Local Mode — No AWS Needed

StreamForge runs **100% fully** on any laptop with `--local`:

```
✅ HTTP ingestion server    (port 8080)
✅ Lock-free ring buffer    (zero contention)
✅ 3 AI anomaly detectors   (Z-score + EWMA + Isolation Forest)
✅ Multi-metric tracking    (unlimited metrics, fully isolated)
✅ Live cyberpunk dashboard (port 8090, auto-refresh 5s)
✅ REST Query API           (port 9090)
✅ Prometheus /metrics      (Grafana compatible)
✅ Custom thresholds        (per-metric via /config API)
✅ Cluster node info        (GET /nodes)
❌ S3 archiving             (skipped in local mode)
❌ DynamoDB logging         (skipped in local mode)
❌ SNS email alerts         (skipped in local mode)
```

### Full local demo:

```bash
# Terminal 1 — Start server
./build/streamforge --local

# Terminal 2 — Build AI baseline (60 normal events)
for i in {1..60}; do
  curl -s -X POST http://localhost:8080/ingest \
    -d "{\"source\":\"api\",\"metric_name\":\"response_time\",\"value\":$((50 + RANDOM % 20))}" > /dev/null
done
echo "Baseline built!"

# Inject anomaly spikes
for i in {1..5}; do
  curl -s -X POST http://localhost:8080/ingest \
    -d '{"source":"api","metric_name":"response_time","value":9999}' > /dev/null
done
echo "Check dashboard at http://localhost:8090"

# Send multiple metrics simultaneously
curl -X POST http://localhost:8080/ingest \
  -d '{"source":"api","metric_name":"error_rate","value":0.02}'
curl -X POST http://localhost:8080/ingest \
  -d '{"source":"db","metric_name":"query_time","value":12}'

# See all active metrics
curl http://localhost:9090/metrics/list

# Set custom threshold for a metric
curl -X POST "http://localhost:9090/config?metric=response_time&threshold=2.0&cooldown=30"

# Check cluster node status
curl http://localhost:9090/nodes
```

---

## 🐳 Docker

```bash
# Easiest — local mode, no AWS needed
docker compose up

# With environment variables for AWS mode
docker run \
  -e AWS_ACCESS_KEY_ID=your_key \
  -e AWS_SECRET_ACCESS_KEY=your_secret \
  -e AWS_DEFAULT_REGION=ap-south-1 \
  -p 8080:8080 -p 9090:9090 -p 8090:8090 \
  streamforge

# Build fresh image
docker build -t streamforge .
docker run -p 8080:8080 -p 9090:9090 -p 8090:8090 streamforge --local
```

---

## ☁️ AWS Cloud Mode

Full AWS mode sends real email alerts, archives events to S3, and logs anomalies to DynamoDB. **All within AWS Free Tier.**

### Step 1 — Create IAM User

Go to **AWS Console → IAM → Users → Create User**

Name: `streamforge-user`

Attach these policies:
```
AmazonS3FullAccess
AmazonDynamoDBFullAccess
AmazonSNSFullAccess
```

Create access keys → save both keys securely.

### Step 2 — Create AWS Resources

```bash
# Configure AWS CLI
aws configure
# Enter: Access Key ID, Secret Key, Region (ap-south-1), output format (json)

# Create S3 bucket (replace 'yourname' with something unique)
aws s3 mb s3://streamforge-events-yourname --region ap-south-1

# Create DynamoDB tables
aws dynamodb create-table \
  --table-name AnomalyLog \
  --attribute-definitions AttributeName=anomaly_id,AttributeType=S \
  --key-schema AttributeName=anomaly_id,KeyType=HASH \
  --billing-mode PAY_PER_REQUEST \
  --region ap-south-1

aws dynamodb create-table \
  --table-name StreamMetrics \
  --attribute-definitions \
    AttributeName=metric_name,AttributeType=S \
    AttributeName=timestamp,AttributeType=S \
  --key-schema \
    AttributeName=metric_name,KeyType=HASH \
    AttributeName=timestamp,KeyType=RANGE \
  --billing-mode PAY_PER_REQUEST \
  --region ap-south-1

# Create SNS topic and get ARN
aws sns create-topic --name StreamForgeAlerts --region ap-south-1

# Subscribe your email to alerts
aws sns subscribe \
  --topic-arn YOUR_SNS_ARN \
  --protocol email \
  --notification-endpoint your@email.com \
  --region ap-south-1
# Check your email and confirm the subscription!
```

### Step 3 — Configure StreamForge

Edit `config.json`:
```json
{
  "region": "ap-south-1",
  "s3_bucket": "streamforge-events-yourname",
  "sns_arn": "arn:aws:sns:ap-south-1:YOUR_ACCOUNT_ID:StreamForgeAlerts",
  "batch_size": 20,
  "flush_secs": 30,
  "workers": 4,
  "port_ingest": 8080,
  "port_query": 9090,
  "port_dashboard": 8090,
  "zscore_threshold": 3.0,
  "node_id": "node-1",
  "cluster_mode": 0
}
```

### Step 4 — Run in AWS Mode

```bash
# Set AWS credentials as environment variables
export AWS_ACCESS_KEY_ID=your_access_key
export AWS_SECRET_ACCESS_KEY=your_secret_key
export AWS_DEFAULT_REGION=ap-south-1

# Run without --local flag
./build/streamforge

# Send events — anomalies will trigger real emails!
curl -X POST http://localhost:8080/ingest \
  -d '{"source":"api","metric_name":"latency","value":52}'
```

### AWS Free Tier Safety

| Service | StreamForge Usage | Free Tier Limit | Safe? |
|---------|------------------|----------------|-------|
| S3 | Raw event archives | 5 GB storage | ✅ |
| DynamoDB | Metrics + anomaly logs | 25 GB + 25 WCU | ✅ |
| SNS | Email alerts | 1M publishes/month | ✅ |
| EC2 t2.micro | Optional — run on cloud | 750 hrs/month | ✅ |

---

## 📊 Live Dashboard

StreamForge includes a **cyberpunk-themed live dashboard** on port 8090.

```
http://localhost:8090
```

**Dashboard features:**
- Live event counter with neon glow animation
- Anomaly counter — flashes red alert banner when AI detects anomaly
- Real-time throughput chart (Chart.js, updates every 5 seconds)
- Active metrics panel — shows all live metric streams and their sources
- Anomaly log table — last 50 anomalies with metric, source, value, score, votes, timestamp
- Works in both local mode and AWS mode

---

## 🤖 AI Anomaly Detection

Three algorithms run natively in C++ on **every single event** — no Python, no subprocess, no IPC cost:

| Algorithm | Detects | Complexity | File |
|-----------|---------|------------|------|
| **Z-score** | Sudden spikes | O(1) | `ZScoreDetector.hpp` |
| **EWMA** | Slow drift | O(1) | `EWMADetector.hpp` |
| **Isolation Forest** | Statistical outliers | O(n log n) | `IsolationForest.hpp` |

**Voting policy:** Anomaly fires only when **2 of 3 detectors agree** — minimising false positives.

```
value=52  →  Z:NO   E:NO   F:NO   →  NORMAL
value=550 →  Z:YES  E:NO   F:YES  →  ANOMALY  (2/3 agreed)
value=55  →  Z:NO   E:NO   F:NO   →  NORMAL
```

**Per-metric isolation:** Each metric+source combination gets its own independent detector state. A spike on `cpu_usage` never affects the baseline for `response_time`.

---

## 📡 API Reference

### Ingestion Server — port 8080

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/ingest` | POST | Ingest a JSON event → 202 Accepted |

**Event schema:**
```json
{
  "source":      "my-app",
  "metric_name": "latency",
  "value":       52.3,
  "timestamp":   0
}
```

### Query API — port 9090

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/health` | GET | Events processed, dropped, anomalies detected |
| `/version` | GET | Build version, language, mode |
| `/metrics` | GET | Prometheus-format counters (Grafana compatible) |
| `/metrics/list` | GET | All active metric streams with sources |
| `/anomalies` | GET | Last 20 anomalies from DynamoDB (AWS mode) |
| `/query?metric=X&source=Y` | GET | Rolling stats filtered by metric + source |
| `/config` | GET | View all custom thresholds |
| `/config` | POST | Set custom threshold + cooldown for a metric |
| `/config/{metric}` | DELETE | Reset metric to default threshold |
| `/nodes` | GET | Cluster node status and leader info |

### Dashboard API — port 8090

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Live cyberpunk dashboard |
| `/api/health` | GET | Pipeline stats (used by dashboard) |
| `/api/anomalies` | GET | Last 50 anomalies (used by dashboard) |
| `/api/metrics-list` | GET | Active metrics (used by dashboard) |

### Custom Threshold Examples

```bash
# Set strict threshold for payment failures (alert at 1.5 sigma)
curl -X POST "http://localhost:9090/config?metric=payment_failures&threshold=1.5&cooldown=60"

# Set relaxed threshold for CPU (alert at 4.0 sigma)
curl -X POST "http://localhost:9090/config?metric=cpu_usage&threshold=4.0&cooldown=300"

# View all current thresholds
curl http://localhost:9090/config

# Reset to default (3.0 sigma)
curl -X DELETE http://localhost:9090/config/cpu_usage
```

---

## ⚙️ Configuration

All settings live in `config.json` — no recompiling needed:

```json
{
  "region":           "ap-south-1",
  "s3_bucket":        "streamforge-events-yourname",
  "sns_arn":          "arn:aws:sns:ap-south-1:ACCOUNT:StreamForgeAlerts",
  "batch_size":       20,
  "flush_secs":       30,
  "workers":          4,
  "port_ingest":      8080,
  "port_query":       9090,
  "port_dashboard":   8090,
  "zscore_threshold": 3.0,
  "node_id":          "node-1",
  "cluster_mode":     0
}
```

| Field | Default | Description |
|-------|---------|-------------|
| `region` | `ap-south-1` | AWS region |
| `s3_bucket` | — | Your S3 bucket name |
| `sns_arn` | — | Your SNS topic ARN for email alerts |
| `batch_size` | `20` | Events per S3 upload batch |
| `workers` | `4` | Thread pool size |
| `port_ingest` | `8080` | HTTP ingestion port |
| `port_query` | `9090` | Query API port |
| `port_dashboard` | `8090` | Dashboard port |
| `zscore_threshold` | `3.0` | Default anomaly sensitivity (sigma) |
| `node_id` | `node-1` | Node identity for cluster mode |
| `cluster_mode` | `0` | 0 = single node, 1 = distributed |

---

## 📊 Benchmark

Stress tested on **EC2 t2.micro** — 1,000 events, 10 concurrent connections. Both servers ran identical AI anomaly detection logic on every event.

| Implementation | Req/sec | Avg Latency | p99 Latency | Errors |
|----------------|---------|-------------|-------------|--------|
| **⚡ C++ StreamForge** | **18,274** | **0.5ms** | **3.1ms** | **0** |
| 🐍 Python FastAPI | 743 | 13.4ms | 19.3ms | 0 |

> **StreamForge is 24.6x faster than Python on identical hardware.**

The gap comes from zero GIL contention, no interpreter overhead, and AI running in-process with no IPC cost.

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

7 unit tests — all passing. CI runs on every push to main.

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

## 🏗️ Architecture

```
POST /ingest
      │
      ▼
┌─────────────────┐
│  HTTP Server    │  Crow :8080
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  RingBuffer<T>  │  lock-free · std::atomic · alignas(64)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  ThreadPool     │  N worker threads (configurable)
│  StreamProc     │  dispatcher loop
└────────┬────────┘
         │
┌────────▼────────┐
│ AnomalyDetector │  per-metric isolation
│  Z-score  O(1)  │  spike detection
│  EWMA     O(1)  │  drift detection
│  IsoForest      │  outlier detection
│  Vote: 2 of 3   │  low false positives
│  Thresholds     │  custom per-metric
└────────┬────────┘
         │
   ┌─────┼──────┐
   ▼     ▼      ▼
  S3  Dynamo   SNS
         │
         ▼
┌─────────────────┐
│ DashboardServer │  Crow :8090 — cyberpunk UI
└─────────────────┘
         │
┌─────────────────┐
│  QueryServer    │  Crow :9090 — REST API
└─────────────────┘
         │
┌─────────────────┐
│  NodeManager    │  cluster identity + leader election
└─────────────────┘
```

---

## 📁 Project Structure

```
Streamforge/
├── src/
│   ├── ingestion/
│   │   ├── RingBuffer.hpp        ← Lock-free SPSC queue (std::atomic)
│   │   └── HttpServer.hpp        ← Crow HTTP server :8080
│   ├── pipeline/
│   │   ├── ThreadPool.hpp        ← Worker thread pool
│   │   └── StreamProcessor.hpp  ← Pipeline orchestrator + metric tracking
│   ├── ai/
│   │   ├── ZScoreDetector.hpp    ← Spike detection O(1)
│   │   ├── EWMADetector.hpp      ← Drift detection O(1)
│   │   ├── IsolationForest.hpp   ← Outlier detection O(n log n)
│   │   ├── AnomalyDetector.hpp   ← 2-of-3 voting + threshold management
│   │   └── ThresholdManager.hpp  ← Per-metric thresholds + cooldown
│   ├── aws/
│   │   ├── S3Uploader.hpp        ← Batch event archiver
│   │   ├── DynamoWriter.hpp      ← DynamoDB anomaly + metrics logger
│   │   └── SNSNotifier.hpp       ← Real-time email alerts
│   ├── api/
│   │   ├── QueryServer.hpp       ← REST Query API :9090
│   │   └── DashboardServer.hpp   ← Cyberpunk live dashboard :8090
│   ├── cluster/
│   │   ├── NodeManager.hpp       ← Node identity + uptime tracking
│   │   └── LeaderElection.hpp    ← Leader node owns SNS alerts
│   ├── Config.hpp                ← config.json loader
│   └── main.cpp                  ← Entry point
├── terraform/
│   └── main.tf                   ← Multi-node AWS infrastructure as code
├── tests/
│   ├── test_ringbuffer.cpp
│   ├── test_pipeline.cpp
│   └── test_anomaly.cpp
├── benchmark/
│   ├── python_server.py          ← Python FastAPI for comparison
│   └── run_benchmark.sh          ← Stress test script
├── scripts/
│   ├── deploy_ec2.sh             ← One-command AWS EC2 deploy
│   └── demo.sh                   ← Live demo (local mode)
├── .devcontainer/
│   └── devcontainer.json         ← GitHub Codespaces config
├── .github/
│   └── workflows/ci.yml          ← GitHub Actions CI
├── config.json                   ← All settings (edit this, not source code)
├── Dockerfile
└── docker-compose.yml
```

---

## 🗺️ Version History

| Version | Name | What Was Added |
|---------|------|----------------|
| **v1.0** | Core Pipeline | Lock-free pipeline, 18,274 req/sec, Z-score + EWMA + IsoForest AI, S3 + DynamoDB + SNS, Docker, CI |
| **v1.1** | Config & Polish | `config.json` — no more hardcoded values, `/version` endpoint, graceful shutdown |
| **v1.2** | Multi-Metric | Unlimited metrics simultaneously, per-metric isolation, source grouping, `/metrics/list` |
| **v2.0** | Web Dashboard | Live cyberpunk dashboard on :8090, real-time Chart.js graphs, anomaly log table |
| **v2.1** | Custom Thresholds | Per-metric Z-score threshold via API, alert cooldown, `POST/GET/DELETE /config` |
| **v3.0** | Distributed Mode | NodeManager, LeaderElection, `/nodes` endpoint, Terraform scripts for multi-EC2 |

All versions permanently available at [github.com/Adarsh73111/Streamforge/releases](https://github.com/Adarsh73111/Streamforge/releases)

---

## 🌍 Real-World Use Cases

| Use Case | Monitors | Alerts When |
|----------|----------|-------------|
| **API Health** | Response time, error rate | p99 latency spikes unexpectedly |
| **E-commerce** | Order rate, payment failures | Volume drops 40% suddenly |
| **IoT Sensors** | Temperature, pressure, humidity | Reading leaves normal range |
| **Cybersecurity** | Login attempts, API hits per second | Brute force spike detected |
| **Game Servers** | Player events, crash reports | Error rate spikes after deploy |
| **Finance** | Transaction volume, amounts | Unusual frequency or transaction size |

---

## 🔑 What Makes This Unique

| Feature | Detail |
|---------|--------|
| **Pure C++ AI** | IsolationForest + EWMA + Z-score in-process — no Python, no sidecar, no IPC |
| **Lock-free pipeline** | `RingBuffer<T>` with `std::atomic` and `alignas(64)` — zero false sharing |
| **Local mode** | Full pipeline on any laptop — no cloud account, no signup, no cost |
| **Per-metric isolation** | Each metric has its own AI baseline — cross-metric pollution is impossible |
| **Live dashboard** | Cyberpunk UI, neon animations, real-time chart, anomaly log |
| **Fully configurable** | `config.json` changes ports, thresholds, workers — no recompiling |
| **Self-benchmarking** | `/metrics` exposes throughput in Prometheus format — Grafana compatible |
| **Distributed ready** | Terraform scripts, leader election, `/nodes` API for multi-node clusters |
| **Rare stack** | C++17 + AWS SDK — almost no open-source pipelines do this |

---

## 📋 Requirements

**Local mode (minimum):**
- Ubuntu 20.04+ / Debian / WSL2
- GCC 11+ or Clang 14+
- CMake 3.20+
- `libssl-dev`, `libcurl4-openssl-dev`, `libasio-dev`, `nlohmann-json3-dev`
- AWS SDK C++ (built from source — see Quickstart)
- Crow HTTP library (header-only — see Quickstart)

**AWS mode (additionally):**
- AWS account (Free Tier sufficient)
- IAM user with: S3 + DynamoDB + SNS permissions
- AWS CLI configured with your credentials

**Docker mode:**
- Docker + Docker Compose installed

---

## 🤝 Contributing

```bash
# Fork and clone
git clone https://github.com/YOUR_USERNAME/Streamforge.git
cd Streamforge

# Build in debug mode
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run all tests — must pass before submitting PR
./test_rb && ./test_pipeline && ./test_anomaly

# Run in local mode to verify
./streamforge --local
```

All pull requests must pass the 7 existing tests. Add tests for any new components. CI runs automatically on every push.

---

## ⚠️ Troubleshooting

**Build fails with "AWSSDK not found"**
→ AWS SDK C++ is not installed. Follow Step 3 in the Quickstart section — it must be built from source.

**Build fails with "crow.h: No such file or directory"**
→ Run: `git clone https://github.com/CrowCpp/Crow.git /tmp/crow && sudo cp -r /tmp/crow/include/crow* /usr/local/include/ && sudo chmod -R 755 /usr/local/include/crow`

**"Permission denied" on crow headers**
→ Run: `sudo chmod -R 755 /usr/local/include/crow && sudo chmod 644 /usr/local/include/crow.h`

**"Address already in use" on startup**
→ Another instance is running. Kill it: `pkill -f streamforge`

**Dashboard shows "-" for all stats**
→ The dashboard fetches from `/api/health` on the same port. Make sure you opened the dashboard through the correct URL, not by typing localhost directly if using Codespaces.

**SNS emails not arriving**
→ Check your email for the AWS subscription confirmation and click the confirm link.

---

<div align="center">

Built in C++17 &nbsp;·&nbsp; AWS Free Tier &nbsp;·&nbsp; Mumbai ap-south-1 &nbsp;·&nbsp; 2026

**If this project helped you learn C++ systems programming or AWS integration, consider giving it a ⭐**

[![Clone](https://img.shields.io/badge/Clone%20this%20repo-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/Adarsh73111/Streamforge)
[![Releases](https://img.shields.io/badge/View%20All%20Releases-0077b6?style=for-the-badge&logo=github&logoColor=white)](https://github.com/Adarsh73111/Streamforge/releases)

</div>
