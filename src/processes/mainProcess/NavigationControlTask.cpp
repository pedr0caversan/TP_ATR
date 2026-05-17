#include "processes/mainProcess/NavigationControlTask.hpp"
#include <iostream>
#include <unistd.h>
#include <semaphore.h>

/*
#define BUFFER_SIZE 10

extern int buffer_navegacao[BUFFER_SIZE];
extern int buffer_out; // Atenção: o Consumidor usa o índice 'out' (saída)

extern sem_t mutex_buffer_nav;
extern sem_t spaces_nav;
extern sem_t items_nav;
*/

void navigationControlHandler(VelBuffer& vel_buffer) {
    while (true) {
        int distancia_lida;
        /*
        sem_wait(&items_nav);        // Espera até que exista pelo menos 1 item válido no buffer
        sem_wait(&mutex_buffer_nav); // Adquire acesso exclusivo ao buffer
        
        // Início da Seção Crítica
        distancia_lida = buffer_navegacao[buffer_out];
        buffer_out = (buffer_out + 1) % BUFFER_SIZE; // Avança o índice de forma circular
        // Fim da Seção Crítica
        */

        std::cout << "[Controle Navegação] Distância percorrida lida: " << distancia_lida << " m" << std::endl; 
        // Processar a distância lida e atualizar as velocidades
        // Implementar posteriormente o controle PID para ajustar as velocidades com base na distância percorrida e na trajetória desejada
    }

        usleep(80000); // A tarefa deve ser executada periodicamente a cada 80ms.
}
