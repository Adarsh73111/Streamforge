#include "../src/pipeline/StreamProcessor.hpp"
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <cassert>

int main() {
    StreamProcessor processor(4);
    std::atomic<int> received{0};

    processor.set_handler([&](const Event& e) {
        received++;
    });

    processor.start();

    const int N = 200;
    for (int i = 0; i < N; i++) {
        std::string json = "{\"source\":\"test\",\"metric_name\":\"latency\",\"value\":"
                         + std::to_string(i * 1.5) + "}";
        processor.ingest(json);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    processor.stop();

    std::cout << "Events sent:      " << N << std::endl;
    std::cout << "Events processed: " << processor.events_processed() << std::endl;
    std::cout << "Events dropped:   " << processor.events_dropped() << std::endl;

    assert(processor.events_processed() == N);
    assert(processor.events_dropped() == 0);
    std::cout << "PASS: StreamProcessor pipeline test" << std::endl;
    return 0;
}
