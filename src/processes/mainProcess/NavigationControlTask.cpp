#include "processes/mainProcess/NavigationControlTask.hpp"
#include <iostream>
#include <unistd.h>

void navigationControlHandler(VelBuffer& vel_buffer) {
    auto next_wake = std::chrono::steady_clock::now();
    while (true) {
        // Tarefa com período de 80 ms
        next_wake += std::chrono::milliseconds(80);
        std::this_thread::sleep_until(next_wake);

        int robot_velocity = std::get<VelData>(vel_buffer.consumer()).vel;
        
        // Processar a distância lida e atualizar as velocidades
        std::cout << "[Controle Navegação] Velocidade lida: " << robot_velocity << " m" << std::endl; 
        
        
        // Implementar posteriormente o controle PID para ajustar a velocidade do robô
    }

}
