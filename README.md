# StreamForge ⚡

![CI](https://github.com/Adarsh73111/Streamforge/actions/workflows/ci.yml/badge.svg)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![AWS](https://img.shields.io/badge/AWS-Free%20Tier-orange.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**Production-grade, AI-powered real-time data analytics pipeline — built entirely in C++17 on AWS Free Tier.**

Receives event streams at high throughput, runs three anomaly detection algorithms natively in C++, archives to S3, logs to DynamoDB, and fires SNS alerts — all on a t2.micro instance.

---

## Architecture
```
HTTP POST /ingest
       │
       ▼
┌─────────────────┐     ┌──────────────────┐     ┌─────────────┐
│  Crow HTTP      │────▶│  Lock-free        │────▶│ ThreadPool  │
│  Server :8080   │     │  RingBuffer<1024> │     │ 4 workers   │
└─────────────────┘     └──────────────────┘     └──────┬──────┘
                                                         │
                                              ┌──────────▼──────────┐
                                              │   AnomalyDetector   │
                                              │  Z-score + EWMA     │
                                              │  + IsolationForest  │
                                              │  Voting: 2 of 3     │
                                              └──────────┬──────────┘
                                                         │
                              ┌──────────────────────────┼──────────────────────┐
                              ▼                          ▼                      ▼
                       ┌─────────────┐          ┌──────────────┐       ┌──────────────┐
                       │ S3Uploader  │          │ DynamoWriter │       │ SNSNotifier  │
                       │ batch→S3    │          │ metrics+     │       │ email alerts │
                       │             │          │ anomalies    │       │              │
                       └─────────────┘          └──────────────┘       └──────────────┘

GET /query /health /metrics /anomalies
       │
       ▼
┌─────────────────┐
│  QueryServer    │
│  Crow :9090     │
└─────────────────┘
```

## Quickstart — Run on your own EC2 in 4 commands
```bash
# 1. Launch EC2 t2.micro (Amazon Linux 2023) and connect via Instance Connect
# 2. Clone
git clone https://github.com/Adarsh73111/Streamforge.git ~/streamforge

# 3. Deploy (installs deps, builds, starts as systemd service)
cd ~/streamforge && bash scripts/deploy_ec2.sh

# 4. Test
curl -X POST http://localhost:8080/ingest \
  -H "Content-Type: application/json" \
  -d '{"source":"app","metric_name":"latency","value":52,"timestamp":0}'
curl http://localhost:9090/health
```

---

## API Reference

### Ingestion
| Endpoint | Method | Description |
|---|---|---|
| `/ingest` | POST | Ingest a JSON event |
| `/health` | GET | Ingestion server health |

### Query API (port 9090)
| Endpoint | Method | Description |
|---|---|---|
| `/health` | GET | Pipeline counters — events, anomalies, drops |
| `/metrics` | GET | Prometheus-compatible counters |
| `/anomalies` | GET | Last 20 anomalies from DynamoDB |
| `/query?metric=X` | GET | Rolling stats for a metric |

### Event Schema
```json
{
  "source":      "my-app",
  "metric_name": "latency",
  "value":       52.3,
  "timestamp":   1700000000
}
```

---

## AI Anomaly Detection

Three algorithms run natively in C++ on every event — no Python sidecar:

| Algorithm | Detects | Complexity |
|---|---|---|
| Z-score | Sudden spikes | O(1) |
| EWMA | Slow drift | O(1) |
| Isolation Forest | Statistical outliers | O(n log n) |

**Voting policy:** 2 of 3 must agree → anomaly flagged. Minimises false positives.

---

## Technology Stack

| Category | Tool |
|---|---|
| Language | C++17 |
| HTTP server | Crow (header-only) |
| AWS SDK | aws-sdk-cpp (S3, DynamoDB, SNS) |
| Build | CMake 3.20+ |
| CI/CD | GitHub Actions |
| Cloud | AWS Free Tier (EC2 t2.micro) |

---

## AWS Free Tier Usage

| Service | Usage | Safe? |
|---|---|---|
| EC2 t2.micro | Runs entire pipeline | ✅ |
| S3 | Raw event archives | ✅ |
| DynamoDB | Metrics + anomaly logs | ✅ |
| SNS | Email alerts | ✅ |

---

## Project Structure
```
streamforge/
├── src/
│   ├── ingestion/     # RingBuffer, HttpServer
│   ├── pipeline/      # StreamProcessor, ThreadPool
│   ├── ai/            # ZScore, EWMA, IsolationForest, AnomalyDetector
│   ├── aws/           # S3Uploader, DynamoWriter, SNSNotifier
│   └── api/           # QueryServer (REST, Prometheus)
├── tests/             # 7 tests — all passing
├── scripts/           # deploy_ec2.sh, smoke tests
├── Dockerfile
└── CMakeLists.txt
```

---

## What Makes This Unique

- **Pure C++ AI** — Isolation Forest, EWMA, Z-score run in the same process as the pipeline
- **Lock-free hot path** — zero mutex contention between ingestion and processing threads
- **REST Query API** — live analytics over HTTP with Prometheus `/metrics` endpoint
- **One-command deploy** — `bash scripts/deploy_ec2.sh` bootstraps everything
- **C++ on AWS** — almost nobody does this; most pipelines are Python or Java

---

*Built on AWS Free Tier · C++17 · EC2 ap-south-1 · 2026*

## Benchmark — C++ vs Python FastAPI

Stress tested on EC2 t2.micro — 1,000 events, 10 concurrent connections:

| Implementation | Req/sec | Avg latency | p99 latency | Errors |
|---|---|---|---|---|
| **C++ StreamForge** | **18,274** | **0.5ms** | **3.1ms** | **0** |
| Python FastAPI | 743 | 13.4ms | 19.3ms | 0 |

**C++ StreamForge is 24.6x faster than Python FastAPI** on identical hardware.

Key differences:
- C++ p99 latency is **6x lower** (3.1ms vs 19.3ms)
- C++ processes requests **24x faster** at sustained throughput
- Both run AI anomaly detection on every event — C++ does it natively in-process
