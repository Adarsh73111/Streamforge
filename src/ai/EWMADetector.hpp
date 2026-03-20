#pragma once
#include <cmath>
#include <string>
#include <unordered_map>

class EWMADetector {
public:
    explicit EWMADetector(double alpha = 0.1, double threshold = 3.0)
        : alpha_(alpha), threshold_(threshold) {}

    struct Result {
        bool   is_anomaly;
        double ewma;
        double variance;
        double deviation;
    };

    Result evaluate(const std::string& metric_key, double value) {
        auto it = state_.find(metric_key);

        if (it == state_.end()) {
            state_[metric_key] = {value, 0.0, 0};
            return {false, value, 0.0, 0.0};
        }

        auto& s = it->second;
        s.count++;

        // Update EWMA mean
        double prev_ewma = s.ewma;
        s.ewma = alpha_ * value + (1.0 - alpha_) * s.ewma;

        // Update EWMA variance (Welford-style exponential)
        double diff = value - prev_ewma;
        s.variance = (1.0 - alpha_) * (s.variance + alpha_ * diff * diff);

        if (s.count < 10) return {false, s.ewma, s.variance, 0.0};

        double stddev = std::sqrt(s.variance);
        if (stddev < 1e-9) return {false, s.ewma, s.variance, 0.0};

        double deviation = std::abs(value - s.ewma) / stddev;
        bool anomaly = deviation > threshold_;

        return {anomaly, s.ewma, s.variance, deviation};
    }

    void set_alpha(double a)     { alpha_ = a; }
    void set_threshold(double t) { threshold_ = t; }
    void reset(const std::string& key) { state_.erase(key); }

private:
    struct State {
        double ewma;
        double variance;
        int    count;
    };

    double alpha_;
    double threshold_;
    std::unordered_map<std::string, State> state_;
};
