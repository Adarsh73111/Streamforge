#include "../src/ingestion/RingBuffer.hpp"
#include <iostream>
#include <thread>
#include <cassert>

void test_basic() {
    RingBuffer<int, 8> rb;
    assert(rb.empty());
    assert(!rb.full());
    for (int i = 0; i < 7; i++) assert(rb.push(i));
    assert(rb.full());
    assert(!rb.push(99));
    for (int i = 0; i < 7; i++) {
        auto val = rb.pop();
        assert(val.has_value());
        assert(val.value() == i);
    }
    assert(rb.empty());
    std::cout << "PASS: basic push/pop test" << std::endl;
}

void test_concurrent() {
    RingBuffer<int, 1024> rb;
    const int N = 500;
    std::thread producer([&]() {
        for (int i = 0; i < N; i++) while (!rb.push(i));
    });
    std::thread consumer([&]() {
        int received = 0;
        while (received < N) {
            auto val = rb.pop();
            if (val.has_value()) received++;
        }
        std::cout << "PASS: concurrent test — " << received << " items transferred" << std::endl;
    });
    producer.join();
    consumer.join();
}

int main() {
    test_basic();
    test_concurrent();
    std::cout << "All RingBuffer tests passed." << std::endl;
    return 0;
}
