#include <sstream>
#include <iostream>
#include <thread>

#include "minmea/minmea.h"
#include "ServeurEnvoiWebsocket.h"
#include "gps_utility.cpp"
using namespace std;

void handleGPS(shared_ptr<ServeurEnvoiWebSocket> serveurEnvoi, istream& is){
    char line[MINMEA_MAX_LENGTH];
    while (is >> line){
        minmea_sentence_rmc frame{};
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            minmea_parse_rmc(&frame, line);
            serveurEnvoi->ajouter(to_json(frame));
        }
    }
}

int main (int argc, const char * argv[]) {
    auto server_address = argc > 1 ? argv[1] : "ws://localhost:8080";
    auto testMode = argc > 2 && strcmp(argv[2], "-t") == 0;
    shared_ptr<ServeurEnvoiWebSocket> serveurEnvoi{new ServeurEnvoiWebSocket(server_address)};
    thread gpsThread{handleGPS, serveurEnvoi, std::ref(cin)};
    gpsThread.join();
}
