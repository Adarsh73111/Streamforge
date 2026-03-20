#include "../src/ai/AnomalyDetector.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

void test_normal_no_anomalies() {
    AnomalyDetector detector;
    int false_positives = 0;
    for (int i = 0; i < 500; i++) {
        double value = 50.0 + (i % 10) * 0.5;
        auto result = detector.evaluate("cpu", value);
        if (result.is_anomaly) false_positives++;
    }
    std::cout << "Normal data false positives: " << false_positives
              << "/500" << std::endl;
    assert(false_positives < 10);
    std::cout << "PASS: normal data produces minimal false positives" << std::endl;
}

void test_spike_detected() {
    AnomalyDetector detector;
    // Warm up with normal data
    for (int i = 0; i < 200; i++)
        detector.evaluate("latency", 100.0 + (i % 5));

    // Inject spikes
    int detected = 0;
    double spikes[] = {500.0, 600.0, 450.0, 700.0, 550.0};
    for (double spike : spikes) {
        auto result = detector.evaluate("latency", spike);
        if (result.is_anomaly) detected++;
        std::cout << "  Spike " << spike << ": " << result.summary() << std::endl;
    }
    std::cout << "Spikes detected: " << detected << "/5" << std::endl;
    assert(detected >= 3);
    std::cout << "PASS: spike detection working" << std::endl;
}

void test_drift_detected() {
    AnomalyDetector detector;
    // Normal baseline
    for (int i = 0; i < 100; i++)
        detector.evaluate("memory", 30.0 + (i % 3));

    // Gradual drift upward
    int drift_detected = 0;
    for (int i = 0; i < 50; i++) {
        double drifted = 30.0 + i * 1.5;
        auto result = detector.evaluate("memory", drifted);
        if (result.is_anomaly) drift_detected++;
    }
    std::cout << "Drift anomalies detected: " << drift_detected
              << "/50" << std::endl;
    assert(drift_detected >= 5);
    std::cout << "PASS: drift detection working" << std::endl;
}

void test_multiple_metrics() {
    AnomalyDetector detector;
    // Two independent metric streams
    for (int i = 0; i < 100; i++) {
        detector.evaluate("service_a", 10.0 + (i % 3));
        detector.evaluate("service_b", 200.0 + (i % 5));
    }
    // Spike only service_a
    auto result_a = detector.evaluate("service_a", 999.0);
    auto result_b = detector.evaluate("service_b", 202.0);

    std::cout << "service_a spike: " << result_a.summary() << std::endl;
    std::cout << "service_b normal: " << result_b.summary() << std::endl;
    assert(result_b.is_anomaly == false);
    std::cout << "PASS: multiple independent metric streams work correctly" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   StreamForge AI Anomaly Detector Test " << std::endl;
    std::cout << "========================================" << std::endl;
    test_normal_no_anomalies();
    std::cout << std::endl;
    test_spike_detected();
    std::cout << std::endl;
    test_drift_detected();
    std::cout << std::endl;
    test_multiple_metrics();
    std::cout << std::endl;
    std::cout << "All anomaly detection tests passed." << std::endl;
    return 0;
}
