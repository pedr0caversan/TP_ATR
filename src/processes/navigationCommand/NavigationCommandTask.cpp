#include "processes/navigationCommand/NavigationCommandTask.hpp"

#include <mosquitto.h>
#include <sys/shm.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>

#include "IPC/IPCData.hpp"

static volatile sig_atomic_t anomaly_flag = 0;
static void on_anomaly(int) { anomaly_flag = 1; }

static volatile sig_atomic_t normal_flag = 0;
static void on_normal(int) { normal_flag = 1; }

static bool is_inspecting = false;
static float inspection_direction =
    1.0f;  // direção capturada ao entrar em inspeção
const float INSPEC_VELOCITY = 5.0f;
static float mqtt_velocity = 0;

static int dbg_anomaly_count = 0;

void on_message_nav_cmd(struct mosquitto* mosq, void* userdata,
                        const struct mosquitto_message* message) {
    std::string topic(message->topic);
    std::string payload((char*)message->payload);

    if (topic == "atr/cmd/velocidade") {
        mqtt_velocity = std::stof(payload);
        // printf("[NavCommand] Recebido via MQTT -> Velocidade: %.2f\n",
        //        mqtt_velocity);
    }
}

void navigationCommandHandler() {
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC via sinais POSIX 
    signal(SIGUSR1, on_anomaly);
    signal(SIGUSR2, on_normal);
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC por memória compartilhada — cria e inicializa segmento NavInfo
    // compartilhado com NavigationControlTask para troca de setpoint e feedback
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

    // Inicializa os mutexes como process-shared para funcionar entre processos
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->setpoint_mtx, &attr);
    pthread_mutex_init(&shm->feedback_mtx, &attr);
    pthread_mutexattr_destroy(&attr);

    shm->initialized = true;  // sinaliza que mutexes estão prontos
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC via MQTT - recebe setpoint de velocidade e publica telemetria
    mosquitto_lib_init();
    struct mosquitto* mqtt_nav = mosquitto_new("robo_nav_cmd", true, NULL);
    mosquitto_message_callback_set(mqtt_nav, on_message_nav_cmd);
    mosquitto_connect(mqtt_nav, "localhost", 1883, 60);
    mosquitto_subscribe(mqtt_nav, NULL, "atr/cmd/#", 0);
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    // ============================================================
    // loop da task
    // ============================================================

    anomaly_flag = 0;
    normal_flag = 0;
    while (true) {
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via MQTT — processa mensagens de entrada de forma síncrona
        mosquitto_loop(mqtt_nav, 0, 1);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // IPC por memória compartilhada — lê feedback de velocidade da NavInfo
        pthread_mutex_lock(&shm->feedback_mtx);
        float current_vel = shm->current_vel;
        pthread_mutex_unlock(&shm->feedback_mtx);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via sinais POSIX 
        if (anomaly_flag) {
            anomaly_flag = 0;
            normal_flag = 0;
            // captura direção pelo sinal da velocidade real do robô no momento
            // da anomalia; evita usar nova_vel que pode já ser 0 (tecla solta)
            inspection_direction = (current_vel >= 0.0f) ? 1.0f : -1.0f;
            is_inspecting = true;
        }
        if (normal_flag) {
            normal_flag = 0;
            is_inspecting = false;
        }
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // Usa velocidade recebida por MQTT (ou AUTO_VELOCITY como padrão)
        float nova_vel = mqtt_velocity;

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC por memória compartilhada — escreve setpoint na NavInfo
        // Em inspeção preserva a direção capturada no momento da anomalia;
        // evita forçar marcha à frente quando o robô estava recuando
        pthread_mutex_lock(&shm->setpoint_mtx);
        // printf("[NavCommand] Nova vel: %.2f | Inspecionando: %d\n", nova_vel, is_inspecting);
        shm->setpoint_vel = is_inspecting
                                ? inspection_direction * INSPEC_VELOCITY
                                : nova_vel;
        // printf("is_inspecting: %d | setpoint_vel: %.2f\n", is_inspecting,
        // shm->setpoint_vel);
        pthread_mutex_unlock(&shm->setpoint_mtx);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
        // IPC via MQTT — publica telemetria de velocidade e estado de inspeção
        std::string vel_str = std::to_string(current_vel);
        mosquitto_publish(mqtt_nav, NULL, "atr/telemetria/velocidade",
                          vel_str.length(), vel_str.c_str(), 0, false);

        std::string insp_str = is_inspecting ? "1" : "0";
        mosquitto_publish(mqtt_nav, NULL, "atr/telemetria/inspecao",
                          insp_str.length(), insp_str.c_str(), 0, false);
        // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

        usleep(1000);
    }

    shmdt(shm);
}
