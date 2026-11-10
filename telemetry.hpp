#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP
#include <string>
#include <vector>
#include <cstddef>

struct Telemetry {
    std::string timestamp, satellite, sensor;
    double value{};
};

std::vector<Telemetry> loadTelemetry(const std::string& filename);

struct SimulationConfig {
    std::size_t targetRecords{10000};
    std::size_t workerCount{4};
    std::size_t queueCapacity{256};
    int workerDelayMs{0};
    int slowWorker{-1};
    int slowWorkerDelayMs{0};
    int throttleDelayMs{0};
    int offlineWorker{-1};
};

struct SimulationStats {
    std::size_t submitted{0}, processed{0}, peakQueue{0}, blockedPushes{0};
    double seconds{0.0}, throughput{0.0};
    std::vector<std::size_t> workerEvents;
};

SimulationStats runSimulation(const std::vector<Telemetry>& records,
                              const SimulationConfig& config,
                              bool showProgress = true);
#endif
