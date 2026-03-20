#pragma once
#include <aws/core/Aws.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <iostream>
#include <string>
#include <chrono>

class DynamoWriter {
public:
    DynamoWriter(const std::string& region) {
        Aws::Client::ClientConfiguration cfg;
        cfg.region = region;
        client_ = std::make_unique<Aws::DynamoDB::DynamoDBClient>(cfg);
    }

    void write_anomaly(const std::string& metric, double value,
                       double score, const std::string& votes) {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string id = metric + "_" + std::to_string(ts);

        Aws::DynamoDB::Model::PutItemRequest req;
        req.SetTableName("AnomalyLog");
        req.AddItem("anomaly_id",  Aws::DynamoDB::Model::AttributeValue(id));
        req.AddItem("metric_name", Aws::DynamoDB::Model::AttributeValue(metric));
        req.AddItem("value",       Aws::DynamoDB::Model::AttributeValue(std::to_string(value)));
        req.AddItem("score",       Aws::DynamoDB::Model::AttributeValue(std::to_string(score)));
        req.AddItem("votes",       Aws::DynamoDB::Model::AttributeValue(votes));
        req.AddItem("timestamp",   Aws::DynamoDB::Model::AttributeValue(std::to_string(ts)));

        auto out = client_->PutItem(req);
        if (out.IsSuccess())
            std::cout << "[Dynamo] Anomaly logged: " << id << "\n";
        else
            std::cerr << "[Dynamo] FAILED: " << out.GetError().GetMessage() << "\n";
    }

    void write_metric(const std::string& metric, double mean, double stddev,
                      double mn, double mx, long long ts) {
        Aws::DynamoDB::Model::PutItemRequest req;
        req.SetTableName("StreamMetrics");
        req.AddItem("metric_name", Aws::DynamoDB::Model::AttributeValue(metric));
        req.AddItem("timestamp",   Aws::DynamoDB::Model::AttributeValue(std::to_string(ts)));
        req.AddItem("mean",        Aws::DynamoDB::Model::AttributeValue(std::to_string(mean)));
        req.AddItem("stddev",      Aws::DynamoDB::Model::AttributeValue(std::to_string(stddev)));
        req.AddItem("min_val",     Aws::DynamoDB::Model::AttributeValue(std::to_string(mn)));
        req.AddItem("max_val",     Aws::DynamoDB::Model::AttributeValue(std::to_string(mx)));

        auto out = client_->PutItem(req);
        if (out.IsSuccess())
            std::cout << "[Dynamo] Metric written: " << metric << "\n";
        else
            std::cerr << "[Dynamo] Metric FAILED: " << out.GetError().GetMessage() << "\n";
    }

private:
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client_;
};
