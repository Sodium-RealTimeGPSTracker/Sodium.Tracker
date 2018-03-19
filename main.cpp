#include <sstream>
#include <iostream>
#include <cmath>
#include <cassert>
#include <thread>

#include "minmea/minmea.h"
#include "easywsclient/easywsclient.hpp"
#include "tracker.h"

using easywsclient::WebSocket;

void const clear_console(){
    std::cout << "\033[H\033[2J";
}

int main (int argc, const char * argv[]) {
    static WebSocket::pointer ws;
    auto server_address = argc > 1 ? argv[1] : "ws://192.168.0.185:8080";
    auto testMode = argc > 2 && strcmp(argv[2], "-t") == 0;

    ws = WebSocket::from_url(server_address);

    assert(ws);

    char line[MINMEA_MAX_LENGTH];
    while (std::cin >> line){
        if(minmea_sentence_id(line, false) == MINMEA_SENTENCE_RMC){
            if(testMode){
                // Taux de rafraîchissement réel du module gps
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            handle_rmc_sentence(ws, line);
        }
    }
}
