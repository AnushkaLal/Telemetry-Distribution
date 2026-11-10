#include "telemetry.hpp"
#include "structures.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>

SimulationStats runSimulation(const std::vector<Telemetry>& records,
                              const SimulationConfig& cfg,
                              bool showProgress) {
    SimulationStats stats;
    const std::size_t target = cfg.targetRecords;
    const std::size_t workers = std::max<std::size_t>(1, cfg.workerCount);
    BoundedQueue<Telemetry> queue(cfg.queueCapacity);
    std::atomic<std::size_t> done{0};
    stats.workerEvents.assign(workers, 0);
    std::vector<std::thread> pool;
    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < workers; ++i) {
        if (static_cast<int>(i) == cfg.offlineWorker) continue;
        pool.emplace_back([&, i] {
            Telemetry item;
            while (queue.pop(item)) {
                int delay = cfg.workerDelayMs;
                if (static_cast<int>(i) == cfg.slowWorker) delay += cfg.slowWorkerDelayMs;
                if (delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                ++stats.workerEvents[i];
                ++done;
            }
        });
    }

    for (std::size_t i = 0; i < target; ++i) {
        const Telemetry& item = records[i % records.size()];
        if (!queue.push(item)) break;
        ++stats.submitted;
        if (cfg.throttleDelayMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(cfg.throttleDelayMs));
        if (showProgress && (i % std::max<std::size_t>(1, target / 20) == 0 || i + 1 == target)) {
            int pct = static_cast<int>((100.0 * (i + 1)) / target);
            std::cout << "\r  Ingesting telemetry  [" << std::string(pct / 5, '#')
                      << std::string(20 - pct / 5, ' ') << "] " << pct << "%" << std::flush;
        }
    }
    queue.close();
    for (auto& t : pool) if (t.joinable()) t.join();
    auto end = std::chrono::steady_clock::now();
    if (showProgress) std::cout << "\n";
    stats.processed = done.load();
    stats.peakQueue = queue.peakOccupancy();
    stats.blockedPushes = queue.blockedPushes();
    stats.seconds = std::chrono::duration<double>(end - start).count();
    stats.throughput = stats.seconds > 0 ? stats.processed / stats.seconds : 0;
    return stats;
}
