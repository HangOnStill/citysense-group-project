#pragma once
#include <thread>
#include <vector>
#include <unordered_map>
#include <memory>

#include "../model/SensorRecord.hpp"
#include "../core/Aggregator.hpp"
#include "SpscQueue.hpp"

namespace parallel {

    struct PerZoneResult {
        int zone_id;
        core::Summary summary;
    };

    class ZoneIngestor {
    public:
        explicit ZoneIngestor(const std::vector<int>& zones) {
            for (int z : zones) {
                queues_.emplace(z, std::make_unique<SpscQueue<model::SensorRecord>>());
                aggregators_.emplace(z, std::make_unique<core::Aggregator>(0)); // 0 = no eviction
            }
        }

        void start_workers() {
            for (auto& [zone, qptr] : queues_) {
                workers_.emplace_back([this, zone, q = qptr.get()] {
                    auto& ag = *aggregators_.at(zone);
                    while (auto rec = q->pop()) {
                        ag.consume(*rec);
                    }
                    });
            }
        }

        void ingest(const model::SensorRecord& rec) {
            auto it = queues_.find(rec.zone_id);
            if (it != queues_.end()) {
                it->second->push(rec);
            }
        }

        std::vector<PerZoneResult> finish() {
            // close all queues
            for (auto& [_, qptr] : queues_) {
                qptr->close();
            }
            // join workers
            for (auto& t : workers_) {
                if (t.joinable()) t.join();
            }

            std::vector<PerZoneResult> out;
            out.reserve(aggregators_.size());
            for (auto& [zone, agptr] : aggregators_) {
                out.push_back(PerZoneResult{ zone, agptr->summary() });
            }
            return out;
        }

    private:
        std::unordered_map<int, std::unique_ptr<SpscQueue<model::SensorRecord>>> queues_;
        std::unordered_map<int, std::unique_ptr<core::Aggregator>>              aggregators_;
        std::vector<std::thread>                                                workers_;
    };

} // namespace parallel
