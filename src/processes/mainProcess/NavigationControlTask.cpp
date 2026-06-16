#include "processes/mainProcess/NavigationControlTask.hpp"

#include <mosquitto.h>
#include <sys/shm.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "IPC/IPCData.hpp"

extern struct mosquitto* mqtt_client_main;

const int T_MS = 80;

// Parâmetros do controlador PI
const float Kp = 1;
const float Ki = 0.1;

// TODO (Pedro): sintonizar controlador assim que tiver modelo da planta
float velocityController(float reference, float feedback) {
    printf("[Controle Navegação] setpoint: %.2f | feedback: %.2f\n", reference, feedback);
    static float integral = 0.0f;
    const float T_S = T_MS / 1000.0f;
    const float U_MAX = 100.0f;

    float e = reference - feedback;
    // integral += e * T_S;

    // float u = Kp * e + Ki * integral;

    // // trata saturação do atuador e congela integrador quando saturado
    // if (u > U_MAX) {
    //     u = U_MAX;
    //     integral -= e * T_S;
    // } else if (u < -U_MAX) {
    //     u = -U_MAX;
    //     integral -= e * T_S;
    // }

    // TODO (Pedro): reativar controlador
    // para testes iniciais, a dinâmica da planta vai ser deixada livre, sem
    // controle
    return e;
}

void navigationControlHandler(std::binary_semaphore& vel_was_sent,
                              std::binary_semaphore& vel_is_needed,
                              VelBuffer& vel_buffer) {
    // Aguarda navCommand criar e inicializar a memória compartilhada
    int shmid = -1;
    NavInfo* navigation_info = nullptr;
    while (navigation_info == nullptr || !navigation_info->initialized) {
        if (shmid < 0) {
            shmid = shmget(SHM_NAV_KEY, sizeof(NavInfo), 0666);
            if (shmid >= 0) {
                navigation_info =
                    (NavInfo*)shmat(shmid, nullptr, 0);  // copia informação
                if (navigation_info ==
                    (void*)-1) {  // erro de shmat retorna todos bits em 1
                    perror("shmat navControl");
                    return;
                }
            }
        }
        usleep(1000);
    }

    auto next_wake = std::chrono::steady_clock::now();
    while (true) {
        // Tarefa com período definido
        next_wake += std::chrono::milliseconds(T_MS);
        std::this_thread::sleep_until(next_wake);

        vel_is_needed
            .release();  // anuncia que precisa de informação de velocidade para
                         // consumir informação mais atualizada possível
        vel_was_sent.acquire();
        VelData vel_data = std::get<VelData>(vel_buffer.consumer_latest());
        float feedback_vel = vel_data.vel;

        // printf("[Controle Navegação] Velocidade atual: %.2f\n",
        // feedback_vel);

        // obtenção do setpoint por memória compartilhada com proteção por mutex
        pthread_mutex_lock(&navigation_info->setpoint_mtx);
        float setpoint = navigation_info->setpoint_vel;
        // printf("[Controle Navegação] Setpoint atual: %.2f\n",setpoint);
        pthread_mutex_unlock(&navigation_info->setpoint_mtx);

        // calcula esforço de controle de 0 a 100%
        float control_effort = velocityController(setpoint, feedback_vel);
        //printf("[Controle Navegação] Esforço: %.2f\n", control_effort);

        // envio do esforço de controle para a simulação
        if (mqtt_client_main != nullptr) {
            // control_effort = 10; // TODO (Pedro): retirar linha de teste
            std::string u_str = std::to_string(control_effort);
            mosquitto_publish(mqtt_client_main, NULL,
                              "atr/sim/esforco_controle", u_str.length(),
                              u_str.c_str(), 0, false);
        }

        // envia velocidade atual para navCommand repassar para display
        pthread_mutex_lock(&navigation_info->feedback_mtx);
        navigation_info->current_vel = feedback_vel;
        pthread_mutex_unlock(&navigation_info->feedback_mtx);

        // auto now = std::chrono::steady_clock::now();
        // double latency_ms =
        //     std::chrono::duration<double, std::milli>(now -
        //     vel_data.timestamp)
        //         .count();
        // printf(
        //     "[Controle Navegação] setpoint: %.2f | vel: %.2f | latência: %.3f
        //     " "ms\n", setpoint, feedback_vel, latency_ms);
    }

    shmdt(navigation_info);
}
