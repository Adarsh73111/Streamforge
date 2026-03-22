#pragma once
#include <string>
#include <chrono>
#include <sstream>
#include <iostream>

class NodeManager {
public:
    NodeManager(const std::string& node_id, const std::string& region)
        : node_id_(node_id), region_(region) {
        start_time_ = std::chrono::system_clock::now();
    }

    std::string get_node_id() const { return node_id_; }
    std::string get_region()  const { return region_; }

    long long uptime_seconds() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(
            now - start_time_).count();
    }

    std::string get_status_json() const {
        std::ostringstream ss;
        ss << "{"
           << "\"node_id\":\"" << node_id_ << "\","
           << "\"region\":\""  << region_  << "\","
           << "\"uptime_secs\":" << uptime_seconds() << ","
           << "\"status\":\"active\""
           << "}";
        return ss.str();
    }

private:
    std::string node_id_;
    std::string region_;
    std::chrono::system_clock::time_point start_time_;
};
