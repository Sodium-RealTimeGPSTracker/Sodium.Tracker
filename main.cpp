#include <sstream>
#include <iostream>
#include <thread>
#include <numeric>
#include <fstream>
#include "minmea/minmea.h"
#include "serveur_envoi_websocket.h"
#include "gps_utility.h"
#include "accelerometer_utility.h"
#include "inv_mpu_dmp_motion_driver.h"
#if defined(TEST)
#include "test_utility.h"
#endif
using namespace std;

static ServeurEnvoiWebSocket* serveurEnvoiWebSocket{};

atomic<bool> termine{false};  // Signale la fin du programme, inutilisé pour l'instant


// Fonctions d'acquisition des données
using GPSDataEntry = minmea_sentence_rmc;
template <int NBUF, int SZBUF>
void handleGPS(istream& is, ntuple_buffer<GPSDataEntry, NBUF, SZBUF>& buff)
{
    char line[MINMEA_MAX_LENGTH];
    while (!termine && is >> line){ // Bloquant
#if defined(TEST_GPS_SENSOR_ACQUISITION)
        tester_gps_sensor(1, [&](){
#endif
        minmea_sentence_rmc frame{};
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            minmea_parse_rmc(&frame, line);
            buff.ajouter(&frame, &frame + 1); // Raisonnable considérant un taux d'échantillonnage de 1Hz
        }
#if defined(TEST_GPS_SENSOR_ACQUISITION)
        });
#endif
    }
}

template <int NBUF, int SZBUF>
void handleAccelerometer(ntuple_buffer<AccelerometerDataEntry, NBUF, SZBUF>& buff, uint16_t accelerometerSens)
{
    while (!termine){

        // On veut au minimum remplir notre buffer chaque fois
        this_thread::sleep_for(chrono::milliseconds((1000/accelerometerPollingRate)*SZBUF));
        int16_t a[3], g[3], sensors;
        int32_t q[4];
        uint32_t stepCount;
        uint8_t fifoCount;

        // Lecture jusqu'à ce que le tampon du capteur contiennent au moins 1 élément
        while(dmp_read_fifo(g,a,q,&sensors,&fifoCount) != 0);
#if defined(TEST_ACCEL_SENSOR_ACQUISITION)
        tester_accel_sensor(fifoCount, [&](){
#endif
        vector<AccelerometerDataEntry> data(fifoCount);
        data.emplace_back(a, stepCount, accelerometerSens);

        for(int i = 0; i<fifoCount; ++i){
            if(dmp_read_fifo(g,a,q,&sensors,&fifoCount) == 0){
                data.emplace_back(a, stepCount, accelerometerSens);
                dmp_get_pedometer_step_count(&stepCount);
            }
        }
        buff.ajouter(data);
#if defined(TEST_ACCEL_SENSOR_ACQUISITION)
        return data.size();
    });
#endif
    }
}

// Fonctions d'analyse des données

// Quelques atomiques pour communiquer entre les threads d'analyse...
atomic<bool> started{false};        // Début de l'activité
atomic<bool> accelerating{false};   // Filtrage du mouvement
atomic<int> step_count{0};          // Nombre de pas

void handleTap(uint8_t, uint8_t count)
{
    if(count == 1){
        static auto lastCheckpoint = high_resolution_clock::now();
#if defined(TEST_TAP_DATA_HANDLING)
        tester_tap_handling(1, [&](){
#endif
        auto now = high_resolution_clock::now();
        if (started == false) {
            serveurEnvoiWebSocket->ajouter("{\"chk\":0}");
            started = true;
        } else {
            auto diff = duration_cast<milliseconds>(now - lastCheckpoint).count();
            serveurEnvoiWebSocket->ajouter("{\"chk\":" + to_string(diff) + ",\"cnt\":" + to_string(count) + "}");
        }
        cout <<"bump!" <<endl;

        lastCheckpoint = now;
#if defined(TEST_TAP_DATA_HANDLING)
        });
#endif
    }
}

