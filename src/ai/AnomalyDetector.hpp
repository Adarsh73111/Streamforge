#pragma once
#include "ZScoreDetector.hpp"
#include "EWMADetector.hpp"
#include "IsolationForest.hpp"
#include "ThresholdManager.hpp"
#include <string>
#include <sstream>

class AnomalyDetector {
public:
    struct Result {
        bool   is_anomaly;
        double score;
        bool   zscore_vote;
        bool   ewma_vote;
        bool   forest_vote;
        double z_value;
        double ewma_value;
        double forest_score;
        std::string summary() const {
            std::ostringstream ss;
            ss << "anomaly=" << (is_anomaly ? "YES" : "NO")
               << " score=" << score
               << " votes=["
               << (zscore_vote ? "Z" : "-")
               << (ewma_vote   ? "E" : "-")
               << (forest_vote ? "F" : "-")
               << "]"
               << " z=" << z_value
               << " ewma_dev=" << ewma_value
               << " forest=" << forest_score;
            return ss.str();
        }
    };

    AnomalyDetector()
        : zscore_(50, 3.0),
          ewma_(0.1, 3.0),
          forest_(50, 32, 256) {}

    Result evaluate(const std::string& metric_key, double value) {
        // ── Apply per-metric threshold ────────────────────────────────
        std::string base_metric = metric_key.substr(0, metric_key.find(':'));
        double threshold = thresholds_.get_threshold(base_metric);
        zscore_.set_threshold(threshold);

        auto z = zscore_.evaluate(metric_key, value);
        auto e = ewma_.evaluate(metric_key, value);
        auto f = forest_.evaluate(metric_key, value);

        int votes = (int)z.is_anomaly +
                    (int)e.is_anomaly +
                    (int)f.is_anomaly;

        double score = (z.z_score / 10.0 +
                        e.deviation / 10.0 +
                        f.score) / 3.0;

        bool is_anomaly = votes >= 2;

        // ── Apply cooldown per metric ─────────────────────────────────
        if (is_anomaly && !thresholds_.cooldown_ok(base_metric)) {
            is_anomaly = false;
        }

        return {
            is_anomaly,
            score,
            z.is_anomaly,
            e.is_anomaly,
            f.is_anomaly,
            z.z_score,
            e.deviation,
            f.score
        };
    }

    ThresholdManager& thresholds() { return thresholds_; }

private:
    ZScoreDetector   zscore_;
    EWMADetector     ewma_;
    IsolationForest  forest_;
    ThresholdManager thresholds_;
};
