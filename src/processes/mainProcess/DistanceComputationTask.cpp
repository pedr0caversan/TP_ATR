#include "processes/mainProcess/DistanceComputationTask.h"

#include <unistd.h>

// Etapa 1

void distanceComputationHandler(std::binary_semaphore& x_was_sent) {
    while (true) {
        // Na hora de colocar a coordenada x no buffer, indicar pelo semáforo
        // que foi enviado para a task de reconstrução da seguinte forma
        // x_was_sent.release()
    }
}
