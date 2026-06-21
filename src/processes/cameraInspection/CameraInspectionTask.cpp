#include "processes/cameraInspection/CameraInspectionTask.hpp"

#include <semaphore.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>

static int dbg_anomaly_count = 0;
static bool is_inspecting = false;

static sem_t wakeup_sem;

static volatile sig_atomic_t anomaly_flag = 0;
static void on_anomaly(int) {
    anomaly_flag = 1;
    sem_post(&wakeup_sem);
}

static volatile sig_atomic_t normal_flag = 0;
static void on_normal(int) {
    normal_flag = 1;
    sem_post(&wakeup_sem);
}

void cameraInspectionHandler() {
    sem_init(&wakeup_sem, 0, 0);
    signal(SIGUSR1, on_anomaly);
    signal(SIGUSR2, on_normal);

    while (true) {
        sem_wait(&wakeup_sem);  // dorme até receber SIGUSR1 ou SIGUSR2

        if (anomaly_flag) {
            anomaly_flag = 0;
            normal_flag = 0;
            dbg_anomaly_count++;
            // printf("[CameraInspection] Anomalia detectada. Total: %d\n",
            // dbg_anomaly_count);
            is_inspecting = true;
        }
        if (normal_flag) {
            normal_flag = 0;
            is_inspecting = false;
            // printf("[CameraInspection] Teto normalizado.\n");
        }

        if (is_inspecting) {
            printf("[CameraInspection] Inspecionando imagens da câmera");
            usleep(500000);  // simula tempo gasto na inspeção
        }
    }

    sem_destroy(&wakeup_sem);
}
