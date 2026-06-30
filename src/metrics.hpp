#pragma once
#include <cstdint>
#include <string>

namespace amplitude {

struct Snapshot {
    // CPU
    double cpu_usage = 0.0; // 0..100, busy % since the previous sample

    // Memory (KiB)
    long long mem_total = 0;
    long long mem_used = 0;
    double mem_usage = 0.0; // 0..100

    // Network throughput (bytes/sec since the previous sample)
    double net_rx = 0.0;
    double net_tx = 0.0;

    // Battery
    bool battery_present = false;
    int battery_capacity = 0; // 0..100
    std::string battery_status = "Unknown";
};

// Stateful collector: keeps the previous CPU/net counters so each poll can
// report a rate rather than a monotonic total.
class Collector {
public:
    Snapshot poll(double interval_seconds);

private:
    void sample_cpu(Snapshot& s);
    void sample_memory(Snapshot& s);
    void sample_network(Snapshot& s, double dt);
    void sample_battery(Snapshot& s);

    // CPU jiffies from the previous /proc/stat read.
    unsigned long long prev_total_ = 0;
    unsigned long long prev_idle_ = 0;
    bool have_cpu_ = false;

    // Aggregate rx/tx bytes from the previous /proc/net/dev read.
    unsigned long long prev_rx_ = 0;
    unsigned long long prev_tx_ = 0;
    bool have_net_ = false;
};

} // namespace amplitude
