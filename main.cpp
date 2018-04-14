#include <sstream>
#include <iostream>
#include <thread>

#include "minmea/minmea.h"
#include "ServeurEnvoiWebsocket.h"
#include "gps_utility.h"
#include "MotionSensor/MotionSensor/inv_mpu_lib/inv_mpu_dmp_motion_driver.h"
#include "MotionSensor/MotionSensor/inv_mpu_lib/inv_mpu.h"

using namespace std;
atomic<bool> termine;
static u_int16_t accelerometerPollingRate = 200; //1000 Hz

void handleGPS(string adresseServeur, istream& is){
    char line[MINMEA_MAX_LENGTH];
    ServeurEnvoiWebSocket ws(adresseServeur);
    while (!termine && is >> line){
        minmea_sentence_rmc frame{};
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            minmea_parse_rmc(&frame, line);
            ws.ajouter(to_json(frame));
        }
    }
}

class ErreurInitialisationSensor{};

void handleTap(uint8_t orientation, uint8_t count){
    cout << "Tap!" << endl;
    //if(count == 2)
    //   termine = true;
}

void initAccelerometer(){
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
    if (dmp_enable_feature(DMP_FEATURE_TAP|DMP_FEATURE_PEDOMETER)!=0) {
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
    if(dmp_register_tap_cb(handleTap)!=0){
        throw ErreurInitialisationSensor();
    };

    int16_t a[3];              // [x, y, z]            accel vector
    int16_t g[3];              // [x, y, z]            gyro vector
    int32_t _q[4];
    int16_t sensors;
    uint8_t fifoCount;
    uint8_t r;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/accelerometerPollingRate));  //dmp will habve 4 (5-1) packets based on the fifo_rate
            r=dmp_read_fifo(g,a,_q,&sensors,&fifoCount);
    } while (r!=0 || fifoCount<5); //packtets!!!//
    //ms_open();
}

void handleAccelerometer(){
    int16_t a[3];              // [x, y, z]            accel vector
    int16_t g[3];              // [x, y, z]            gyro vector
    int32_t _q[4];
    int16_t sensors;
    uint8_t fifoCount;
    while (!termine){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/accelerometerPollingRate));  //dmp will habve 4 (5-1) packets based on the fifo_rate
        dmp_read_fifo(g,a,_q,&sensors,&fifoCount);
    }
}

int main (int argc, const char * argv[]) {
    auto adresseServeur = argc > 1 ? argv[1] : "ws://localhost:8080";
    auto testMode = argc > 2 && "-t" == argv[2];
    initAccelerometer();
    thread gpsThread{handleGPS, adresseServeur, std::ref(cin)};
    thread accelerometerThread{handleAccelerometer};
    gpsThread.join();
    accelerometerThread.join();
}
