#include "ingestion/HttpServer.hpp"
#include "pipeline/StreamProcessor.hpp"
#include "api/QueryServer.hpp"
#include "api/DashboardServer.hpp"
#include "Config.hpp"
#include <aws/core/Aws.h>
#include "aws/S3Uploader.hpp"
#include "aws/DynamoWriter.hpp"
#include "aws/SNSNotifier.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <mutex>
#include <string>

std::atomic<bool> running{true};
void signal_handler(int) {
    std::cout << "\n[StreamForge] Shutting down gracefully..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    bool local_mode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--local") local_mode = true;
    }

    Config cfg = Config::load("config.json");

    std::cout << R"(
  ____  _                      _____
 / ___|| |_ _ __ ___  __ _ _ _|  ___|__  _ __ __ _  ___
 \___ \| __| '__/ _ \/ _` | '_| |_ / _ \| '__/ _` |/ _ \
  ___) | |_| | |  __/ (_| | | |  _| (_) | | | (_| |  __/
 |____/ \__|_|  \___|\__,_|_| |_|  \___/|_|  \__, |\___|
                                               |___/
)" << std::endl;

    std::cout << "  Version    : v2.0" << std::endl;
    std::cout << "  Mode       : " << (local_mode ? "LOCAL (no AWS)" : "AWS CLOUD") << std::endl;
    std::cout << "  Ingestion  : http://0.0.0.0:" << cfg.port_ingest << "/ingest" << std::endl;
    std::cout << "  Query API  : http://0.0.0.0:" << cfg.port_query  << "/health | /metrics | /anomalies | /query | /version" << std::endl;
    std::cout << "  Dashboard  : http://0.0.0.0:" << cfg.port_dashboard << "/" << std::endl;
    std::cout << "  Config     : region=" << cfg.region << " batch=" << cfg.batch_size << " workers=" << cfg.workers << std::endl;
    std::cout << "─────────────────────────────────────────────────────" << std::endl;

    Aws::SDKOptions options;
    if (!local_mode) Aws::InitAPI(options);

    std::unique_ptr<S3Uploader>   s3;
    std::unique_ptr<DynamoWriter> dynamo;
    std::unique_ptr<SNSNotifier>  sns;

    if (!local_mode) {
        s3     = std::make_unique<S3Uploader>(cfg.s3_bucket, cfg.region);
        dynamo = std::make_unique<DynamoWriter>(cfg.region);
        sns    = std::make_unique<SNSNotifier>(cfg.sns_arn, cfg.region);
        std::cout << "[AWS] S3 / DynamoDB / SNS clients initialised" << std::endl;
    } else {
        std::cout << "[LOCAL] AWS disabled — pipeline + AI + API fully active" << std::endl;
    }

    std::vector<std::string> event_batch;
    std::mutex batch_mutex;

    StreamProcessor processor(cfg.workers);
    DashboardServer dashboard(processor, cfg.port_dashboard);

    processor.set_handler([&](const Event& e) {
        if (local_mode) return;
        std::string record = "{\"source\":\"" + e.source +
            "\",\"metric\":\""  + e.metric_name +
            "\",\"value\":"     + std::to_string(e.value) +
            ",\"ts\":"          + std::to_string(e.timestamp) + "}";
        std::lock_guard<std::mutex> lock(batch_mutex);
        event_batch.push_back(record);
        if ((int)event_batch.size() >= cfg.batch_size) {
            s3->upload_batch(event_batch);
            event_batch.clear();
        }
    });

    processor.set_anomaly_handler([&](const AnomalyEvent& ae) {
        const auto& e = ae.event;
        const auto& r = ae.result;
        std::string votes =
            std::string(r.zscore_vote ? "Z" : "-") +
            std::string(r.ewma_vote   ? "E" : "-") +
            std::string(r.forest_vote ? "F" : "-");

        dashboard.add_anomaly(e.metric_name, e.source, e.value, r.score, votes);

        if (!local_mode) {
            dynamo->write_anomaly(e.metric_name, e.value, r.score, votes);
            sns->notify(e.metric_name, e.value, r.score, votes, r.z_value, r.ewma_value);
        } else {
            std::cout << "[LOCAL] Anomaly: metric=" << e.metric_name
                      << " source=" << e.source
                      << " value=" << e.value
                      << " votes=[" << votes << "]"
                      << " score=" << r.score << std::endl;
        }
    });

    processor.start();
    dashboard.start_async();

    QueryServer query_server(processor, cfg.region, cfg.port_query, local_mode);
    query_server.start_async();

    HttpServer server(processor, cfg.port_ingest);
    server.start();

    std::cout << "[StreamForge] Flushing buffers..." << std::endl;
    if (!local_mode) {
        std::lock_guard<std::mutex> lock(batch_mutex);
        if (!event_batch.empty()) {
            s3->upload_batch(event_batch);
            event_batch.clear();
            std::cout << "[StreamForge] S3 buffer flushed." << std::endl;
        }
        Aws::ShutdownAPI(options);
    }

    processor.stop();
    std::cout << "[StreamForge] Shutdown complete." << std::endl;
    return 0;
}
