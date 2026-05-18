#include "processes/mainProcess/DistanceComputationTask.hpp"

#include <unistd.h>

#include <iostream>

static bool i_encoder = false;
static int call_count = 0;  // para simulação do encoder

void simulateEncoder(double t) {
    call_count++;

    // alterna entre estados a cada segundo
    int mode = static_cast<int>(t) % 2;

    if (mode == 0) {
        // muda de estado a toda execução
        i_encoder = !i_encoder;
    } else {
        // muda de estado a cada duas execuções
        if (call_count % 2 == 0) {
            i_encoder = !i_encoder;
        }
    }
}

// TODO (Mylena): usar semáforo x_was_sent pra sincronizar consumo da informação
// de distância pela thread de reconstrução
// TODO (Mylena): derivar velocidade a partir da posição e guardar no buffer
// vel_buffer
void distanceComputationHandler(std::binary_semaphore& x_was_sent,
                                PosBuffer& pos_buffer, VelBuffer& vel_buffer) {
    PosData distancia_total = {0};
    bool estado_anterior_encoder =
        i_encoder;  // Guarda a última leitura do encoder
    auto task_start = std::chrono::steady_clock::now();
    auto next_wake = task_start;

    while (true) {
        // Tarefa com período de 20 ms
        next_wake += std::chrono::milliseconds(20);
        std::this_thread::sleep_until(next_wake);

        double t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - task_start)
                       .count();
        simulateEncoder(t);
        bool estado_atual = i_encoder;

        // Verificar se houve uma mudança no estado do encoder, se sim,
        // incrementa a distância total

        if (estado_atual != estado_anterior_encoder) {
            distancia_total.pos += 1;
            estado_anterior_encoder = estado_atual;

<<<<<<< HEAD
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
=======
            // Insere a distância total na fila para o processo de controle de
            // velocidade usando a função pública "Producer"

            pos_buffer.producer(distancia_total);
>>>>>>> 6600f7b457226964f97161b562b2225babd094c9
        }
    }
}
