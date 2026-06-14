#include "processes/mainProcess/DataColector.hpp"

#include <unistd.h>

#include "utils/coord_buffer.hpp"

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

void saveDataDB(const CoordData& item, float confidence) {
    // Implementation for saving data to database
    FormatedData formatted;
    formatted.timestamp = item.timestamp;
    formatted.coord[0] = item.coord[0];
    formatted.coord[1] = item.coord[1];
    formatted.confidence = confidence;

    // printf(
    //     "[Coletor de Dados] Salvando na database: (%.2f, %.2f) com confiança %.2f\n",
    //     formatted.coord[0], formatted.coord[1], formatted.confidence);
}

void saveDataTopic(const CoordData& item, float confidence) {
    // Implementation for saving data to topic
    FormatedData formatted;
    formatted.timestamp = item.timestamp;
    formatted.coord[0] = item.coord[0];
    formatted.coord[1] = item.coord[1];
    formatted.confidence = confidence;

    // printf(
    //     "[Coletor de Dados] Publicando no Tópico: (%.2f, %.2f) com confiança %.2f\n",
    //     formatted.coord[0], formatted.coord[1], formatted.confidence);
}

void dataColectorHandler(CoordBuffer& coord_buf) {
    static std::vector<CoordData> history;
    while (true) {
        CoordData item = getBufferData(&coord_buf);
        float confidence = calcConfidence(item, history);
        saveDataDB(item, confidence);
        saveDataTopic(item, confidence);
        updateHistory(history, item);
    }
}
