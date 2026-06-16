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
const float AUTO_VELOCITY = 10.0f;

static int dbg_anomaly_count = 0;

void on_message_nav_cmd(struct mosquitto* mosq, void* userdata,
                        const struct mosquitto_message* message) {
    NavInfo* shm =
        (NavInfo*)userdata;  // Recupera o ponteiro da memória compartilhada
    std::string topic(message->topic);
    std::string payload((char*)message->payload);

    if (topic == "atr/cmd/velocidade") {
        float nova_vel = std::stof(payload);
        pthread_mutex_lock(&shm->setpoint_mtx);
        shm->setpoint_vel = nova_vel;  // Atualiza o setpoint
        pthread_mutex_unlock(&shm->setpoint_mtx);
        printf("[NavCommand] Recebido via MQTT -> Novo Setpoint: %.2f\n",
               nova_vel);
    }
}

void navigationCommandHandler() {
    signal(SIGUSR1, on_anomaly);
    signal(SIGUSR2, on_normal);
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

    mosquitto_lib_init();
    struct mosquitto* mqtt_nav =
        mosquitto_new("robo_nav_cmd", true, shm);  // Passa 'shm' como userdata
    mosquitto_message_callback_set(mqtt_nav, on_message_nav_cmd);
    mosquitto_connect(mqtt_nav, "localhost", 1883, 60);
    mosquitto_subscribe(mqtt_nav, NULL, "atr/cmd/#", 0);
    mosquitto_loop_start(mqtt_nav);  // Roda em background

    while (true) {
        if (anomaly_flag) {
            anomaly_flag = 0;
            normal_flag = 0;
            is_inspecting = true;
            dbg_anomaly_count++;
            // printf("[NavCommand] Anomalia detectada. Total: %d\n",
            // dbg_anomaly_count);
        }
        if (normal_flag) {
            normal_flag = 0;
            is_inspecting = false;
            // printf("[NavCommand] Teto normalizado. Retomando velocidade
            // normal.\n");
        }
        // TODO (Pedro): receber do operador remoto
        float nova_vel = AUTO_VELOCITY;  // Para teste

        pthread_mutex_lock(&shm->setpoint_mtx);
        shm->setpoint_vel = is_inspecting ? nova_vel / 4 : nova_vel;
        pthread_mutex_unlock(&shm->setpoint_mtx);

        pthread_mutex_lock(&shm->feedback_mtx);
        float current_vel = shm->current_vel;
        pthread_mutex_unlock(&shm->feedback_mtx);

        std::string vel_str = std::to_string(current_vel);
        mosquitto_publish(mqtt_nav, NULL, "atr/telemetria/velocidade",
                          vel_str.length(), vel_str.c_str(), 0, false);

        std::string insp_str = is_inspecting ? "1" : "0";
        mosquitto_publish(mqtt_nav, NULL, "atr/telemetria/inspecao",
                          insp_str.length(), insp_str.c_str(), 0, false);
    }

    shmdt(shm);
}
