#include <sstream>
#include <iostream>
#include <thread>

#include "minmea/minmea.h"
#include "ServeurEnvoiWebsocket.h"
#include "gps_utility.h"
#include "MotionSensorExample/MotionSensor/inv_mpu_lib/inv_mpu_dmp_motion_driver.h"
#include "MotionSensorExample/MotionSensor/inv_mpu_lib/inv_mpu.h"
#include "ntuple_buffer.h"
#include "AccelerometerDataEntry.h"

using namespace std;
atomic<bool> termine;
static u_int16_t accelerometerPollingRate = 200; // 200 Hz
static u_int16_t gpsPollingRate = 1; // 1 Hz

template <int NBUF, int SZBUF>
void handleGPS(istream& is, ntuple_buffer<minmea_sentence_rmc, NBUF, SZBUF>& buff){
    char line[MINMEA_MAX_LENGTH];
    while (!termine && is >> line){ // Bloquant
        minmea_sentence_rmc frame{};
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            minmea_parse_rmc(&frame, line);
            buff.ajouter(&frame, &frame + 1); // On sait que ça n'arrivera pas plus souvent que 1Hz
        }
    }
}

class ErreurInitialisationSensor{};

void handleTap(uint8_t orientation, uint8_t count){
    cout << "Tap!" << endl;
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

    AccelerometerDataEntry e{};
    uint8_t r;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000/accelerometerPollingRate));
            r=dmp_read_fifo(e.g,e.a,e._q,&(e.sensors),&(e.fifoCount));
    } while (r!=0 || e.fifoCount<5); //packtets!!!//
    //ms_open();
}

template <int NBUF, int SZBUF>
void handleAccelerometer(ntuple_buffer<AccelerometerDataEntry, NBUF, SZBUF>& buff){
    while (!termine){
        AccelerometerDataEntry e{};
        this_thread::sleep_for(chrono::milliseconds(1000/accelerometerPollingRate));
        while(dmp_read_fifo(e.g,e.a,e._q,&(e.sensors),&(e.fifoCount))!=0);
        dmp_get_pedometer_step_count(&(e.stepCount));
        buff.ajouter(&e, &e + 1);
    }
}

int main (int argc, const char * argv[]) {
    auto adresseServeur = argc > 1 ? argv[1] : "ws://localhost:8080";
    auto testMode = argc > 2 && "-t" == argv[2];
    ntuple_buffer<minmea_sentence_rmc, 2, 2> gpsDataBuf{};
    ntuple_buffer<AccelerometerDataEntry, 2, 1000> accelerometerDataBuf{};

    initAccelerometer();
    atomic<bool> moving{false};
    thread gpsSensorAcquisitionThread(handleGPS<2, 2>, std::ref(cin), std::ref(gpsDataBuf));
    thread accelSensorAcquisitionThread(handleAccelerometer<2, 1000>, std::ref(accelerometerDataBuf));

    thread accelDataHandlingThread([&]{
        int prevStepCount{0};
        while(!termine) {
            auto donnees = accelerometerDataBuf.extraire_tout();
            if (!donnees.empty()) {
                auto stepCount = rbegin(donnees)->stepCount;
                if (stepCount > prevStepCount) {
                    moving = true;
                    cout << "moving" << endl;
                    prevStepCount = stepCount;
                }
            }
        }
    });

    static ServeurEnvoiWebSocket ws(adresseServeur);

    thread gpsDataHandlingThread([&]{
        while(!termine){
            auto donnees = gpsDataBuf.extraire_tout();
            if(moving && !donnees.empty()){
                cout << "sent position!" << endl;
                ws.ajouter(to_json(*rbegin(donnees)));
                moving = false;
            }
        }
    });

    gpsSensorAcquisitionThread.join();
    gpsDataHandlingThread.join();
    accelSensorAcquisitionThread.join();
    accelDataHandlingThread.join();
}
