#pragma once
#include <crow.h>
#include <aws/core/Aws.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/dynamodb/model/QueryRequest.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include "../pipeline/StreamProcessor.hpp"
#include <string>
#include <iostream>
#include <chrono>
#include <sstream>

class QueryServer {
public:
    QueryServer(StreamProcessor& processor, const std::string& region, int port = 9090)
        : processor_(processor), port_(port) {
        Aws::Client::ClientConfiguration cfg;
        cfg.region = region;
        dynamo_ = std::make_unique<Aws::DynamoDB::DynamoDBClient>(cfg);
    }

    void start_async() {
        server_thread_ = std::thread([this]() { run(); });
        server_thread_.detach();
    }

private:
    void run() {
        crow::SimpleApp app;
        app.loglevel(crow::LogLevel::Warning);

        // GET /health
        CROW_ROUTE(app, "/health")
        ([this]() {
            std::ostringstream ss;
            ss << "{"
               << "\"status\":\"ok\","
               << "\"events_processed\":" << processor_.events_processed() << ","
               << "\"events_dropped\":"   << processor_.events_dropped()   << ","
               << "\"anomalies_detected\":" << processor_.anomalies_detected()
               << "}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            return res;
        });

        // GET /metrics  (Prometheus format)
        CROW_ROUTE(app, "/metrics")
        ([this]() {
            std::ostringstream ss;
            ss << "# HELP events_processed_total Total events processed\n"
               << "# TYPE events_processed_total counter\n"
               << "events_processed_total " << processor_.events_processed() << "\n"
               << "# HELP events_dropped_total Total events dropped\n"
               << "# TYPE events_dropped_total counter\n"
               << "events_dropped_total " << processor_.events_dropped() << "\n"
               << "# HELP anomalies_detected_total Total anomalies detected\n"
               << "# TYPE anomalies_detected_total counter\n"
               << "anomalies_detected_total " << processor_.anomalies_detected() << "\n";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "text/plain");
            return res;
        });

        // GET /anomalies  — last 20 from DynamoDB AnomalyLog
        CROW_ROUTE(app, "/anomalies")
        ([this]() {
            Aws::DynamoDB::Model::ScanRequest req;
            req.SetTableName("AnomalyLog");
            req.SetLimit(20);
            auto out = dynamo_->Scan(req);

            std::ostringstream ss;
            ss << "{\"anomalies\":[";
            if (out.IsSuccess()) {
                auto& items = out.GetResult().GetItems();
                for (size_t i = 0; i < items.size(); i++) {
                    auto& item = items[i];
                    ss << "{";
                    ss << "\"anomaly_id\":\""  << get_s(item, "anomaly_id")  << "\",";
                    ss << "\"metric_name\":\"" << get_s(item, "metric_name") << "\",";
                    ss << "\"value\":"         << get_s(item, "value")       << ",";
                    ss << "\"score\":"         << get_s(item, "score")       << ",";
                    ss << "\"votes\":\""       << get_s(item, "votes")       << "\",";
                    ss << "\"timestamp\":"     << get_s(item, "timestamp");
                    ss << "}";
                    if (i + 1 < items.size()) ss << ",";
                }
            }
            ss << "]}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            return res;
        });

        // GET /query?metric=latency  — rolling stats from StreamMetrics
        CROW_ROUTE(app, "/query")
        ([this](const crow::request& req) {
            std::string metric = req.url_params.get("metric") ?
                                 req.url_params.get("metric") : "latency";

            Aws::DynamoDB::Model::QueryRequest qreq;
            qreq.SetTableName("StreamMetrics");
            qreq.SetKeyConditionExpression("metric_name = :m");
            qreq.AddExpressionAttributeValues(
                ":m", Aws::DynamoDB::Model::AttributeValue(metric));
            qreq.SetScanIndexForward(false);
            qreq.SetLimit(10);

            auto out = dynamo_->Query(qreq);

            std::ostringstream ss;
            ss << "{\"metric\":\"" << metric << "\",\"results\":[";
            if (out.IsSuccess()) {
                auto& items = out.GetResult().GetItems();
                for (size_t i = 0; i < items.size(); i++) {
                    auto& item = items[i];
                    ss << "{";
                    ss << "\"timestamp\":"  << get_s(item, "timestamp") << ",";
                    ss << "\"mean\":"       << get_s(item, "mean")      << ",";
                    ss << "\"stddev\":"     << get_s(item, "stddev")    << ",";
                    ss << "\"min\":"        << get_s(item, "min_val")   << ",";
                    ss << "\"max\":"        << get_s(item, "max_val");
                    ss << "}";
                    if (i + 1 < items.size()) ss << ",";
                }
            }
            ss << "]}";
            auto res = crow::response(200, ss.str());
            res.set_header("Content-Type", "application/json");
            return res;
        });

        std::cout << "StreamForge query API starting on port " << port_ << "\n";
        app.port(port_).run();
    }

    std::string get_s(
        const Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>& item,
        const std::string& key) {
        auto it = item.find(key);
        if (it == item.end()) return "null";
        if (!it->second.GetS().empty()) return it->second.GetS();
        if (!it->second.GetN().empty()) return it->second.GetN();
        return "null";
    }

    StreamProcessor& processor_;
    int port_;
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> dynamo_;
    std::thread server_thread_;
};
