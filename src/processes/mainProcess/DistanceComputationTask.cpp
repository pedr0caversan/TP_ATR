#include "processes/mainProcess/DistanceComputationTask.hpp"

#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "utils/analise.hpp"

const int PRINT_EVERY_N = 5;

Medicao dc_exec{"DistanceComputation"};
Medicao dc_jitter{"DistanceComputation/jitter"};

const int T_MS = 4;
const float METERS_PER_ENCODER_SIGNAL = 1;

std::atomic<bool> mqtt_i_encoder{false};
std::atomic<float> mqtt_i_sim_vel{
    0.0f};  // velocidade com sinal publicada pela simulação


void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                std::binary_semaphore& x_is_needed,
                                std::binary_semaphore& vel_was_sent,
                                std::binary_semaphore& vel_is_needed,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer) {
    PosData pos_data = {0, std::chrono::steady_clock::now()};
    VelData vel_data = {0, std::chrono::steady_clock::now()};

    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC via MQTT (indireta) — lê estado atual do encoder publicado por
    // MainProcessInit a partir do tópico atr/sim/encoder
    bool previous_encoder_state = mqtt_i_encoder.load();
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    auto task_start = std::chrono::steady_clock::now();
    auto prev_encoder_timestamp = task_start;
    auto next_wake = task_start;

    // ============================================================
    // loop da task
    // ============================================================

    while (true) {
        // Definição do período da tarefa
        next_wake += std::chrono::milliseconds(T_MS);
        std::this_thread::sleep_until(next_wake);
        dc_jitter.jitter(next_wake, 250);
        dc_exec.inicio();

        auto now = std::chrono::steady_clock::now();

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via MQTT (indireta) — lê novo estado do encoder via atômica
        // atualizada pelo callback MQTT de MainProcessInit
        bool current_state = mqtt_i_encoder.load();
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        static int print_counter = 0;
        if (print_counter++ % PRINT_EVERY_N == 0) {
            // printf("[Distance Computation] leitura do encoder: %d\n",
            // current_state);
        }

        if (current_state != previous_encoder_state) {
            /*=================================
            AÇÕES CASO HAJA MUDANÇA DE ESTADO
            ===================================*/

            // usa o sinal da velocidade simulada para determinar a direção do
            // movimento; permite pos_data.pos decrementar ao andar para
            // esquerda
            int direction = (mqtt_i_sim_vel.load() >= 0.0f) ? 1 : -1;

            pos_data.pos += direction;
            // printf("[Distance Computation] posição do robô: %d\n",
            //        pos_data.pos);
            previous_encoder_state = current_state;

            // Derivar a velocidade usando tempo real entre eventos
            double dt_s =
                std::chrono::duration<double>(now - prev_encoder_timestamp)
                    .count();
            if (dt_s > 0) {
                double velocity = direction * METERS_PER_ENCODER_SIGNAL / dt_s;
                vel_data.vel = velocity;
                // printf("[Distance Computation] velocidade do robô: %.2f
                // m/s\n",
                //        vel_data.vel);
            }

            prev_encoder_timestamp = now;
        }

        vel_data.timestamp = now;
        pos_data.timestamp = now;

        vel_buffer.producer(vel_data);
        pos_buffer.producer(pos_data);

        // ########################################################################
        // Sincronização de threads por dupla de semáforos
        // Notifica CeilingReconstructionTask que a posição foi atualizada no
        // buffer, se ela já sinalizou que está esperando
        if (x_is_needed.try_acquire()) {
            x_was_sent.release();
        }
        // Notifica NavigationControlTask que a velocidade foi atualizada no
        // buffer, se ela já sinalizou que está esperando
        if (vel_is_needed.try_acquire()) {
            vel_was_sent.release();
        }
        // ########################################################################
        dc_exec.fim(250);
    }
}
