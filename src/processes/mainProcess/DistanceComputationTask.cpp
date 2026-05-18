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

            // 1. Derivar a velocidade 
            double delta_t = ciclos_20ms * 0.02;
            double velocidade = 1.0 / delta_t;
            
            // Reseta o contador para a próxima medição
            ciclos_20ms = 0; 

            // Monta a estrutura VelData (ajuste os campos conforme a sua struct real)
            VelData vel_data;
            vel_data.velocidade = velocidade; 

            // 2. Insere os dados nos respectivos buffers
            vel_buffer->producer(vel_data);
            dist_buffer->producer(distancia_total);

            // 3. Padrão Signaling: faz o release para acordar a thread de reconstrução
            if (sem_reconstrucao != nullptr) {
                sem_reconstrucao->release();
            }
        }
        }

        // A tarefa deve ser executada periodicamente a cada 20ms.
        usleep(20000);
            
    } 
    
}
