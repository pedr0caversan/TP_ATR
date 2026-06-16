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

const float CEILING_HEIGHT = 5.64;
const float TOLERANCE = 0.5f;  // tolerância para detecção de anomalia no teto

std::atomic<float> mqtt_i_lidar{
    CEILING_HEIGHT};  // valor do sensor LIDAR atualizado pela simulação via
                      // MQTT
std::atomic<bool> mqtt_lidar_ready{
    false};  // sinaliza que ao menos uma leitura real foi recebida via MQTT
const int LIDAR_SIMULATION_T_S = 1;
// período da task em ms
const int T_MS = 100;
const float EMA_ALPHA =
    0.5;  // grau de sensibilidade do filtro EMA em relação a valores novos

static bool is_inspecting = false;

// DEBUG
static int dbg_i = 0;

float i_lidar = 200;
void simulateLidarSensor(double t) {
    i_lidar = (200 + 100 * std::cos(2 * M_PI * LIDAR_SIMULATION_T_S * t));
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

    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC por memória compartilhada (temporário para troca de PIDs)
    // Pré-requisito para IPC por eventos
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
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    printf("[CeilingReconstruction] PIDs obtidos: navCmd=%d, camera=%d\n",
           pid_nav_command, pid_cam_inspection);

    // ============================================================
    // loop da task
    //============================================================

    while (true) {
        next_wake += std::chrono::milliseconds(T_MS);
        std::this_thread::sleep_until(next_wake);

        // ########################################################################
        // Sincronização de threads por dupla de semáforos
        // Garante que o valor da coordenada já foi atualizado no buffer pela
        // thread de Cálculo de Distância antes de consumi-lo
        x_is_needed.release();
        x_was_sent.acquire();
        PosData pos_data = std::get<PosData>(pos_buf.consumer_latest());
        int x_coord = pos_data.pos;

        // ########################################################################

        // TODO (Pedro): apagar esses cálculos na versão final para evitar
        // chamadas ao sistema desnecessárias
        auto now = std::chrono::steady_clock::now();
        double latency_ms =
            std::chrono::duration<double, std::milli>(now - pos_data.timestamp)
                .count();
        // printf("[Reconstrução do Teto] latência x: %.3f ms\n", latency_ms);

        double t = std::chrono::duration<double>(
                       std::chrono::steady_clock::now() - task_start)
                       .count();
        float y_coord = mqtt_i_lidar.load();
        printf("[Reconstrução do Teto] x: %d, y: %.2f, latência: %.3f ms\n",
               x_coord, y_coord, latency_ms);

        CoordData refined_data = {std::chrono::steady_clock::now(), {0, 0}};
        refined_data.coord[0] = filterValue(f_x, x_coord);
        refined_data.coord[1] = filterValue(f_y, y_coord);

        bool anomaly_detected = mqtt_lidar_ready.load() &&
                                ((y_coord > CEILING_HEIGHT + TOLERANCE) ||
                                 (y_coord < CEILING_HEIGHT - TOLERANCE));
        // printf("[Reconstrução do Teto] y e altura - altura esperada: %.2f
        // m\n", y_coord - CEILING_HEIGHT);

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via sinais para notificar threads de Comando de Navegação e
        // Inspeção por Câmera sobre anomalia detectada
        if (anomaly_detected && !is_inspecting) {
            is_inspecting = true;
            kill(pid_nav_command, SIGUSR1);
            kill(pid_cam_inspection, SIGUSR1);
        } else if (!anomaly_detected && is_inspecting) {
            is_inspecting = false;
            kill(pid_nav_command, SIGUSR2);
            kill(pid_cam_inspection, SIGUSR2);
        }
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // envio dos dados percebidos pelo lidar e encoder ao buffer de
        // coordenadas
        coord_buf.producer(refined_data);
    }
}
