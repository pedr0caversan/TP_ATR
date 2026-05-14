#include "buffer.hpp"

Buffer::Buffer() = default;

void Buffer::produz(const Item& item) {
    buffer[in] = item;
    in = (in + 1) % BUFFER_SIZE;
}

Buffer::Item Buffer::consome() {
    Buffer::Item item = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    return item;
}

void Buffer::producer(const Item& item) {
    empty_slots.acquire();
    std::scoped_lock lock(mtx);
    produz(item);
    full_slots.release();
}

Buffer::Item Buffer::consumer() {
    Buffer::Item item;
    full_slots.acquire();
    std::scoped_lock lock(mtx);
    item = consome();
    empty_slots.release();
    return item;
}