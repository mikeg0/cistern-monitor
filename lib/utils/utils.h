#include <Adafruit_ADS1X15.h>

#define BUFFER_SIZE 5

void add_to_buffer(int val);
int check_trend();
int water_level_is_trending(int val);

float get_current_transformer_reading(Adafruit_ADS1115 ads);
float get_pressure_sensor_reading(Adafruit_ADS1115 ads);