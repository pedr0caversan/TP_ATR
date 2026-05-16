#pragma once

#include <semaphore>

#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"

void ceilingReconstructionHandler(std::binary_semaphore& x_was_sent,
                                  PosBuffer& pos_buf, CoordBuffer& coord_buf);
