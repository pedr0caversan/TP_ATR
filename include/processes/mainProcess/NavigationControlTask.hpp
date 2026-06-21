#pragma once

#include <semaphore>

#include "utils/analise.hpp"
#include "utils/vel_buffer.hpp"

extern Medicao nc_exec;
extern Medicao nc_jitter;
extern Medicao nc_bloqueio;

void navigationControlHandler(std::binary_semaphore& vel_was_sent,
                              std::binary_semaphore& vel_is_needed,
                              VelBuffer& vel_buffer);
