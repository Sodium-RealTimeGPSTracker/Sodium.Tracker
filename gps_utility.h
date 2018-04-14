//
// Created by Hugo Boisselle on 2018-03-18.
//
#ifndef GPS_UTILITY_H
#define GPS_UTILITY_H

#include <cmath>
#include <string>
#include <sstream>
#include "minmea/minmea.h"

using namespace std;

double toRadians(double degree);
double getDistance(double lat1, double lon1, double lat2, double lon2);
std::string to_json(minmea_sentence_rmc& rmc_sentence);

#endif
