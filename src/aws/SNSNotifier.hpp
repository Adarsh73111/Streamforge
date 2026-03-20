#pragma once
#include <aws/core/Aws.h>
#include <aws/sns/SNSClient.h>
#include <aws/sns/model/PublishRequest.h>
#include <iostream>
#include <sstream>
#include <ctime>

class SNSNotifier {
public:
    SNSNotifier(const std::string& topic_arn, const std::string& region)
        : topic_arn_(topic_arn) {
        Aws::Client::ClientConfiguration cfg;
        cfg.region = region;
        client_ = std::make_unique<Aws::SNS::SNSClient>(cfg);
    }

    void notify(const std::string& metric, double value, double score,
                const std::string& votes, double z_value, double ewma_value) {

        // Human-readable message
        std::ostringstream msg;
        msg << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        msg << "  ⚠️  STREAMFORGE ANOMALY ALERT\n";
        msg << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        msg << "  Metric   : " << metric << "\n";
        msg << "  Value    : " << value  << " (ANOMALOUS)\n";
        msg << "  Score    : " << score  << " (higher = more anomalous)\n\n";
        msg << "  Detectors that flagged this:\n";
        if (votes.find('Z') != std::string::npos)
            msg << "    ✓ Z-score    — sudden spike detected (z=" << z_value << ")\n";
        if (votes.find('E') != std::string::npos)
            msg << "    ✓ EWMA       — drift detected (dev=" << ewma_value << ")\n";
        if (votes.find('F') != std::string::npos)
            msg << "    ✓ Isolation Forest — statistical outlier detected\n";
        msg << "\n  Voting   : " << votes << " (2 of 3 detectors agreed)\n\n";
        msg << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        msg << "  StreamForge — Real-Time Analytics\n";
        msg << "  github.com/Adarsh73111/Streamforge\n";
        msg << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        Aws::SNS::Model::PublishRequest req;
        req.SetTopicArn(topic_arn_);
        req.SetMessage(msg.str());
        req.SetSubject("⚠️ StreamForge ANOMALY: " + metric + " = " + std::to_string(value));

        auto out = client_->Publish(req);
        if (out.IsSuccess())
            std::cout << "[SNS] Alert sent: " << metric << " value=" << value << "\n";
        else
            std::cerr << "[SNS] Alert FAILED: " << out.GetError().GetMessage() << "\n";
    }

private:
    std::string topic_arn_;
    std::unique_ptr<Aws::SNS::SNSClient> client_;
};
