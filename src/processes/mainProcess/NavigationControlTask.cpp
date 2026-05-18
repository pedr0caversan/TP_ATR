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

        int robot_velocity = std::get<VelData>(vel_buffer.consumer_latest()).vel;
        
        // Processar a distância lida e atualizar as velocidades
        std::cout << "[Controle Navegação] Velocidade lida: " << robot_velocity << " m" << std::endl; 
        
        
        // Implementar posteriormente o controle PID para ajustar a velocidade do robô
    }

}
