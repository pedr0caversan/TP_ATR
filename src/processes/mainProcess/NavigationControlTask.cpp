#include "processes/mainProcess/NavigationControlTask.hpp"

#include <mosquitto.h>
#include <sys/shm.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "IPC/IPCData.hpp"
#include "utils/analise.hpp"

extern struct mosquitto* mqtt_client_main;

Medicao nc_exec{"NavigationControl"};
Medicao nc_jitter{"NavigationControl/jitter"};
Medicao nc_bloqueio{"NavigationControl/sem"};

const int T_MS = 16;

// Parâmetros do controlador PI
const float Kp = 5.0f;
const float Ki = 0.1;

// NOTE (Pedro): controlador PI não funcionou
float velocityController(float reference, float feedback) {
    // printf("[Controle Navegação] setpoint: %.2f | feedback: %.2f\n",
    // reference,
    //        feedback);
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
    return Kp * e;
}

void navigationControlHandler(std::binary_semaphore& vel_was_sent,
                              std::binary_semaphore& vel_is_needed,
                              VelBuffer& vel_buffer) {
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC por memória compartilhada — aguarda NavigationCommandTask criar e
    // inicializar o segmento NavInfo antes de anexá-lo
    int shmid = -1;
    NavInfo* navigation_info = nullptr;
    while (navigation_info == nullptr || !navigation_info->initialized) {
        if (shmid < 0) {
            shmid = shmget(SHM_NAV_KEY, sizeof(NavInfo), 0666);
            if (shmid >= 0) {
                navigation_info = (NavInfo*)shmat(shmid, nullptr, 0);
                if (navigation_info == (void*)-1) {
                    perror("shmat navControl");
                    return;
                }
            }
        }
        usleep(1000);
    }
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    auto next_wake = std::chrono::steady_clock::now();

    // ============================================================
    // loop da task
    // ============================================================

    while (true) {
        // Tarefa com período definido
        next_wake += std::chrono::milliseconds(T_MS);
        std::this_thread::sleep_until(next_wake);
        nc_jitter.jitter(next_wake, 62);
        nc_exec.inicio();

        // ########################################################################
        // Sincronização de threads por dupla de semáforos
        // Sinaliza que precisa da velocidade e aguarda DistanceComputationTask
        // confirmar que o buffer foi atualizado
        vel_is_needed.release();
        nc_bloqueio.inicio();
        vel_was_sent.acquire();
        nc_bloqueio.fim(62);
        VelData vel_data = std::get<VelData>(vel_buffer.consumer_latest());
        float feedback_vel = vel_data.vel;
        // ########################################################################

        // printf("[Controle Navegação] Velocidade atual: %.2f\n",
        // feedback_vel);

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC por memória compartilhada — lê setpoint definido por
        // NavigationCommandTask com proteção por mutex
        pthread_mutex_lock(&navigation_info->setpoint_mtx);
        float setpoint = navigation_info->setpoint_vel;
        // printf("[Controle Navegação] Setpoint atual: %.2f\n",setpoint);
        pthread_mutex_unlock(&navigation_info->setpoint_mtx);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // calcula esforço de controle de 0 a 100%
        float control_effort = velocityController(setpoint, feedback_vel);
        // printf("[Controle Navegação] Esforço: %.2f\n", control_effort);

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via MQTT — publica erro de velocidade em atr/sim/esforco_controle
        // para a simulação aplicar o controlador PI interno na planta
        if (mqtt_client_main != nullptr) {
            std::string u_str = std::to_string(control_effort);
            mosquitto_publish(mqtt_client_main, NULL,
                              "atr/sim/esforco_controle", u_str.length(),
                              u_str.c_str(), 0, false);
        }
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC por memória compartilhada — escreve velocidade atual na NavInfo
        // para NavigationCommandTask publicar como telemetria
        pthread_mutex_lock(&navigation_info->feedback_mtx);
        navigation_info->current_vel = feedback_vel;
        pthread_mutex_unlock(&navigation_info->feedback_mtx);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        nc_exec.fim(62);
    }

    shmdt(navigation_info);
}
