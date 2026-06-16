#include "processes/cameraInspection/CameraInspectionTask.hpp"

#include <unistd.h>

#include <csignal>

#include <cstdio>

static int dbg_anomaly_count = 0;

static volatile sig_atomic_t anomaly_flag = 0;
static void on_anomaly(int) { anomaly_flag = 1; }

void cameraInspectionHandler() {
    signal(SIGUSR1, on_anomaly);

    while (true) {
        if (anomaly_flag) {
            anomaly_flag = 0;
            dbg_anomaly_count++;
            //printf("[CameraInspection] Anomalia detectada. Total: %d\n", dbg_anomaly_count);
            // TODO (Pedro): implemenar lógica de inspeção
        }
    }
}
