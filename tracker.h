//
// Created by Hugo Boisselle on 2018-03-18.
//

#ifndef SODIUM_TRACKER_TRACKER_H
#define SODIUM_TRACKER_TRACKER_H

#include "easywsclient/easywsclient.hpp"
#include "minmea/minmea.h"

using easywsclient::WebSocket;

void handle_rmc_sentence(WebSocket::pointer ws, const char* line);

#endif //SODIUM_TRACKER_TRACKER_H
