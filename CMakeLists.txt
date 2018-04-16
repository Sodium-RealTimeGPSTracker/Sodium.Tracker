cmake_minimum_required(VERSION 3.3)
project(Sodium.Tracker)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -pthread")
set(SOURCE_FILES
        main.cpp
        minmea/minmea.c minmea/minmea.h
        easywsclient/easywsclient.cpp easywsclient/easywsclient.hpp
        gps_utility.h
        serveur_envoi_websocket.cpp serveur_envoi_websocket.h
        ntuple_buffer.h accelerometer_utility.cpp accelerometer_utility.h)

add_executable(Sodium.Tracker ${SOURCE_FILES})
target_link_libraries(Sodium.Tracker /usr/lib/libMotionSensor.a /usr/lib/libI2Cdev.a)
