#include "processes/mainProcess/NavigationControlTask.hpp"

#include <sys/shm.h>
#include <unistd.h>

#include <iostream>

#include "IPC/Channels.hpp"

const int TASK_PERIOD_MS = 80;

int velocityController(int reference, int feedback) {
    int control_action;
    // TODO (Pedro): implementar lógica do controlador assim que o sistema for
    // modelado
    return control_action;
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
                navigation_info = (NavInfo*)shmat(shmid, nullptr, 0); // copia informação 
                if (navigation_info == (void*)-1) { // erro de shmat retorna todos bits em 1
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
        next_wake += std::chrono::milliseconds(TASK_PERIOD_MS);
        std::this_thread::sleep_until(next_wake);

        vel_is_needed
            .release();  // anuncia que precisa de informação de velocidade para
                         // consumir informação mais atualizada possível
        vel_was_sent.acquire();
        VelData vel_data = std::get<VelData>(vel_buffer.consumer_latest());
        int feedback_vel = vel_data.vel;

        pthread_mutex_lock(&navigation_info->setpoint_mtx);
        float setpoint = navigation_info->setpoint_vel;
        pthread_mutex_unlock(&navigation_info->setpoint_mtx);

        int control_action = velocityController(setpoint, feedback_vel);

        pthread_mutex_lock(&navigation_info->feedback_mtx);
        navigation_info->current_vel = static_cast<float>(feedback_vel);
        pthread_mutex_unlock(&navigation_info->feedback_mtx);

        auto now = std::chrono::steady_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(now - vel_data.timestamp)
                .count();
        printf(
            "[Controle Navegação] setpoint: %.2f | vel: %d | latência: %.3f "
            "ms\n",
            setpoint, feedback_vel, latency_ms);
    }

    shmdt(navigation_info);
}
