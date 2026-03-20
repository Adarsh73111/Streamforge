#pragma once
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>

class S3Uploader {
public:
    S3Uploader(const std::string& bucket, const std::string& region)
        : bucket_(bucket) {
        Aws::Client::ClientConfiguration cfg;
        cfg.region = region;
        client_ = std::make_unique<Aws::S3::S3Client>(cfg);
    }

    void upload_batch(const std::vector<std::string>& records) {
        if (records.empty()) return;
        std::ostringstream ss;
        for (auto& r : records) ss << r << "\n";

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::string key = "events/" + std::to_string(now_ms) + ".json";

        auto body = Aws::MakeShared<Aws::StringStream>("S3Up", ss.str());
        Aws::S3::Model::PutObjectRequest req;
        req.SetBucket(bucket_);
        req.SetKey(key);
        req.SetBody(body);

        auto out = client_->PutObject(req);
        if (out.IsSuccess())
            std::cout << "[S3] Uploaded " << records.size() << " events → " << key << "\n";
        else
            std::cerr << "[S3] FAILED: " << out.GetError().GetMessage() << "\n";
    }

private:
    std::string bucket_;
    std::unique_ptr<Aws::S3::S3Client> client_;
};
