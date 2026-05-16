#ifndef POS_BUFFER_HPP
#define POS_BUFFER_HPP

#include "buffer.hpp"

class PosBuffer : public Buffer {
    public:
        void producer(const PosData& item);
};

#endif  // POS_BUFFER_HPP