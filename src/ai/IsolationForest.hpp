#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <deque>

class IsolationForest {
public:
    explicit IsolationForest(int num_trees = 50,
                              int sample_size = 32,
                              std::size_t window = 256)
        : num_trees_(num_trees),
          sample_size_(sample_size),
          window_(window),
          rng_(std::random_device{}()) {}

    struct Result {
        bool   is_anomaly;
        double score;
    };

    Result evaluate(const std::string& metric_key, double value) {
        auto& buf = buffers_[metric_key];
        buf.push_back(value);
        if (buf.size() > window_) buf.pop_front();

        if ((int)buf.size() < sample_size_) return {false, 0.0};

        std::vector<double> data(buf.begin(), buf.end());
        double score = anomaly_score(data, value);
        return {score > 0.6, score};
    }

    void set_threshold(double t) { threshold_ = t; }

private:
    struct Node {
        bool   is_leaf = false;
        double split_value = 0.0;
        int    size = 0;
        std::unique_ptr<Node> left, right;
    };

    std::unique_ptr<Node> build_tree(std::vector<double>& data,
                                      int depth, int max_depth) {
        auto node = std::make_unique<Node>();
        node->size = (int)data.size();

        if (depth >= max_depth || data.size() <= 1) {
            node->is_leaf = true;
            return node;
        }

        double mn = *std::min_element(data.begin(), data.end());
        double mx = *std::max_element(data.begin(), data.end());

        if (std::abs(mx - mn) < 1e-9) {
            node->is_leaf = true;
            return node;
        }

        std::uniform_real_distribution<double> dist(mn, mx);
        node->split_value = dist(rng_);

        std::vector<double> left_data, right_data;
        for (double v : data) {
            if (v < node->split_value) left_data.push_back(v);
            else                       right_data.push_back(v);
        }

        if (left_data.empty() || right_data.empty()) {
            node->is_leaf = true;
            return node;
        }

        node->left  = build_tree(left_data,  depth + 1, max_depth);
        node->right = build_tree(right_data, depth + 1, max_depth);
        return node;
    }

    double path_length(const Node* node, double value, int depth) {
        if (node->is_leaf || !node->left || !node->right) {
            return depth + c_factor(node->size);
        }
        if (value < node->split_value)
            return path_length(node->left.get(),  value, depth + 1);
        else
            return path_length(node->right.get(), value, depth + 1);
    }

    double c_factor(int n) {
        if (n <= 1) return 0.0;
        return 2.0 * (std::log(n - 1) + 0.5772156649) - 2.0 * (n - 1) / n;
    }

    double anomaly_score(std::vector<double>& data, double value) {
        int actual_sample = std::min(sample_size_, (int)data.size());
        int max_depth = (int)std::ceil(std::log2(actual_sample));

        double avg_path = 0.0;
        for (int i = 0; i < num_trees_; i++) {
            std::shuffle(data.begin(), data.end(), rng_);
            std::vector<double> sample(data.begin(),
                data.begin() + actual_sample);
            auto tree = build_tree(sample, 0, max_depth);
            avg_path += path_length(tree.get(), value, 0);
        }
        avg_path /= num_trees_;

        double cf = c_factor(actual_sample);
        if (cf <= 0.0) return 0.5;
        return std::pow(2.0, -avg_path / cf);
    }

    int         num_trees_;
    int         sample_size_;
    std::size_t window_;
    double      threshold_ = 0.6;
    std::mt19937 rng_;
    std::unordered_map<std::string, std::deque<double>> buffers_;
};
