#include "processes/mainProcess/CeilingReconstructionTask.hpp"

#include <sys/shm.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <csignal>
#include <thread>

#include "IPC/IPCData.hpp"
#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"

const int SIMULATION_PERIOD_S = 1;
// período da task em ms
const int T_MS = 100;
const float EMA_ALPHA =
    0.5;  // grau de sensibilidade do filtro EMA em relação a valores novos

int i_lidar = 200;

// DEBUG
static int dbg_i = 0;

void simulateLidarSensor(double t) {
    i_lidar = static_cast<int>(
        200 + 100 * std::cos(2 * M_PI * SIMULATION_PERIOD_S * t));
}

// Filtro EMA (Exponential Moving Average) foi escolhido no lugar de SMA (Simple
// Moving Average) por também suavizar os dados, mas dar peso maior a dados
// recentes, evitando que haja muito atraso na percepção de uma mudança brusca
// na superfície
struct EMAFilter {
        bool is_initialized = false;
        float value = 0;
};

float filterValue(EMAFilter& f, float new_value) {
    if (!f.is_initialized) {
        f.value = new_value;
        f.is_initialized = true;
    } else {
        f.value = EMA_ALPHA * new_value + (1.0f - EMA_ALPHA) * f.value;
    }
    return f.value;
}

void ceilingReconstructionHandler(std::binary_semaphore& x_was_sent,
                                  std::binary_semaphore& x_is_needed,
                                  PosBuffer& pos_buf, CoordBuffer& coord_buf) {
    EMAFilter f_x;
    EMAFilter f_y;
    auto task_start = std::chrono::steady_clock::now();
    auto next_wake = task_start;

    /*obtenção dos PIDs de NavCommand e CameraInspection pela memória
    compartilhada para realização do IPC por signal loop da task */
    int shmid = shmget(SHM_PID_SHARING_KEY, sizeof(SignalPIDs), 0666);
    if (shmid < 0) {
        perror("shmget SignalPIDs");
        return;
    }
    SignalPIDs* pids = (SignalPIDs*)shmat(shmid, nullptr, SHM_RDONLY);
    if (pids == (void*)-1) {
        perror("shmat SignalPIDs");
        return;
    }
    while (!pids->ready) {  // aguarda pai preencher os PIDs
        usleep(1000);
    }
    pid_t pid_nav_command = pids->pid_nav_command;
    pid_t pid_cam_inspection = pids->pid_camera;
    shmdt(pids);  // memória compartilhada desanexada
    printf("[CeilingReconstruction] PIDs obtidos: navCmd=%d, camera=%d\n",
           pid_nav_command, pid_cam_inspection);

    while (true) {
        next_wake += std::chrono::milliseconds(T_MS);
        std::this_thread::sleep_until(next_wake);

        x_is_needed.release();
        x_was_sent.acquire();  // garante que o valor da coordenada no
        // buffer de cima já foi atualizado antes de consumi-lo
        PosData pos_data = std::get<PosData>(pos_buf.consumer_latest());
        int x_coord = pos_data.pos;

        auto now = std::chrono::steady_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(now - pos_data.timestamp)
                .count();
        // printf("[Reconstrução do Teto] latência x: %.3f ms\n", latency_ms);

        double t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - task_start)
                       .count();
        simulateLidarSensor(t);  // simulação para testes
        int y_coord =
            i_lidar;  // aqui deve ser lido o valor percebido pelo lidar

        CoordData refined_data = {std::chrono::steady_clock::now(), {0, 0}};
        refined_data.coord[0] = filterValue(f_x, x_coord);
        refined_data.coord[1] = filterValue(f_y, y_coord);

        // TODO (Pedro): implementar condicional de detecção de anomalia
        dbg_i++;
        bool anomaly_detected = (dbg_i % 10 == 0);
        if (anomaly_detected) {
            kill(pid_nav_command, SIGUSR1);
            kill(pid_cam_inspection, SIGUSR1);
        }

        // envio dos dados percebidos pelo lidar e encoder ao buffer de
        // coordenadas
        coord_buf.producer(refined_data);
    }
}
