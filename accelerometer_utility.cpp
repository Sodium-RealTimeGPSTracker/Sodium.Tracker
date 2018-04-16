//
// Created by Hugo Boisselle on 2018-04-16.
//

#include "accelerometer_utility.h"

#include <iostream>
#include <thread>
#include <sstream>

#include "inv_mpu_dmp_motion_driver.h"
#include "inv_mpu.h"

double AccelerometerDataEntry::getAverageAccel()
{
    if(avgAccel == -1)
        avgAccel = sqrt(pow(accel[0], 2) + pow(accel[1], 2) + pow(accel[2],2));
    return avgAccel;
}

string AccelerometerDataEntry::to_string()
{
    std::stringstream ss{};
    ss << "(" << accel[0] << "," << accel[1] << ","<< accel[2] << ") "<<getAverageAccel();
    return ss.str();
}

uint16_t initAccelerometer(void (*tapCallback)(uint8_t, uint8_t))
{
    class ErreurInitialisationSensor{};

    if (mpu_init(NULL) != 0) {
        cerr << "MPU init failed!" << endl;
        throw ErreurInitialisationSensor();
    }

    cout << "Setting MPU sensors..." << endl;
    if (mpu_set_sensors(INV_XYZ_GYRO|INV_XYZ_ACCEL)!=0) {
        cerr << "Failed to set sensors!" << endl;
        throw ErreurInitialisationSensor();
    }

    cout << "Setting GYRO sensitivity..." << endl;
    if (mpu_set_gyro_fsr(2000)!=0) {
        cerr << "Failed to set gyro sensitivity!" << endl;
        throw ErreurInitialisationSensor();
    }

    cout << "Setting ACCEL sensitivity..." << endl;
    if (mpu_set_accel_fsr(2)!=0) {
        cerr << "Failed to set accel sensitivity!" << endl;
        throw ErreurInitialisationSensor();
    }

    // verify connection
    cout << "Powering up MPU..." << endl;
    uint8_t devStatus;
    mpu_get_power_state(&devStatus);
    if(devStatus){
        cout << "MPU6050 connection successful " << endl;
    }else{
        cout << "MPU6050 connection failed " << devStatus << endl;
        throw ErreurInitialisationSensor();
    }

    //fifo config
    cout << "Setting MPU fifo..." << endl;
    if (mpu_configure_fifo(INV_XYZ_ACCEL)!=0) {
        cerr << "Failed to initialize MPU fifo!" << endl;
        throw ErreurInitialisationSensor();
    }

    // load and configure the DMP
    cout << "Loading DMP firmware..." << endl;
    if (dmp_load_motion_driver_firmware()!=0) {
        cerr << "Failed to enable DMP!" << endl;
        throw ErreurInitialisationSensor();
    }

    cout << "Activating DMP..." << endl;
    if (mpu_set_dmp_state(1)!=0) {
        cerr << "Failed to enable DMP!" << endl;
        throw ErreurInitialisationSensor();
    }

    cout << "Configuring DMP..." << endl;
    if (dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT|DMP_FEATURE_SEND_RAW_ACCEL|
                           DMP_FEATURE_SEND_CAL_GYRO|DMP_FEATURE_GYRO_CAL|
                           DMP_FEATURE_TAP|DMP_FEATURE_PEDOMETER)!=0) {
        throw ErreurInitialisationSensor();
    }

    cout << "Setting polling rate..." << endl;
    if (dmp_set_fifo_rate(accelerometerPollingRate)!=0) {
        throw ErreurInitialisationSensor();
    }
    cout << "Setting polling rate..." << endl;
    if (mpu_reset_fifo()!=0) {
        throw ErreurInitialisationSensor();
    }

    cout << "Setting tap callback.." << endl;

    u_int16_t accelerometerSens;
    dmp_get_fifo_rate(&accelerometerPollingRate);
    mpu_get_accel_sens(&accelerometerSens);

    int16_t g[3], sensors;
    int32_t q[4];
    uint8_t r, fifoCount;

    cout << "Accelerometer calibration..." << endl;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/accelerometerPollingRate));
        r=dmp_read_fifo(g,accelerometerOffset,q,&sensors,&fifoCount);

    } while (r!=0);
    if(dmp_register_tap_cb(tapCallback)!=0){
        throw ErreurInitialisationSensor();
    };
    cout << "Accelerometer ready!" << endl;
    return accelerometerSens;
}