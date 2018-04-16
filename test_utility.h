//
// Created by Hugo Boisselle on 2018-04-16.
//

#ifndef SODIUM_TRACKER_TEST_UTILITY_H
#define SODIUM_TRACKER_TEST_UTILITY_H

#include <chrono>
#include <fstream>
#include <vector>
#include <iostream>

using namespace std;
using namespace std::chrono;
template <class F>
high_resolution_clock::duration minuter(F fct)
{
    auto avant = high_resolution_clock::now();
    fct();
    return high_resolution_clock::now() - avant;
}

template <int N>
class tester{
    int n{0};
    ofstream out;
    string testName;
    float periode;
public:
    class TestSuccess{};
    tester(int tauxEchantillonnage, const string& testName):
            periode{(1.f/tauxEchantillonnage)*1000},
            out{testName+"_results.csv", std::ios::out},
            testName{testName}
    {
        out << "ecoule,taille,tempsMax" << endl;
    }

    template <class F>
    void operator()(int taille, F fct) noexcept
    {
        if(n<N){
            auto t = minuter(fct);
            out << duration_cast<milliseconds>(t).count() << "," << taille << "," << periode * taille << endl;
            n++;
        }else{
            out.close();
            throw TestSuccess{}; // die in hell
        }
    };
};

#if defined(TEST_ACCEL_SENSOR_ACQUISITION)
tester<1000> tester_accel_sensor{200, "test_accel_sensor_acquisition"};
#endif
#if defined(TEST_GPS_SENSOR_ACQUISITION)
tester<10> tester_gps_sensor{1, "test_gps_sensor_acquisition"};
#endif
#if defined(TEST_TAP_DATA_HANDLING)
tester<10> tester_tap_handling{1, "test_tap_data_handling"};
#endif
#if defined(TEST_ACCEL_DATA_HANDLING)
tester<1000> tester_accel_data_handling {200, "test_accel_data_handling"};
#endif
#if defined(TEST_GPS_DATA_HANDLING)
tester<10> tester_gps_data_handling {1, "test_gps_data_handling"};
#endif

#endif //SODIUM_TRACKER_TEST_UTILITY_H
