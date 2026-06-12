// Na pasta em que está esse arquivo ficarão os canais de comunicação (como
// servidores), indepente da forma de IPC que escolhermos utilizar

#pragma once

#include <pthread.h>
#include <sys/shm.h>

// chave fixa para o segmento de memória compartilhada
#define SHM_NAV_KEY 0x4E415601

// mutexes são inicializados com PTHREAD_PROCESS_SHARED
struct NavInfo {
        float setpoint_vel;
        float current_vel;
        pthread_mutex_t setpoint_mtx;
        pthread_mutex_t feedback_mtx;
        bool initialized;  // true após mutexes inicializados por navCommand
};