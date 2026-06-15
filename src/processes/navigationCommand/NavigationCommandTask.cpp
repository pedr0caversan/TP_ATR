#include "processes/navigationCommand/NavigationCommandTask.hpp"

#include <sys/shm.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mosquitto.h>

#include "IPC/IPCData.hpp"


static volatile sig_atomic_t anomaly_flag = 0;
static void on_anomaly(int) { anomaly_flag = 1; }

static int dbg_anomaly_count = 0;


void on_message_nav_cmd(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    NavInfo* shm = (NavInfo*)userdata; // Recupera o ponteiro da memória compartilhada
    std::string topic(message->topic);
    std::string payload((char*)message->payload);

    if (topic == "atr/cmd/velocidade") {
        float nova_vel = std::stof(payload);
        pthread_mutex_lock(&shm->setpoint_mtx);
        shm->setpoint_vel = nova_vel; // Atualiza o setpoint
        pthread_mutex_unlock(&shm->setpoint_mtx);
        printf("[NavCommand] Recebido via MQTT -> Novo Setpoint: %.2f\n", nova_vel);
    }
}


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

    
    mosquitto_lib_init();
    struct mosquitto *mqtt_nav = mosquitto_new("robo_nav_cmd", true, shm); // Passa 'shm' como userdata
    mosquitto_message_callback_set(mqtt_nav, on_message_nav_cmd);
    mosquitto_connect(mqtt_nav, "localhost", 1883, 60);
    mosquitto_subscribe(mqtt_nav, NULL, "atr/cmd/#", 0);
    mosquitto_loop_start(mqtt_nav); // Roda em background
    

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

        std::string vel_str = std::to_string(current_vel);
        mosquitto_publish(mqtt_nav, NULL, "atr/telemetria/velocidade", vel_str.length(), vel_str.c_str(), 0, false);
        
        
        // TODO (Pedro): transmitir current_vel por MQTT
    }

    shmdt(shm);
}
