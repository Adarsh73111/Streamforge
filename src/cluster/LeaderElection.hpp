#pragma once
#include <string>
#include <atomic>
#include <sstream>
#include <iostream>

class LeaderElection {
public:
    explicit LeaderElection(const std::string& node_id)
        : node_id_(node_id), is_leader_(false) {}

    void elect() {
        is_leader_ = true;
        std::cout << "[Leader] Node " << node_id_
                  << " elected as leader — owns SNS alerts" << std::endl;
    }

    bool is_leader()      const { return is_leader_.load(); }
    std::string node_id() const { return node_id_; }

    std::string get_status_json() const {
        std::ostringstream ss;
        ss << "{"
           << "\"node_id\":\"" << node_id_ << "\","
           << "\"is_leader\":"  << (is_leader_.load() ? "true" : "false")
           << "}";
        return ss.str();
    }

private:
    std::string       node_id_;
    std::atomic<bool> is_leader_;
};
