#ifndef COORD_BUFFER_HPP
#define COORD_BUFFER_HPP

#include "buffer.hpp"

class CoordBuffer : public Buffer {
    public:
        void producer(const coord_buffer& item);
};

#endif  // COORD_BUFFER_HPP