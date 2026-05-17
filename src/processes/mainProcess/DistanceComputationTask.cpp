#include "processes/mainProcess/DistanceComputationTask.h"
#include <unistd.h>
#include <iostream>
#include <semaphore.h>


// Declarar referências externas

/*
#define BUFFER_SIZE 10

extern int buffer_navegacao[BUFFER_SIZE];
extern int buffer_in;
extern bool i_encoder;

extern sem_t mutex_buffer_nav;
extern sem_t spaces_nav;
extern sem_t items_nav;
*/


void distanceComputationHandler()
{
    int distancia_total = 0;
    bool estado_anterior_encoder = i_encoder; //Guarda a última leitura do encoder

    while (true)
    {
        bool estado_atual = i_encoder;

        // Verificar se houve uma mudança no estado do encoder, se sim, incrementa a distância total

        if (estado_atual != estado_anterior_encoder)
        {
            distancia_total += 1;
            estado_anterior_encoder = estado_atual;

            /*
            sem_wait(&spaces_nav);       // Espera haver espaço livre no buffer
            sem_wait(&mutex_buffer_nav); // Adquire acesso exclusivo ao buffer

            // Início da Seção Crítica
            
            buffer_navegacao[buffer_in] = distancia_total;
            buffer_in = (buffer_in + 1) % BUFFER_SIZE;

            //Fim da Seção Crítica
            

            sem_post(&mutex_buffer_nav); // Libera o acesso exclusivo
            sem_post(&items_nav);        // Avisa a tarefa consumidora que há um novo item

            */
    }

    // A tarefa deve ser executada periodicamente a cada 20ms.
    usleep(20000);
    
}
