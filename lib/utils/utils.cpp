#include "utils.h"

int buffer[BUFFER_SIZE];
int buffer_index = 0;
int last_trend = 0;

void add_to_buffer(int val) {
    buffer[buffer_index] = val;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
}

int check_trend() {
    int trend = 0;
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
        int diff = buffer[(buffer_index + i) % BUFFER_SIZE] - buffer[(buffer_index + i + 1) % BUFFER_SIZE];
        if (diff < 0) {
            if (trend > 0) return 0; // No clear trend, fluctuating
            trend = -1;
        } else if (diff > 0) {
            if (trend < 0) return 0; // No clear trend, fluctuating
            trend = 1;
        }
    }
    return trend;
}


int water_level_is_trending(int val) {

    add_to_buffer(val);
    int current_trend = check_trend();
    if (current_trend != last_trend && current_trend != 0) {
        if (current_trend > 0) {
            // Trend is up
            return 1;
        } else {
            // Trend is down
            return 1;
        }
    }
    last_trend = current_trend;

    // no trend change
    return 0;
}