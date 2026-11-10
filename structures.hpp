#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP
#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstddef>

template <typename T>
class BoundedQueue {
    std::deque<T> data;
    std::size_t capacity, peak{0}, blocked{0};
    bool closed{false};
    mutable std::mutex m;
    std::condition_variable notEmpty, notFull;
public:
    explicit BoundedQueue(std::size_t cap) : capacity(cap ? cap : 1) {}
    bool push(const T& value) {
        std::unique_lock<std::mutex> lock(m);
        if (data.size() >= capacity) ++blocked;
        notFull.wait(lock, [&]{ return data.size() < capacity || closed; });
        if (closed) return false;
        data.push_back(value);
        if (data.size() > peak) peak = data.size();
        notEmpty.notify_one();
        return true;
    }
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(m);
        notEmpty.wait(lock, [&]{ return !data.empty() || closed; });
        if (data.empty()) return false;
        value = std::move(data.front()); data.pop_front();
        notFull.notify_one();
        return true;
    }
    void close() { std::lock_guard<std::mutex> lock(m); closed = true; notEmpty.notify_all(); notFull.notify_all(); }
    std::size_t peakOccupancy() const { std::lock_guard<std::mutex> lock(m); return peak; }
    std::size_t blockedPushes() const { std::lock_guard<std::mutex> lock(m); return blocked; }
    std::size_t currentSize() const { std::lock_guard<std::mutex> lock(m); return data.size(); }
};
#endif
