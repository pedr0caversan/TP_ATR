#include "processes/mainProcess/DistanceComputationTask.hpp"

#include <unistd.h>

#include <iostream>

const int TASK_PERIOD_MS = 20;
const float METERS_PER_ENCODER_SIGNAL = 1;

static bool i_encoder = false;
static int call_count = 0;  // para simulação do encoder

void simulateEncoder(double t) {
    call_count++;

    // alterna entre estados a cada segundo
    int mode = static_cast<int>(t) % 2;

    if (mode == 0) {
        // muda de estado a toda execução
        i_encoder = !i_encoder;
    } else {
        // muda de estado a cada duas execuções
        if (call_count % 2 == 0) {
            i_encoder = !i_encoder;
        }
    }
}

void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                std::binary_semaphore& x_is_needed,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer) {
    PosData pos_data = {0};
    VelData vel_data = {0};
    bool previous_encoder_state =
        i_encoder;  // Guarda a última leitura do encoder
    auto task_start = std::chrono::steady_clock::now();
    auto prev_encoder_timestamp = task_start;
    auto next_wake = task_start;

    while (true) {
        // Definição do período da tarefa
        next_wake += std::chrono::milliseconds(TASK_PERIOD_MS);
        std::this_thread::sleep_until(next_wake);

        auto now = std::chrono::steady_clock::now();
        double t = std::chrono::duration<double>(now - task_start).count();
        simulateEncoder(t);
        bool current_state = i_encoder;

        if (current_state != previous_encoder_state) {
            /*=================================
            AÇÕES CASO HAJA MUDANÇA DE ESTADO
            ===================================*/

            pos_data.pos += 1;
            previous_encoder_state = current_state;

            // Derivar a velocidade usando tempo real entre eventos
            double dt_s =
                std::chrono::duration<double>(now - prev_encoder_timestamp)
                    .count();
            double velocity = METERS_PER_ENCODER_SIGNAL / dt_s;
            prev_encoder_timestamp = now;
            vel_data.vel = velocity;
        }
        vel_data.timestamp = now;
        pos_data.timestamp = now;
        vel_buffer.producer(vel_data);
        pos_buffer.producer(pos_data);
        if (x_is_needed.try_acquire()) {
            x_was_sent.release();
        }
    }
}
