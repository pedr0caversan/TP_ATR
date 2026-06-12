// Na pasta em que está esse arquivo ficarão os canais de comunicação (como
// servidores), indepente da forma de IPC que escolhermos utilizar

#pragma once

#include <pthread.h>
#include <sys/shm.h>
#include <sys/types.h>

// chave para o segmento de memória compartilhada de navegação
#define SHM_NAV_KEY 0x4E415601

// chave para o compartilhamento de PIDs entre processos
#define SHM_PID_SHARING_KEY 0x52454701

// Objeto para armazenar PIDs que são preenchidos pelo processo pai (main) após
// todos os forks para IPC por signal
struct SignalPIDs {
        pid_t pid_main_process;
        pid_t pid_nav_command;
        pid_t pid_camera;
        bool ready;  // true após pai preencher todos os PIDs
};

// mutexes são inicializados com PTHREAD_PROCESS_SHARED
struct NavInfo {
        float setpoint_vel;
        float current_vel;
        pthread_mutex_t setpoint_mtx;
        pthread_mutex_t feedback_mtx;
        bool initialized;  // true após mutexes inicializados por navCommand
};