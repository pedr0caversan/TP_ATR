#include "utils/buffer.hpp"

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

Buffer::Item Buffer::consumer_latest() {
    Buffer::Item item;
    full_slots.acquire();
    std::scoped_lock lock(mtx);
    int n_disposable_items = 0;  // número de itens que serão descartados 
    while (full_slots.try_acquire()) {
        n_disposable_items++;
    }
    for (int i = 0; i < n_disposable_items; i++) {
        consome();
        empty_slots.release();
    }
    item = consome();
    empty_slots.release();
    return item;
}