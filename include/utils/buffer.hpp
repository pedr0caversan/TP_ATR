#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <chrono>
#include <mutex>
#include <semaphore>
#include <string>
#include <variant>

struct PosData {
        int pos;
        std::chrono::steady_clock::time_point timestamp; // útil para debug
};

struct VelData {
        float vel;
        std::chrono::steady_clock::time_point timestamp; // útil para debug
};

struct CoordData {
        std::chrono::steady_clock::time_point timestamp;
        int coord[2];
};

class Buffer {
    public:
        using Item = std::variant<PosData, VelData, CoordData>;

        Buffer();
        virtual ~Buffer() = default;

        void producer(const Item& item);
        Item consumer();
        // consome o último item colocado no buffer e descarta o restante
        Item consumer_latest();

    protected:
        static constexpr int BUFFER_SIZE = 100;
        Item buffer[BUFFER_SIZE];
        int in = 0, out = 0;

        std::counting_semaphore<BUFFER_SIZE> empty_slots{BUFFER_SIZE};
        std::counting_semaphore<BUFFER_SIZE> full_slots{0};
        std::mutex mtx;

        void produz(const Item& item);
        Item consome();
};

#endif  // BUFFER_HPP