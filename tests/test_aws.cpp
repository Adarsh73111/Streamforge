#include <aws/core/Aws.h>
#include "../src/aws/S3Uploader.hpp"
#include "../src/aws/DynamoWriter.hpp"
#include "../src/aws/SNSNotifier.hpp"
#include <iostream>

// PASTE YOUR SNS ARN BELOW
static const std::string SNS_ARN    = "arn:aws:sns:ap-south-1:318370043798:StreamForgeAlerts";
static const std::string S3_BUCKET  = "streamforge-events-adarsh";

int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    std::cout << "=== Phase 3 AWS Test ===\n";

    // Test S3
    std::cout << "\n[1] Testing S3 upload...\n";
    S3Uploader s3(S3_BUCKET, 5, 60);
    for (int i = 0; i < 5; i++)
        s3.add({"test", "latency", 50.0 + i, 1700000000LL + i});
    s3.flush();

    // Test DynamoDB
    std::cout << "\n[2] Testing DynamoDB writes...\n";
    DynamoWriter dynamo;
    dynamo.write_metric("latency", 52.3, 4.1, 45.0, 62.0, 1700000001LL);
    dynamo.write_anomaly("latency", 520.0, 0.85, "ZF", 1700000002LL);

    // Test SNS
    std::cout << "\n[3] Testing SNS alert...\n";
    SNSNotifier sns(SNS_ARN);
    sns.notify("latency", 520.0, 0.85, "ZF", 1700000002LL);

    std::cout << "\n=== All AWS tests fired — check console + email ===\n";

    Aws::ShutdownAPI(options);
    return 0;
}
