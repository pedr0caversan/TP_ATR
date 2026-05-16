#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <chrono>
#include <counting_semaphore>
#include <mutex>
#include <string>
#include <variant>

struct pos_buffer {
        int pos;
};

struct vel_buffer {
        int vel;
};

struct coord_buffer {
        std::chrono::system_clock timestamp;
        int coord[2];
};

class Buffer {
    public:
        using Item = std::variant<pos_buffer, vel_buffer, coord_buffer>;

        Buffer();
        virtual ~Buffer() = default;

        void producer(const Item& item);
        Item consumer();

    protected:
        static constexpr int BUFFER_SIZE;
        Item buffer[BUFFER_SIZE];
        int in = 0, out = 0;

        std::counting_semaphore<BUFFER_SIZE> empty_slots(BUFFER_SIZE);
        std::counting_semaphore<BUFFER_SIZE> full_slots(0);
        std::mutex mtx;

        void produz(const Item& item);
        Item consome();
}

#endif  // BUFFER_HPP