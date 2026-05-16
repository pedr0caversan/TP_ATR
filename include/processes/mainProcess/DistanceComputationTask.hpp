#pragma once

#include <semaphore>

#include "utils/pos_buffer.hpp"
#include "utils/vel_buffer.hpp"

void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer);
