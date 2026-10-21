//
// Created by samuel on 31/07/2026.
//

#ifndef CONCURRENT_LAB_SPSC_V1_H
#define CONCURRENT_LAB_SPSC_V1_H


#include <vector>
#include <cstddef>
#include <utility>
#include <limits>
#include <stdexcept>





#include <array>
#include <atomic>
#include <cstddef>
#include <new> // hardware_destructuive_infrerence
#include <type_traits>

//**
// a amodern c++ spsc quue type safe
// should be size configurable


// this is a naive approach to an atomic ring buffer
// like other queues accepts contract and uses differ

#include <cstdint>

#include <atomic>


template<typename T>
class spsc_v1 {




public:


    // Exactly slots usable entries, plus one spare to distinguish full from empty.
    explicit spsc_v1(const size_t req_slots)
        :
            requested_(req_slots),
            capacity_(storage_size(req_slots)),
            mask_(capacity_ - 1),
            buffer_(capacity_){}

    using value_type = T;

    // producer
    bool try_push(const T& item) {

        // SPSC only this producer will write so relaxed
        const size_t write_idx = write_idx_.load(std::memory_order_relaxed);
        const size_t next_write_idx = next_idx(write_idx);


        // lazy load head
        if (next_write_idx - cached_read_idx_ > requested_) {
            // we assume its full
            cached_read_idx_ = read_idx_.load(std::memory_order_acquire);
            if (next_write_idx - cached_read_idx_ > requested_) {
                return false;
            }
        }


        // mask only touches buffer
        buffer_[write_idx & mask_] = item;
        write_idx_.store(next_write_idx, std::memory_order_release);
        return true;
    }


    // consumer
    bool try_pop(T& item) {

        // Only consumer will read in SPSC this will never change so relaxed
        const size_t read_idx = read_idx_.load(std::memory_order_relaxed);

        if (read_idx_ == cached_write_idx_) {
            cached_write_idx_ = write_idx_.load(std::memory_order_acquire);
            if (read_idx_ == cached_write_idx_) {
                return false;
            }
        }

        item = std::move(buffer_[read_idx & mask_]);

        const size_t next_head_idx = next_idx(read_idx);
        read_idx_.store(next_head_idx, std::memory_order_release);
        return true;
    }




private:

    // The ring no longer needs power-of-two storage or a bit mask.
    std::size_t next_idx(std::size_t idx) const {
        return idx + 1;
    }

    // Check that adding the spare slot is representable before allocating.
    static std::size_t storage_size(std::size_t req_slots) {
        if (req_slots == 0) {
            throw std::invalid_argument("SPSC capacity must be positive");
        }
        if (req_slots >= std::numeric_limits<std::size_t>::max()) {
            throw std::length_error("SPSC capacity is too large");
        }

        const std::size_t target = req_slots + 1;
        
        // Cant exceed max power of 2
        constexpr std::size_t max_power_of_two = std::numeric_limits<std::size_t>::max() / 2 + 1;
        if (target > max_power_of_two) {
            throw std::length_error("SPSC capacity exceeds maximum allowable power-of-two size");
        }
        return std::bit_ceil(target);
    }

    

#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t cacheLineSize =
        std::hardware_destructive_interference_size;
#else
    static constexpr size_t cacheLineSize =
        64; // fallback to amd64 sizes
#endif

    const std::size_t requested_;
    const std::size_t capacity_; // Storage slots, including the spare.
    const std::size_t mask_;
    alignas(cacheLineSize) std::atomic<size_t> read_idx_{0}; // next slot to read
    alignas(cacheLineSize) std::atomic<size_t> write_idx_{0};  // next slot to write
    alignas(cacheLineSize) std::size_t cached_read_idx_ {0};
    alignas(cacheLineSize) std::size_t cached_write_idx_ {0};

    std::vector<T> buffer_;
};


//  5
// producer
// write
// read
// if read == write return



#endif // CONCURRENT_LAB_SPSC_V1_H
