#pragma once

#include <random>
#include <cstdint>

namespace sim{

class SeededRNG{
    public:
        // Default seed = 12345, can call with a custom seed using: 
        //      sim::SeededRNG rng(my_seed); where my_seed is an int
        // You can then call:
        //      double traffic_speed = rng.uniform(15.0, 35.0); 
        // which will return the same random number deterministically based on seed.
        explicit SeededRNG(int seed = 12345) : engine_(static_cast<std::uint64_t>(seed)) {}

        //When called with Doubles
        double uniform(double min, double max){
            std::uniform_real_distribution<double> dist(min, max);
            return dist(engine_);
        }

        //When called with Ints
        int uniform(int min, int max){
            std::uniform_int_distribution<int> dist(min, max);
            return dist(engine_);
        }
    private:
        // This is a 64-bit Mersenne Twister pseudo-random generator, which basically means
        // deterministic random generator
        std::mt19937_64 engine_;
};
}//namespace sim