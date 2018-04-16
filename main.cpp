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
        minmea_sentence_rmc frame{};
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            minmea_parse_rmc(&frame, line);
            buff.ajouter(&frame, &frame + 1); // Raisonnable considérant un taux d'échantillonnage de 1Hz
        }
    }
}

template <int NBUF, int SZBUF>
void handleAccelerometer(ntuple_buffer<AccelerometerDataEntry, NBUF, SZBUF>& buff, uint16_t accelerometerSens)
{
    while (!termine){
        this_thread::sleep_for(chrono::milliseconds(1000/accelerometerPollingRate));
        int16_t a[3], g[3], sensors;
        int32_t q[4];
        uint32_t stepCount;
        uint8_t fifoCount;

        // Lecture jusqu'à ce que le tampon du capteur contiennent au moins 1 élément
        while(dmp_read_fifo(g,a,q,&sensors,&fifoCount) != 0);
        vector<AccelerometerDataEntry> data(fifoCount);
        data.emplace_back(a, stepCount, accelerometerSens, accelerometerOffset);

        for(int i = 0; i<fifoCount; ++i){
            dmp_read_fifo(g,a,q,&sensors,&fifoCount);
            data.emplace_back(a, stepCount, accelerometerSens, accelerometerOffset);
            dmp_get_pedometer_step_count(&stepCount);
        }

        buff.ajouter(data);
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
        auto now = high_resolution_clock::now();
        if(started == false){
            serveurEnvoiWebSocket->ajouter("{\"chk\":0}");
            started = true;
        }else{
            auto diff = duration_cast<milliseconds>(now - lastCheckpoint).count();
            serveurEnvoiWebSocket->ajouter("{\"chk\":" + to_string(diff) + ",\"cnt\":"+to_string(count) + "}");
        }
        lastCheckpoint = now;
    }
}

template <int NBUF, int SZBUF>
void handleAccelerometerData(ntuple_buffer<AccelerometerDataEntry, NBUF, SZBUF>& buff)
{
    static const double movementAccelThreshold{0.6f}; // Seuil où on considère que l'utilisateur est en mouvement
    while(!termine) {
        auto donnees = buff.extraire();

        if (started && !donnees.empty()) {
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
            for(auto it = begin(donnees); it != end(donnees); ++it){
                auto donnee = *it;
                double speed = minmea_tofloat(&donnee.speed)* 1.852; // Noeuds -> km/h
                sampleCount += 1;
                avgSpeed = avgSpeed + (speed - avgSpeed)/sampleCount; // Moyenne mobile cumulative
                if(speed > 3.f || accelerating) {
                    serveurEnvoiWebSocket->ajouter(to_json(donnee, avgSpeed, step_count));
                    accelerating = false;
                }
            }
        }
    }
};

int main (int argc, const char * argv[])
{
    if(argc < 2){
        cout <<"Veuillez entrer l'adresse du serveur websocket..." << endl;
        return -1;
    }
    auto adresseServeur = argv[1];
    enum {
        accelDataAnalysisSampleSize = 10,
        accelDataBufferCount = 10,
        gpsDataAnalysisSampleSize = 1,
        gpsDataBufferCount = 10
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

    thread gpsDataHandlingThread(
        handleGpsData<gpsDataBufferCount, gpsDataAnalysisSampleSize>,
        std::ref(gpsDataBuf));

    gpsSensorAcquisitionThread.join();
    gpsDataHandlingThread.join();
    accelSensorAcquisitionThread.join();
    accelDataHandlingThread.join();
    delete serveurEnvoiWebSocket;
}
