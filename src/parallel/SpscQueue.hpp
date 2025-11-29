#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class SpscQueue {
public:
    void push(T value) {
        {
            std::lock_guard lk(m_);
            if (closed_) return;
            q_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // Blocking pop; returns nullopt on closed + drained
    std::optional<T> pop() {
        std::unique_lock lk(m_);
        cv_.wait(lk, [&] { return closed_ || !q_.empty(); });
        if (q_.empty()) return std::nullopt;
        T v = std::move(q_.front());
        q_.pop();
        return v;
    }

    void close() {
        {
            std::lock_guard lk(m_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::queue<T>        q_;
    std::mutex           m_;
    std::condition_variable cv_;
    bool                 closed_{ false };
};