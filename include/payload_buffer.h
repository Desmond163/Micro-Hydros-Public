#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Fixed-size FIFO for payloads that could not be published (Wi-Fi or broker down).
// When full, the oldest payload is overwritten and counted as dropped, so memory use is
// bounded and the newest data is kept. Data is not persisted across a reboot.
template <std::size_t Capacity, std::size_t MaxLength>
class PayloadBuffer {
public:
    // Returns false if the oldest payload had to be dropped to make room.
    bool push(const char* payload) {
        bool keptAll = true;
        if (count_ == Capacity) {
            head_ = (head_ + 1) % Capacity;
            --count_;
            ++dropped_;
            keptAll = false;
        }
        const std::size_t tail = (head_ + count_) % Capacity;
        std::strncpy(slots_[tail].data(), payload, MaxLength - 1);
        slots_[tail][MaxLength - 1] = '\0';
        ++count_;
        return keptAll;
    }

    const char* front() const { return count_ == 0 ? nullptr : slots_[head_].data(); }

    void pop() {
        if (count_ == 0) return;
        head_ = (head_ + 1) % Capacity;
        --count_;
    }

    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    std::uint32_t dropped() const { return dropped_; }

private:
    std::array<std::array<char, MaxLength>, Capacity> slots_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    std::uint32_t dropped_ = 0;
};
