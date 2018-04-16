//
// Created by Hugo Boisselle on 2018-04-16.
//

#ifndef SODIUM_TRACKER_ACCELEROMETER_UTILITY_H
#define SODIUM_TRACKER_ACCELEROMETER_UTILITY_H

#include <cstdint>
#include <cmath>
#include <array>
#include <string>

using namespace std;

static u_int16_t accelerometerPollingRate = 200; // 200 Hz

struct AccelerometerDataEntry {
public:
    array<float, 3> accel{};
    int stepCount{};
private:
    double avgAccel{-1};
public:
    AccelerometerDataEntry() = default;
    AccelerometerDataEntry(const AccelerometerDataEntry& autre) = default;
    AccelerometerDataEntry(const int16_t a[3], uint32_t stepCount, uint16_t sens);

    double getAverageAccel();
    string to_string();
};

u_int16_t initAccelerometer(void (*tapCallback)(uint8_t, uint8_t));

#endif //SODIUM_TRACKER_ACCELEROMETER_UTILITY_H