template <int NBUF, int SZBUF>
void handleAccelerometerData(ntuple_buffer<AccelerometerDataEntry, NBUF, SZBUF>& buff)
{
    static const double movementAccelThreshold{0.6f}; // Seuil où on considère que l'utilisateur est en mouvement
    while(!termine) {
        auto donnees = buff.extraire();

        if (started && !donnees.empty()) {
#if defined(TEST_ACCEL_DATA_HANDLING)
            tester_accel_data_handling(donnees.size(), [&](){
#endif
            auto derniereDonnee = *rbegin(donnees);
            auto beginIt = begin(donnees);
            auto endIt = end(donnees);

            // Filtrage du bruit...
            sort(begin(donnees), end(donnees), [](AccelerometerDataEntry &val1, AccelerometerDataEntry& val2){
                return val1.getAverageAccel() < val2.getAverageAccel();
            });
            advance(beginIt, (int)(0.2 * donnees.size()));
            advance(endIt, (int)(-0.2 * donnees.size()));
            if(beginIt != endIt){
                double avgAccel = accumulate(beginIt,endIt,double{0},
                                             [](long double cumul, AccelerometerDataEntry& data){
                                                 return cumul + data.getAverageAccel();
                                             });
            //...

                avgAccel = avgAccel / (int)distance(beginIt, endIt);
                if (derniereDonnee.stepCount > step_count || avgAccel > movementAccelThreshold) {
                    accelerating = true;
                    step_count = derniereDonnee.stepCount;
                }
            }
#if defined(TEST_ACCEL_DATA_HANDLING)
            });
#endif
        }
    }
}

template <int NBUF, int SZBUF>
void handleGpsData(ntuple_buffer<GPSDataEntry, NBUF, SZBUF>& buff)
{
    double avgSpeed{0};
    int sampleCount{0};
    while(!termine){
        auto donnees = buff.extraire();
        if(started && !donnees.empty()){
#if defined(TEST_GPS_DATA_HANDLING)
            tester_gps_data_handling(donnees.size(), [&](){
#endif
            for(auto it = begin(donnees); it != end(donnees); ++it){
                auto donnee = *it;
                double speed = minmea_tofloat(&donnee.speed)* 1.852; // Noeuds -> km/h
                sampleCount += 1;
                avgSpeed = avgSpeed + (speed - avgSpeed)/sampleCount; // Moyenne mobile cumulative
                if(speed > 3.f || accelerating) {
                    cout <<"mouvement" <<endl;

                    serveurEnvoiWebSocket->ajouter(to_json(donnee, avgSpeed, step_count));
                    accelerating = false;
                }
            }
#if defined(TEST_GPS_DATA_HANDLING)
            });
#endif
        }
    }
};

int main (int argc, const char * argv[])
{
    if(argc < 2){
        cout << "Veuillez entrer l'adresse du serveur websocket..." << endl;
        return -1;
    }
    auto adresseServeur = argv[1];
    enum {
        accelDataAnalysisSampleSize = 20,
        accelDataBufferCount = 60,
        gpsDataAnalysisSampleSize = 1,
        gpsDataBufferCount = 50
    };

    serveurEnvoiWebSocket = new ServeurEnvoiWebSocket(adresseServeur);
    auto accelSens = initAccelerometer(handleTap);

    ntuple_buffer<GPSDataEntry,
        gpsDataBufferCount,
        gpsDataAnalysisSampleSize> gpsDataBuf{};

    ntuple_buffer<
        AccelerometerDataEntry,
        accelDataBufferCount,
        accelDataAnalysisSampleSize> accelerometerDataBuf{};

    thread accelSensorAcquisitionThread(
            handleAccelerometer<accelDataBufferCount,accelDataAnalysisSampleSize>,
            std::ref(accelerometerDataBuf),
            accelSens);

    thread gpsSensorAcquisitionThread(
        handleGPS<gpsDataBufferCount, gpsDataAnalysisSampleSize>,
        std::ref(cin),
        std::ref(gpsDataBuf));

    thread accelDataHandlingThread(
        handleAccelerometerData<accelDataBufferCount,accelDataAnalysisSampleSize>,
        std::ref(accelerometerDataBuf));

    handleGpsData<gpsDataBufferCount, gpsDataAnalysisSampleSize>(gpsDataBuf);

    gpsSensorAcquisitionThread.join();
    accelSensorAcquisitionThread.join();
    accelDataHandlingThread.join();
    delete serveurEnvoiWebSocket;
}
