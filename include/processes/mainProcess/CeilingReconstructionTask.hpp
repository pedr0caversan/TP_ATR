#pragma once

#include <chrono>
#include <cmath>
#include <cstdio>
#include <semaphore>
#include <thread>

#include "utils/analise.hpp"
#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"

extern Medicao cr_exec;
extern Medicao cr_jitter;
extern Medicao cr_bloqueio;

void ceilingReconstructionHandler(std::binary_semaphore& x_was_sent,
                                  std::binary_semaphore& x_is_needed,
                                  PosBuffer& pos_buf, CoordBuffer& coord_buf);
