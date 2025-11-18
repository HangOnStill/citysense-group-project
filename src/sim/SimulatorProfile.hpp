#pragma once

namespace sim{

    //Will be used by the program to identify patterns based on whether it is a weekday
    //or weekend

    //May be edited in future to add Special Events, Snow Days, etc, But for simplicity it is 
    //currently only weekday vs weekend.
enum class SimulatorProfile{
    Weekday,
    Weekend
};

} //namespace sim