//
// Created by Hugo Boisselle on 2018-03-18.
//
#ifndef GPS_UTILITY_H
#define GPS_UTILITY_H

#include <sstream>
#include "minmea/minmea.h"

using namespace std;

std::string to_json(minmea_sentence_rmc& rmc_sentence, double avgSpeed, int stepCount)
{
    std::ostringstream os;
    os <<
       "{"
               "\"lat\":{"
               "\"v\":" << rmc_sentence.latitude.value << ","
               "\"s\":" << rmc_sentence.latitude.scale <<
       "},"
               "\"lon\":{"
               "\"v\":" << rmc_sentence.longitude.value << ","
               "\"s\":" << rmc_sentence.longitude.scale <<
       "},"
               "\"t\":{"
               "\"h\":" << rmc_sentence.time.hours << ","
               "\"m\":" << rmc_sentence.time.minutes << ","
               "\"s\":" << rmc_sentence.time.seconds << ","
               "\"us\":" << rmc_sentence.time.microseconds <<
       "},"
               "\"spd\":" << minmea_tofloat(&rmc_sentence.speed) * 1.852 << "," <<
               "\"step\":" << stepCount << "," <<
               "\"aspd\":" << avgSpeed <<
       "}";
    return os.str();
}

#endif
