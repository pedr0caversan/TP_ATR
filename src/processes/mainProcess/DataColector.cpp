#include "processes/mainProcess/DataColector.hpp"

#include <mosquitto.h>
#include <unistd.h>

#include <iostream>

#include "utils/coord_buffer.hpp"

extern struct mosquitto* mqtt_client_main;

void updateHistory(std::vector<CoordData>& history, const CoordData& new_item) {
    history.push_back(new_item);
    if (history.size() > 50) {
        history.erase(history.begin());
    }
}

CoordData getBufferData(CoordBuffer* buffer) {
    return std::get<CoordData>(buffer->consumer());
}

float calcConfidence(const CoordData& item, std::vector<CoordData>& history) {
    if (history.empty()) {
        return 0.0;
    }
    float conf = 0.0;
    for (const auto& past_item : history) {
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            item.timestamp - past_item.timestamp);
        // Caso diferença de tempo menor que 100ms, aumenta a confiança
        if (time_diff.count() < 500) {
            conf += 0.1;
        }
    }
    return conf;
}

void saveDataDisk(const CoordData& item, float confidence) {
    const std::string filename = "data_log.csv";
    bool write_header = false;
    static bool first_run = true;

    // Verifica se o arquivo existe e está vazio
    {
        std::ifstream check(filename);
        if (!check.good() ||
            check.peek() == std::ifstream::traits_type::eof()) {
            write_header = true;
        }
        check.close();
    }

    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Erro: Não foi possível abrir arquivo " << filename
                  << " (pwd: " << getcwd(nullptr, 0) << ")" << std::endl;
        return;
    }

    if (first_run) {
        std::cerr << "[DataColector] Arquivo de log criado/aberto em: "
                  << filename << std::endl;
        first_run = false;
    }

    if (write_header) {
        file << "timestamp,coord_x,coord_y,confidence\n";
        std::cerr << "[DataColector] Header escrito no CSV" << std::endl;
    }

    const auto timestamp_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            item.timestamp.time_since_epoch())
            .count();

    file << timestamp_ms << ',' << item.coord[0] << ',' << item.coord[1] << ','
         << confidence << '\n';

    // Force sincronização com disco
    file.flush();
}

void saveDataTopic(const CoordData& item, float confidence) {
    if (mqtt_client_main) {
        std::string payload = "{\"x\": " + std::to_string(item.coord[0]) +
                              ", \"y\": " + std::to_string(item.coord[1]) +
                              ", \"confianca\": " + std::to_string(confidence) +
                              "}";

        mosquitto_publish(mqtt_client_main, NULL, "atr/telemetria/log",
                          payload.length(), payload.c_str(), 0, false);
    }
}

void dataColectorHandler(CoordBuffer& coord_buf) {
    static std::vector<CoordData> history;

    // ============================================================
    // loop da task
    // ============================================================

    while (true) {
        // Sincronização de threads por buffer bloqueante
        CoordData item = getBufferData(&coord_buf);

        float confidence = calcConfidence(item, history);

        saveDataDisk(item, confidence);

        saveDataTopic(item, confidence);

        updateHistory(history, item);
    }
}
