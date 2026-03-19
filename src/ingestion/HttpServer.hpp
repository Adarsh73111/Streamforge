#pragma once
#include <crow.h>
#include "../pipeline/StreamProcessor.hpp"
#include <string>
#include <iostream>

class HttpServer {
public:
    explicit HttpServer(StreamProcessor& processor, uint16_t port = 8080)
        : processor_(processor), port_(port) {}

    void start() {
        CROW_ROUTE(app_, "/ingest").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
            if (req.body.empty()) {
                return crow::response(400, "Empty body");
            }
            bool ok = processor_.ingest(req.body);
            if (!ok) {
                return crow::response(429, "Buffer full — backpressure");
            }
            return crow::response(202, "Accepted");
        });

        CROW_ROUTE(app_, "/health")
        ([this]() {
            std::string body = "{\"status\":\"ok\","
                "\"processed\":" + std::to_string(processor_.events_processed()) +
                ",\"dropped\":"  + std::to_string(processor_.events_dropped()) + "}";
            auto res = crow::response(200, body);
            res.set_header("Content-Type", "application/json");
            return res;
        });

        CROW_ROUTE(app_, "/metrics")
        ([this]() {
            std::string body =
                "# HELP events_processed_total Total events processed\n"
                "# TYPE events_processed_total counter\n"
                "events_processed_total " +
                std::to_string(processor_.events_processed()) + "\n"
                "# HELP events_dropped_total Total events dropped\n"
                "# TYPE events_dropped_total counter\n"
                "events_dropped_total " +
                std::to_string(processor_.events_dropped()) + "\n";
            auto res = crow::response(200, body);
            res.set_header("Content-Type", "text/plain");
            return res;
        });

        std::cout << "StreamForge ingestion server starting on port "
                  << port_ << std::endl;
        app_.port(port_).multithreaded().run();
    }

    void stop() { app_.stop(); }

private:
    crow::SimpleApp app_;
    StreamProcessor& processor_;
    uint16_t port_;
};
