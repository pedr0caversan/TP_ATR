#include <chrono>
#include <vector>

#include "utils/coord_buffer.hpp"

#pragma once

struct formated_data {
        std::chrono::system_clock::time_point timestamp;
        int coord[2];
        float confidence;
};

void updateHistory(std::vector<CoordData>& history, const CoordData& new_item);

CoordData getBufferData(CoordBuffer* buffer);

float calcConfidence(const CoordData& item, std::vector<CoordData>& history);

void saveDataDB(const CoordData& item, float confidence);

void saveDataTopic(const CoordData& item, float confidence);

void dataColectorHandler(CoordBuffer& coord_buf);
