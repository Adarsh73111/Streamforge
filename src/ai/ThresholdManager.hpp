#pragma once
#include <string>
#include <map>
#include <mutex>
#include <chrono>

struct MetricConfig {
    double threshold    = 3.0;
    int    cooldown_secs = 0;
    long long last_alert = 0;
};

class ThresholdManager {
public:
    void set(const std::string& metric, double threshold, int cooldown_secs) {
        std::lock_guard<std::mutex> lock(mutex_);
        configs_[metric].threshold     = threshold;
        configs_[metric].cooldown_secs = cooldown_secs;
    }

    void reset(const std::string& metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        configs_.erase(metric);
    }

    double get_threshold(const std::string& metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = configs_.find(metric);
        return it != configs_.end() ? it->second.threshold : 3.0;
    }

    bool cooldown_ok(const std::string& metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& cfg = configs_[metric];
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (cfg.cooldown_secs == 0 || now - cfg.last_alert >= cfg.cooldown_secs) {
            cfg.last_alert = now;
            return true;
        }
        return false;
    }

    std::map<std::string, MetricConfig> get_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        return configs_;
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, MetricConfig> configs_;
};
