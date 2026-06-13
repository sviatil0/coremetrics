#ifndef __RING_BUFFER_HPP__
#define __RING_BUFFER_HPP__

#include <cstddef>
#include <vector>

// Fixed-capacity circular buffer of T. Newest sample at the back, oldest at
// the front. When the buffer is full, push() drops the oldest entry. Mirrors
// the style of include/Tree.hpp: header-only, owns its storage, no exception
// dependencies beyond what std::vector already throws.
//
// Intended for fixed-size time-series windows: a 64-sample rolling history of
// each metric, fed once per poll tick, drawn once per frame.
template<typename T>
class RingBuffer
{
private:
    std::vector<T> data;
    std::size_t capacity;
    std::size_t head;
    std::size_t count;

public:
    explicit RingBuffer(std::size_t cap)
        : data(cap == 0 ? 1 : cap),
          capacity(cap == 0 ? 1 : cap),
          head(0),
          count(0)
    {
    }

    std::size_t getCapacity() const
    {
        return capacity;
    }

    std::size_t getSize() const
    {
        return count;
    }

    bool isEmpty() const
    {
        return count == 0;
    }

    bool isFull() const
    {
        return count == capacity;
    }

    void clear()
    {
        head = 0;
        count = 0;
    }

    void push(const T &value)
    {
        data[head] = value;
        head = (head + 1) % capacity;
        if (count < capacity)
        {
            ++count;
        }
    }

    // Index 0 is the OLDEST sample, index (getSize() - 1) is the NEWEST.
    // Returns a default-constructed T when index is out of range. The
    // caller is expected to check getSize() first; the bounds check is a
    // safety net, not the contract.
    T at(std::size_t index) const
    {
        if (index >= count)
        {
            return T();
        }
        std::size_t start = (count < capacity) ? 0 : head;
        std::size_t physical = (start + index) % capacity;
        return data[physical];
    }

    T newest() const
    {
        if (count == 0)
        {
            return T();
        }
        std::size_t physical = (head == 0) ? capacity - 1 : head - 1;
        return data[physical];
    }

    T oldest() const
    {
        if (count == 0)
        {
            return T();
        }
        std::size_t start = (count < capacity) ? 0 : head;
        return data[start];
    }
};

#endif
