#ifndef VEL_BUFFER_HPP
#define VEL_BUFFER_HPP

#include "buffer.hpp"

class VelBuffer : public Buffer {
    public:
        void producer(const VelData& item);
};

#endif  // VEL_BUFFER_HPP