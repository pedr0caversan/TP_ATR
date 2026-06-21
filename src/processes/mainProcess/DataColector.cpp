#include "processes/mainProcess/DataColector.hpp"

#include <mosquitto.h>
#include <unistd.h>

#include <iostream>

#include "utils/coord_buffer.hpp"

extern struct mosquitto* mqtt_client_main;

static constexpr int FLUSH_EVERY_N = 10;

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
            item.timestamp -
            past_item.timestamp);  // Caso diferença de tempo menor que 100ms,
                                   // aumenta a confiança
        if (time_diff.count() < 500) {
            conf += 0.1;
        }
    }
    return conf;
}

void saveDataDisk(const CoordData& item, float confidence) {
    const std::string filename = "data_log.csv";
    static bool first_run = true;
    static std::ofstream
        file;  // mantido aberto, evita open e close por escrita
    static int write_count = 0;
    
    if (!file.is_open()) {
        bool write_header = false;

        // Verifica se o arquivo existe e está vazio
        {
            std::ifstream check(filename);
            if (!check.good() ||
                check.peek() == std::ifstream::traits_type::eof()) {
                write_header = true;
            }
            check.close();
        }

        file.open(filename, std::ios::app);
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
    }

    const auto timestamp_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            item.timestamp.time_since_epoch())
            .count();

    file << timestamp_ms << ',' << item.coord[0] << ',' << item.coord[1] << ','
         << confidence << '\n';

    // Flush é uma syscall muito custosa, e para um sistema de arquivos não é
    // crítico que a transferência dos dados para o kernel seja feita toda iteração
    if (++write_count >= FLUSH_EVERY_N) {
        file.flush();
        write_count = 0;
    }
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
