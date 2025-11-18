#pragma once
#include <cstdint>

namespace sim{

struct Criteria{
    const double pm25_threshold = 35.0;         //Air quality threshold in μg/m³
    const int pm25_mins_required = 120;         //How many mins above threshold to be flagged

    const double rush_speed_threshold = 25.0;   //Average rush hour speed threshold in kph
    const int required_rush_windows = 2;        //How many rush hour windows below threshold to be flagged

    const double noise_threshold = 70.0;        //Noise level threshold in db
    const int noise_mins_required = 30;         //How many mins above threshold to be flagged
};
}//namespace sim