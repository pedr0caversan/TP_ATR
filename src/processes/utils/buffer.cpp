#include <mutex>
#include <counting_semaphore>


class Buffer() {

}

const int BUFFER_SIZE = 10;

int buffer[BUFFER_SIZE];
int in = 0, out = 0;

void produz(int dado) {
    buffer[in] = dado;
    in = (in+1) % BUFFER_SIZE;
}

int consome() {
    int item = buffer[out];
    out = (out+1) % BUFFER_SIZE;
    return item;
}

std::counting_semaphore<BUFFER_SIZE> empty_slots(BUFFER_SIZE);
std::counting_semaphore<BUFFER_SIZE> full_slots(0);
std::mutex mtx;

void producer(int dado) {
    empty_slots.acquire();
    mtx.lock();
    produz(dado);
    mtx.unlock();
    full_slots.release();
}

void consumer() {
    int item;
    full_slots.acquire();
    mtx.lock();
    item = consome();
    mtx.unlock();
    empty_slots.release();
}