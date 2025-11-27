#pragma once
#include <vector>
#include <mutex>
#include "Window.hpp"

namespace core {

    struct Incident {
        std::string incident_id;
        std::string message;
        model::SensorRecord instance;
    };

    struct Report {

        const std::vector<Incident>& get_incidents() const {
            return incidents;
        }
        void record_incident(const Incident& incident) {
            std::scoped_lock lock(mutex_);
            incidents.push_back(incident);
        }
        void clear_incidents() {
            incidents.clear();
        }

    private:
        std::vector<Incident> incidents;
        mutable std::mutex mutex_;
    }
}