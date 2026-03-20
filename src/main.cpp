#include "ingestion/HttpServer.hpp"
#include "pipeline/StreamProcessor.hpp"
#include "aws/S3Uploader.hpp"
#include "aws/DynamoWriter.hpp"
#include "aws/SNSNotifier.hpp"
#include "api/QueryServer.hpp"
#include <aws/core/Aws.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <vector>
#include <mutex>

std::atomic<bool> running{true};
void signal_handler(int) {
    std::cout << "\nShutting down StreamForge..." << std::endl;
    running = false;
}

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    std::cout << "========================================" << std::endl;
    std::cout << "   StreamForge v0.3 — Starting up       " << std::endl;
    std::cout << "========================================" << std::endl;

    const std::string REGION    = "ap-south-1";
    const std::string S3_BUCKET = "streamforge-events-adarsh";
    const std::string SNS_ARN   =
        "arn:aws:sns:ap-south-1:318370043798:StreamForgeAlerts";

    S3Uploader   s3(S3_BUCKET, REGION);
    DynamoWriter dynamo(REGION);
    SNSNotifier  sns(SNS_ARN, REGION);

    std::vector<std::string> event_batch;
    std::mutex batch_mutex;
    const int BATCH_SIZE = 20;

    StreamProcessor processor(4);

    processor.set_handler([&](const Event& e) {
        std::string record = "{\"source\":\"" + e.source +
            "\",\"metric\":\"" + e.metric_name +
            "\",\"value\":"    + std::to_string(e.value) +
            ",\"ts\":"         + std::to_string(e.timestamp) + "}";
        std::lock_guard<std::mutex> lock(batch_mutex);
        event_batch.push_back(record);
        if ((int)event_batch.size() >= BATCH_SIZE) {
            s3.upload_batch(event_batch);
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
        dynamo.write_anomaly(e.metric_name, e.value, r.score, votes);
        sns.notify(e.metric_name, e.value, r.score, votes, r.z_value, r.ewma_value);
    });

    processor.start();

    // Start Query API on port 9090
    QueryServer query_server(processor, REGION, 9090);
    query_server.start_async();

    std::cout << "Pipeline started — AWS integration active" << std::endl;
    std::cout << "  Ingestion:  http://0.0.0.0:8080/ingest"  << std::endl;
    std::cout << "  Query API:  http://0.0.0.0:9090/query"   << std::endl;
    std::cout << "  Health:     http://0.0.0.0:9090/health"  << std::endl;
    std::cout << "  Metrics:    http://0.0.0.0:9090/metrics" << std::endl;
    std::cout << "  Anomalies:  http://0.0.0.0:9090/anomalies" << std::endl;

    HttpServer server(processor, 8080);
    server.start();

    {
        std::lock_guard<std::mutex> lock(batch_mutex);
        if (!event_batch.empty()) {
            s3.upload_batch(event_batch);
            event_batch.clear();
        }
    }

    processor.stop();
    Aws::ShutdownAPI(options);
    return 0;
}
