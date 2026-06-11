#include "processes/mainProcess/NavigationControlTask.hpp"

#include <unistd.h>

#include <iostream>

const int TASK_PERIOD_MS = 80;

// Velocidade recebida do processo de comando de navegação
int commanded_vel = 0;

int velocityController(int reference, int feedback) {
    int control_action;
    return control_action;
}

void navigationControlHandler(std::binary_semaphore& vel_was_sent,
                              std::binary_semaphore& vel_is_needed,
                              VelBuffer& vel_buffer) {
    auto next_wake = std::chrono::steady_clock::now();
    while (true) {
        // Tarefa com período definido
        next_wake += std::chrono::milliseconds(TASK_PERIOD_MS);
        std::this_thread::sleep_until(next_wake);

        vel_is_needed
            .release();  // anuncia que precisa de informação de velocidade para
                         // consumir informação mais atualizada possível
        vel_was_sent.acquire();
        VelData vel_data = std::get<VelData>(vel_buffer.consumer_latest());
        int feedback_vel = vel_data.vel;


        auto now = std::chrono::steady_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(now - vel_data.timestamp)
                .count();
        printf("[Controle Navegação] latência vel: %.3f ms\n", latency_ms);

    }
}
