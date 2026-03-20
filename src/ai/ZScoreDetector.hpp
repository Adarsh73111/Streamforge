#pragma once
#include <cmath>
#include <deque>
#include <string>
#include <unordered_map>

class ZScoreDetector {
public:
    explicit ZScoreDetector(std::size_t window = 50, double threshold = 3.0)
        : window_(window), threshold_(threshold) {}

    struct Result {
        bool  is_anomaly;
        double z_score;
        double mean;
        double stddev;
    };

    Result evaluate(const std::string& metric_key, double value) {
        auto& win = windows_[metric_key];
        win.push_back(value);
        if (win.size() > window_) win.pop_front();

        if (win.size() < 5) return {false, 0.0, value, 0.0};

        double mean = compute_mean(win);
        double stddev = compute_stddev(win, mean);

        if (stddev < 1e-9) return {false, 0.0, mean, 0.0};

        double z = std::abs((value - mean) / stddev);
        return {z > threshold_, z, mean, stddev};
    }

    void set_threshold(double t) { threshold_ = t; }
    void reset(const std::string& key) { windows_.erase(key); }

private:
    double compute_mean(const std::deque<double>& w) {
        double sum = 0.0;
        for (double v : w) sum += v;
        return sum / w.size();
    }

    double compute_stddev(const std::deque<double>& w, double mean) {
        double sq = 0.0;
        for (double v : w) sq += (v - mean) * (v - mean);
        return std::sqrt(sq / w.size());
    }

    std::size_t window_;
    double threshold_;
    std::unordered_map<std::string, std::deque<double>> windows_;
};
