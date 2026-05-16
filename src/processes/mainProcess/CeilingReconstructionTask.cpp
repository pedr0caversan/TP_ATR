#include "processes/mainProcess/CeilingReconstructionTask.hpp"

#include <unistd.h>

#include <chrono>
#include <thread>

#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"

// Etapa 1

// Filtro EMA (Exponential Moving Average) foi escolhido no lugar de SMA (Simple
// Moving Average) por também suavizar os dados, mas dar peso maior a dados
// recentes, evitando que haja muito atraso na percepção de uma mudança brusca
// na superfície
struct EMAFilter {
        bool is_initialized = false;
        float value = 0;
};

float filterValue(EMAFilter& f, float new_value) {
    // implementação do filtro
    return new_value;
}

struct RefinedData {
        int x;
        int y;
        std::chrono::steady_clock::time_point
            time_stamp;  // verificar o tipo das variáveis
};

void ceilingReconstructionHandler(std::binary_semaphore& x_was_sent,
                                  PosBuffer& pos_buf, CoordBuffer& coord_buf) {
    EMAFilter f_x;
    EMAFilter f_y;
    auto next_wake = std::chrono::steady_clock::now();
    while (true) {
        next_wake += std::chrono::milliseconds(100);
        std::this_thread::sleep_until(next_wake);

        RefinedData refined_data = {0};

        x_was_sent.acquire();  // garante que o valor da coordenada no buffer de
                               // cima já foi atualizado
        int x_coord = std::get<PosData>(pos_buf.consumer()).pos;

        int y_coord = 1;  // aqui eu leio o valor percebido pelo lidar

        refined_data.x = filterValue(f_x, x_coord);
        refined_data.y = filterValue(f_y, y_coord);
        refined_data.time_stamp = std::chrono::steady_clock::now();

        // aqui a refined_data deve ser enviada ao buffer de baixo
    }
}
