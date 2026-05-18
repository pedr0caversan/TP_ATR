#include "processes/mainProcess/NavigationControlTask.hpp"
#include <iostream>
#include <unistd.h>

const int TASK_PERIOD_MS = 80;

void navigationControlHandler(VelBuffer& vel_buffer) {
    auto next_wake = std::chrono::steady_clock::now();
    while (true) {
        // Tarefa com período definido
        next_wake += std::chrono::milliseconds(TASK_PERIOD_MS);
        std::this_thread::sleep_until(next_wake);

        VelData vel_data = std::get<VelData>(vel_buffer.consumer_latest());
        int robot_velocity = vel_data.vel;

        auto now = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(
                                now - vel_data.timestamp)
                                .count();
        std::cout << "[Controle Navegação] latência vel: " << latency_ms << " ms" << std::endl;
        
        
        // Implementar posteriormente o controle PID para ajustar a velocidade do robô
    }

}
