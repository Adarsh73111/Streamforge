#pragma once
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>

struct Config {
    std::string region           = "ap-south-1";
    std::string s3_bucket        = "streamforge-events-adarsh";
    std::string sns_arn          = "";
    int         batch_size       = 20;
    int         flush_secs       = 30;
    int         workers          = 4;
    int         port_ingest      = 8080;
    int         port_query       = 9090;
    double      zscore_threshold = 3.0;

    static Config load(const std::string& path = "config.json") {
        Config cfg;
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cout << "[Config] config.json not found, using defaults" << std::endl;
            return cfg;
        }
        try {
            nlohmann::json j;
            f >> j;
            if (j.contains("region"))           cfg.region           = j["region"];
            if (j.contains("s3_bucket"))         cfg.s3_bucket        = j["s3_bucket"];
            if (j.contains("sns_arn"))           cfg.sns_arn          = j["sns_arn"];
            if (j.contains("batch_size"))        cfg.batch_size       = j["batch_size"];
            if (j.contains("flush_secs"))        cfg.flush_secs       = j["flush_secs"];
            if (j.contains("workers"))           cfg.workers          = j["workers"];
            if (j.contains("port_ingest"))       cfg.port_ingest      = j["port_ingest"];
            if (j.contains("port_query"))        cfg.port_query       = j["port_query"];
            if (j.contains("zscore_threshold"))  cfg.zscore_threshold = j["zscore_threshold"];
            std::cout << "[Config] Loaded config.json successfully" << std::endl;
        } catch (const std::exception& ex) {
            std::cout << "[Config] Parse error: " << ex.what() << ", using defaults" << std::endl;
        }
        return cfg;
    }
};
