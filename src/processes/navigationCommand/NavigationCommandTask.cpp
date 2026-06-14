#include "processes/navigationCommand/NavigationCommandTask.hpp"

#include <sys/shm.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "IPC/IPCData.hpp"

static volatile sig_atomic_t anomaly_flag = 0;
static void on_anomaly(int) { anomaly_flag = 1; }

static int dbg_anomaly_count = 0;

void navigationCommandHandler() {
    signal(SIGUSR1, on_anomaly);
    // Abre memória compatilhada para trocar informações com NavigationControl
    int shmid = shmget(SHM_NAV_KEY, sizeof(NavInfo), 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget");
        return;
    }

    NavInfo* shm = (NavInfo*)shmat(shmid, nullptr, 0);
    if (shm == (void*)-1) {
        perror("shmat");
        return;
    }

    shm->initialized = false;
    shm->setpoint_vel = 0.0f;
    shm->current_vel = 0.0f;

    // Inicializa o mutex como process-shared para funcionar entre processos
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->setpoint_mtx, &attr);
    pthread_mutex_init(&shm->feedback_mtx, &attr);
    pthread_mutexattr_destroy(&attr);

    shm->initialized = true;  // sinaliza que mutexes estão prontos

    while (true) {
        float nova_vel = 10;  // TODO (Pedro): receber do operador remoto

        pthread_mutex_lock(&shm->setpoint_mtx);
        shm->setpoint_vel = nova_vel;
        pthread_mutex_unlock(&shm->setpoint_mtx);

        if (anomaly_flag) {
            anomaly_flag = 0;
            dbg_anomaly_count++;
            printf("[NavCommand] Anomalia detectada. Total: %d\n", dbg_anomaly_count);
            // TODO (Pedro): reduzir setpoint de velocidade
        }

        pthread_mutex_lock(&shm->feedback_mtx);
        float current_vel = shm->current_vel;
        pthread_mutex_unlock(&shm->feedback_mtx);

        // TODO (Pedro): transmitir current_vel por MQTT
    }

    shmdt(shm);
}
