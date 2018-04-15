//
// Created by Hugo Boisselle on 2018-04-14.
//

#ifndef SODIUM_TRACKER_ACCELEROMETERDATAENTRY_H
#define SODIUM_TRACKER_ACCELEROMETERDATAENTRY_H

#include <cstdint>

struct AccelerometerDataEntry {
    int16_t a[3];              // [x, y, z]            accel vector
    int16_t g[3];              // [x, y, z]            gyro vector
    int32_t _q[4];
    int16_t sensors;
    uint8_t fifoCount;
    uint32_t stepCount;
};

#endif //SODIUM_TRACKER_ACCELEROMETERDATAENTRY_H
