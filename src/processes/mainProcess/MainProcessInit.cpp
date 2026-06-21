#include "processes/mainProcess/MainProcessInit.hpp"

#include <mosquitto.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <semaphore>
#include <string>
#include <thread>

#include "processes/mainProcess/CeilingReconstructionTask.hpp"
#include "processes/mainProcess/DataColector.hpp"
#include "processes/mainProcess/DistanceComputationTask.hpp"
#include "processes/mainProcess/NavigationControlTask.hpp"
#include "utils/coord_buffer.hpp"
#include "utils/pos_buffer.hpp"
#include "utils/vel_buffer.hpp"

struct mosquitto* mqtt_client_main = nullptr;

#ifdef ANALISE
static void on_sigint_main(int) {
    resumo_task("DistanceComputation", &dc_exec, &dc_jitter);
    resumo_task("CeilingReconstruction", &cr_exec, &cr_jitter, &cr_bloqueio);
    resumo_task("NavigationControl", &nc_exec, &nc_jitter, &nc_bloqueio);
    exit(0);
}
#endif

extern std::atomic<bool> mqtt_i_encoder;
extern std::atomic<float> mqtt_i_sim_vel;
extern std::atomic<float> mqtt_i_lidar;
extern std::atomic<bool> mqtt_lidar_ready;

void on_message_main_process(struct mosquitto* mosq, void* userdata,
                             const struct mosquitto_message* message) {
    std::string topic(message->topic);
    std::string payload((char*)message->payload);

    // Se a mensagem for do encoder simulado, atualiza a variável atômica
    if (topic == "atr/sim/encoder") {
        mqtt_i_encoder.store(payload == "1" || payload == "true");
    }
    if (topic == "atr/sim/velocidade") {
        mqtt_i_sim_vel.store(std::stof(payload));
    }
    if (topic == "atr/sim/lidar") {
        mqtt_i_lidar.store(std::stof(payload));
        mqtt_lidar_ready.store(true);
    }
}

void mainProcessInit() {
#ifdef ANALISE
    signal(SIGINT, on_sigint_main);
#endif
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // IPC via MQTT — conecta ao broker, assina tópicos
    mosquitto_lib_init();
    mqtt_client_main = mosquitto_new("robo_main_process", true, NULL);

    mosquitto_message_callback_set(mqtt_client_main, on_message_main_process);

    mosquitto_connect(mqtt_client_main, "localhost", 1883, 60);

    mosquitto_subscribe(mqtt_client_main, NULL, "atr/sim/encoder", 0);

    mosquitto_subscribe(mqtt_client_main, NULL, "atr/sim/velocidade", 0);

    mosquitto_subscribe(mqtt_client_main, NULL, "atr/sim/lidar", 0);

    mosquitto_loop_start(mqtt_client_main);
    // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    CoordBuffer coord_buf;
    PosBuffer pos_buf;
    VelBuffer vel_buf;

    // ########################################################################
    // Sincronização de threads por duplas de semáforos binários
    // x_was_sent / x_is_needed: entre DistanceComputation e
    // CeilingReconstruction vel_was_sent / vel_is_needed: entre
    // DistanceComputation e NavigationControl
    std::binary_semaphore x_was_sent{0};
    std::binary_semaphore x_is_needed{0};
    std::binary_semaphore vel_was_sent{0};
    std::binary_semaphore vel_is_needed{0};
    // ########################################################################

    std::thread t2(ceilingReconstructionHandler, std::ref(x_was_sent),
                   std::ref(x_is_needed), std::ref(pos_buf),
                   std::ref(coord_buf));
    std::thread t3(distanceComputationHandler, std::ref(x_was_sent),
                   std::ref(x_is_needed), std::ref(vel_was_sent),
                   std::ref(vel_is_needed), std::ref(pos_buf),
                   std::ref(vel_buf));
    std::thread t4(navigationControlHandler, std::ref(vel_was_sent),
                   std::ref(vel_is_needed), std::ref(vel_buf));
    std::thread t5(dataColectorHandler, std::ref(coord_buf));

    t2.join();
    t3.join();
    t4.join();
    t5.join();

    if (mqtt_client_main) {
        mosquitto_loop_stop(mqtt_client_main, true);
        mosquitto_destroy(mqtt_client_main);
    }

    mosquitto_lib_cleanup();
}
