#pragma once
#include <atomic>
#include <cstddef>
#include <optional>

template<typename T, std::size_t Capacity>
class RingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0,
        "Capacity must be a power of 2");
public:
    RingBuffer() : head_(0), tail_(0) {}

    bool push(T item) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[head] = std::move(item);
        head_.store(next, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T item = std::move(buffer_[tail]);
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    bool full() const {
        const std::size_t next =
            (head_.load(std::memory_order_acquire) + 1) & mask_;
        return next == tail_.load(std::memory_order_acquire);
    }

    std::size_t size_approx() const {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & mask_;
    }

    static constexpr std::size_t capacity() { return Capacity; }

private:
    static constexpr std::size_t mask_ = Capacity - 1;
    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
    alignas(64) T buffer_[Capacity];
};
