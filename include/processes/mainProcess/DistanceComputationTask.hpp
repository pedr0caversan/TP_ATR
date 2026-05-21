#pragma once

#include <chrono>
#include <semaphore>
#include <thread>

#include "utils/pos_buffer.hpp"
#include "utils/vel_buffer.hpp"

void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                std::binary_semaphore& x_is_needed,
                                std::binary_semaphore& vel_was_sent,
                                std::binary_semaphore& vel_is_needed,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer);
