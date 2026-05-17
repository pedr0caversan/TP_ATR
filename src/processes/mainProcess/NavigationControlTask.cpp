#include "processes/mainProcess/NavigationControlTask.hpp"
#include <iostream>
#include <unistd.h>

void navigationControlHandler(VelBuffer& vel_buffer) {
    while (true) {

        int distancia_lida = buffer->consumer();
        
        // Processar a distância lida e atualizar as velocidades
        std::cout << "[Controle Navegação] Distância percorrida lida: " << distancia_lida << " m" << std::endl; 
        
        
        // Implementar posteriormente o controle PID para ajustar as velocidades com base na distância percorrida e na trajetória desejada
        usleep(80000); // A tarefa deve ser executada periodicamente a cada 80ms.
    }

}
