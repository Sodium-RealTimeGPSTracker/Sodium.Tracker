//
// Created by Hugo Boisselle on 2018-03-18.
//
#include <sstream>
#include <iostream>
#include <cmath>

#include "tracker.h"
#include "minmea/minmea.h"

using easywsclient::WebSocket;

static const std::string to_json(minmea_sentence_rmc& rmc){
    std::ostringstream os;
    os <<
       "{"
               "\"lat\":{"
               "\"v\":" << rmc.latitude.value << ","
               "\"s\":" << rmc.latitude.scale <<
       "},"
               "\"lon\":{"
               "\"v\":" << rmc.longitude.value << ","
               "\"s\":" << rmc.longitude.scale <<
       "},"
               "\"t\":{"
               "\"h\":" << rmc.time.hours << ","
               "\"m\":" << rmc.time.minutes << ","
               "\"s\":" << rmc.time.seconds << ","
               "\"us\":" << rmc.time.microseconds <<
       "},"
               "\"spd\":" << minmea_tofloat(&rmc.speed) * 1.852 <<
       "}";
    return os.str();
}

void handle_rmc_sentence(const WebSocket::pointer ws, const char* line){
    minmea_sentence_rmc frame;
    if (minmea_parse_rmc(&frame, line)) {
        ws->send(to_json(frame));
        ws->poll(0);
        std::cout << "." << std::endl;
    }
}
