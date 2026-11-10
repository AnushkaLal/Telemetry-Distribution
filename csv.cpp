#include "telemetry.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<Telemetry> loadTelemetry(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open dataset: " + filename);
    std::vector<Telemetry> rows;
    std::string line;
    std::getline(file, line); // header
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string timestamp, satellite, sensor, value;
        if (!std::getline(ss, timestamp, ',') || !std::getline(ss, satellite, ',') ||
            !std::getline(ss, sensor, ',') || !std::getline(ss, value, ',')) continue;
        try { rows.push_back({timestamp, satellite, sensor, std::stod(value)}); }
        catch (...) { /* Skip malformed rows; never use the dataset's anomaly label. */ }
    }
    if (rows.empty()) throw std::runtime_error("Dataset contains no readable telemetry rows.");
    return rows;
}
