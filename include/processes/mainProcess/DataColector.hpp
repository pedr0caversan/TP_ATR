#include <chrono>
#include <vector>

#include "utils/coord_buffer.hpp"

#pragma once

struct formated_data {
        std::chrono::steady_clock::time_point timestamp;
        int coord[2];
        float confidence;
};

void updateHistory(std::vector<coord_buffer>& history,
                   const coord_buffer& new_item);

coord_buffer getBufferData(CoordBuffer* buffer);

float calcConfidence(const coord_buffer& item,
                     std::vector<coord_buffer>& history);

void saveDataDB(const coord_buffer& item, float confidence);

void saveDataTopic(const coord_buffer& item, float confidence);

void dataColectorHandler();
