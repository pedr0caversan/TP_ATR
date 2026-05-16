#include "processes/mainProcess/CeilingReconstructionTask.hpp"

#include <unistd.h>

#include <chrono>
#include <cmath>
#include <thread>

#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"

int i_lidar = 200;

void simulateLidarSensor(double t) {
    i_lidar = static_cast<int>(200 + 100 * std::cos(t));
}

// Filtro EMA (Exponential Moving Average) foi escolhido no lugar de SMA (Simple
// Moving Average) por também suavizar os dados, mas dar peso maior a dados
// recentes, evitando que haja muito atraso na percepção de uma mudança brusca
// na superfície
struct EMAFilter {
        bool is_initialized = false;
        float value = 0;
};

float filterValue(EMAFilter& f, float new_value) {
    // TODO (Pedro): implementação do filtro
    return new_value;
}

void ceilingReconstructionHandler(std::binary_semaphore& x_was_sent,
                                  PosBuffer& pos_buf, CoordBuffer& coord_buf) {
    EMAFilter f_x;
    EMAFilter f_y;
    auto task_start = std::chrono::steady_clock::now();
    auto next_wake = task_start;
    // loop da task
    while (true) {
        next_wake += std::chrono::milliseconds(100);
        std::this_thread::sleep_until(next_wake);

        x_was_sent.acquire();  // garante que o valor da coordenada no
        // buffer de cima já foi atualizado antes de consumi-lo
        int x_coord = std::get<PosData>(pos_buf.consumer()).pos;

        double t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - task_start)
                       .count();
        simulateLidarSensor(t); // simulação para testes
        int y_coord =
            i_lidar;  // aqui deve ser lido o valor percebido pelo lidar

        CoordData refined_data = {std::chrono::system_clock::now(), {0, 0}};
        refined_data.coord[0] = filterValue(f_x, x_coord);
        ;
        refined_data.coord[1] = filterValue(f_y, y_coord);
        ;

        // envio dos dados percebidos pelo lidar e encoder ao buffer de
        // coordenadas
        coord_buf.producer(refined_data);
    }
}
