#pragma once

#include <vector>
#include "../model/SensorRecord.hpp"
#include "Criteria.hpp"

namespace sim{
    struct DailyFlags{
        bool air_flag = false;
        bool noise_flag = false;
        bool traffic_flag = false;
    };

    struct MonthlySummary{
        int month_num = 0;
        int air_days = 0;
        int noise_days = 0;
        int traffic_days = 0;
    };

    struct YearlySummary{
        std::vector<MonthlySummary> months;
    };

    DailyFlags analyse_day(const std::vector<model::SensorRecord>& day, const Criteria& criteria);

    MonthlySummary analyse_month(const std::vector<model::SensorRecord>& month, const Criteria& criteria);

    YearlySummary analyse_year(const std::vector<model::SensorRecord>& year, const Criteria& criteria);

    void print_month_summary(const sim::MonthlySummary& ms);

    void print_year_summary(const sim::YearlySummary& ys);

}//namespace sim