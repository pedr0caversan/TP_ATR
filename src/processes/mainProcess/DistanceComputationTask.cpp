#include "processes/mainProcess/DistanceComputationTask.h"
#include <unistd.h>
#include <iostream>

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

            // Insere a distância total na fila para o processo de controle de velocidade usando a função pública "Producer"

            buffer->producer(distancia_total);
        }

        // A tarefa deve ser executada periodicamente a cada 20ms.
        usleep(20000);
            
    } 
    
}
