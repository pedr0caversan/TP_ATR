#ifndef POS_BUFFER_HPP
#define POS_BUFFER_HPP

#include "buffer.hpp"

class PosBuffer : public Buffer {
    public:
        void producer(const pos_buffer& item);
};

#endif  // POS_BUFFER_HPP