#pragma once
#include <aws/core/Aws.h>
#include <aws/sns/SNSClient.h>
#include <aws/sns/model/PublishRequest.h>
#include <iostream>
#include <sstream>

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
        std::ostringstream msg;
        msg << "{"
            << "\"metric\":\""   << metric     << "\","
            << "\"value\":"      << value      << ","
            << "\"score\":"      << score      << ","
            << "\"votes\":\""    << votes      << "\","
            << "\"z_value\":"    << z_value    << ","
            << "\"ewma_value\":" << ewma_value
            << "}";

        Aws::SNS::Model::PublishRequest req;
        req.SetTopicArn(topic_arn_);
        req.SetMessage(msg.str());
        req.SetSubject("StreamForge ANOMALY: " + metric);

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
