#pragma once
#include "../ingestion/RingBuffer.hpp"
#include "../ai/AnomalyDetector.hpp"
#include "ThreadPool.hpp"
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

struct Event {
    std::string source;
    std::string metric_name;
    double value;
    long long timestamp;
};

struct AnomalyEvent {
    Event event;
    AnomalyDetector::Result result;
};

class StreamProcessor {
public:
    using EventHandler   = std::function<void(const Event&)>;
    using AnomalyHandler = std::function<void(const AnomalyEvent&)>;

    explicit StreamProcessor(std::size_t num_threads = 4)
        : pool_(num_threads), running_(false),
          events_processed_(0), events_dropped_(0),
          anomalies_detected_(0) {}

    ~StreamProcessor() { stop(); }

    void set_handler(EventHandler h)           { handler_ = std::move(h); }
    void set_anomaly_handler(AnomalyHandler h) { anomaly_handler_ = std::move(h); }

    bool ingest(const std::string& raw_json) {
        Event e = parse(raw_json);
        if (!buffer_.push(std::move(e))) {
            events_dropped_++;
            return false;
        }
        return true;
    }

    void start() {
        running_ = true;
        dispatcher_ = std::thread([this]() {
            while (running_) {
                auto event = buffer_.pop();
                if (event.has_value()) {
                    pool_.enqueue([this, e = std::move(event.value())]() {
                        auto ai_result = detector_.evaluate(e.metric_name, e.value);
                        if (ai_result.is_anomaly) {
                            anomalies_detected_++;
                            if (anomaly_handler_) anomaly_handler_({e, ai_result});
                            std::cout << "[ANOMALY] metric=" << e.metric_name
                                      << " value=" << e.value
                                      << " " << ai_result.summary()
                                      << std::endl;
                        }
                        if (handler_) handler_(e);
                        events_processed_++;
                    });
                } else {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });
    }

    void stop() {
        running_ = false;
        if (dispatcher_.joinable()) dispatcher_.join();
        pool_.shutdown();
    }

    std::size_t events_processed()   const { return events_processed_; }
    std::size_t events_dropped()     const { return events_dropped_; }
    std::size_t anomalies_detected() const { return anomalies_detected_; }

private:
    Event parse(const std::string& json) {
        Event e;
        e.source      = extract(json, "source");
        e.metric_name = extract(json, "metric_name");
        e.timestamp   = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        try {
            auto pos = json.find("\"value\"");
            if (pos != std::string::npos) {
                auto colon = json.find(':', pos);
                e.value = std::stod(json.substr(colon + 1));
            }
        } catch (...) { e.value = 0.0; }
        return e;
    }

    std::string extract(const std::string& json, const std::string& key) {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        auto colon = json.find(':', pos);
        auto q1    = json.find('"', colon);
        auto q2    = json.find('"', q1 + 1);
        if (q1 == std::string::npos || q2 == std::string::npos) return "";
        return json.substr(q1 + 1, q2 - q1 - 1);
    }

    RingBuffer<Event, 1024> buffer_;
    ThreadPool              pool_;
    AnomalyDetector         detector_;
    EventHandler            handler_;
    AnomalyHandler          anomaly_handler_;
    std::thread             dispatcher_;
    std::atomic<bool>       running_;
    std::atomic<std::size_t> events_processed_;
    std::atomic<std::size_t> events_dropped_;
    std::atomic<std::size_t> anomalies_detected_;
};
