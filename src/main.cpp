#include "ingestion/HttpServer.hpp"
#include "pipeline/StreamProcessor.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running{true};

void signal_handler(int) {
    std::cout << "\nShutting down StreamForge..." << std::endl;
    running = false;
}

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "========================================" << std::endl;
    std::cout << "   StreamForge v0.1 — Starting up       " << std::endl;
    std::cout << "========================================" << std::endl;

    StreamProcessor processor(4);

    processor.set_handler([](const Event& e) {
        std::cout << "[EVENT] source=" << e.source
                  << " metric=" << e.metric_name
                  << " value="  << e.value
                  << " ts="     << e.timestamp
                  << std::endl;
    });

    processor.start();
    std::cout << "Pipeline started with 4 worker threads" << std::endl;

    HttpServer server(processor, 8080);
    std::cout << "HTTP server listening on port 8080" << std::endl;
    std::cout << "  POST /ingest  — send events" << std::endl;
    std::cout << "  GET  /health  — check status" << std::endl;
    std::cout << "  GET  /metrics — prometheus metrics" << std::endl;

    server.start();
    processor.stop();
    return 0;
}
