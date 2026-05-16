#include "processes/mainProcess/DistanceComputationTask.hpp"

#include <unistd.h>

#include "utils/pos_buffer.hpp"
#include "utils/vel_buffer.hpp"

// Etapa 1

void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer) {
    while (true) {
        // Na hora de colocar a coordenada x no buffer, indicar pelo semáforo
        // que foi enviado para a task de reconstrução da seguinte forma
        // x_was_sent.release()
    }
}
