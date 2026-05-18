#pragma once

#include <semaphore>

#include "utils/vel_buffer.hpp"

void navigationControlHandler(std::binary_semaphore& vel_was_sent,
                              std::binary_semaphore& vel_is_needed,
                              VelBuffer& vel_buffer);
