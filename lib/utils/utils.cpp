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


// ==  read voltage from current transformer sensor == //
float get_current_transformer_reading(Adafruit_ADS1115 ads) {
    float sumOfSquares = 0;
    int numSamples = 100;

    // Configure A0 & A1 for the ±2.048V range
    ads.setGain(GAIN_TWO);
    ads.setDataRate(RATE_ADS1115_860SPS);

    long sum = 0;
    for (int i = 0; i < 100; i++) {
        int16_t adc0 = ads.readADC_Differential_0_1();

        float voltage = ads.computeVolts(adc0);

        sumOfSquares += voltage * voltage;

        delay(10); // Small delay to avoid overwhelming the ADC
    }

    float meanOfSquares = sumOfSquares / numSamples;
    float rmsVoltage = sqrt(meanOfSquares);

    return rmsVoltage;
}

// ==  read voltage from pressure sensor == //
float get_pressure_sensor_reading(Adafruit_ADS1115 ads) {

    // Configure A3 for the ±4.096V range
    ads.setGain(GAIN_ONE);  // GAIN_ONE sets the range to ±4.096V
    long sum = 0;
    for (int i = 0; i < 100; i++) {
        int16_t adc3 = ads.readADC_SingleEnded(3);
        sum += adc3;
        delay(10); // Small delay to avoid overwhelming the ADC
    }

    int16_t average = sum / 100;
    return ads.computeVolts(average);  // Convert to voltage using computeVolts()
}
