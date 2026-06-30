#include "metrics.hpp"

#include "sysfs.hpp"

#include <array>
#include <fstream>
#include <glob.h>
#include <sstream>
#include <string>

namespace amplitude {

void Collector::sample_cpu(Snapshot& s) {
    std::ifstream f("/proc/stat");
    std::string cpu;
    std::array<unsigned long long, 10> v{};
    if (!(f >> cpu))
        return;
    int n = 0;
    while (n < 10 && (f >> v[n]))
        ++n;

    unsigned long long idle = v[3] + v[4]; // idle + iowait
    unsigned long long total = 0;
    for (int i = 0; i < n; ++i)
        total += v[i];

    if (have_cpu_ && total > prev_total_) {
        unsigned long long dt = total - prev_total_;
        unsigned long long di = idle - prev_idle_;
        s.cpu_usage = 100.0 * static_cast<double>(dt - di) / static_cast<double>(dt);
    }
    prev_total_ = total;
    prev_idle_ = idle;
    have_cpu_ = true;
}

void Collector::sample_memory(Snapshot& s) {
    std::ifstream f("/proc/meminfo");
    std::string key;
    long long value;
    std::string unit;
    long long total = 0, avail = 0;
    while (f >> key >> value >> unit) {
        if (key == "MemTotal:")
            total = value;
        else if (key == "MemAvailable:") {
            avail = value;
            break;
        }
    }
    if (total > 0) {
        s.mem_total = total;
        s.mem_used = total - avail;
        s.mem_usage = 100.0 * static_cast<double>(s.mem_used) / static_cast<double>(total);
    }
}

void Collector::sample_network(Snapshot& s, double dt) {
    std::ifstream f("/proc/net/dev");
    std::string line;
    std::getline(f, line); // header
    std::getline(f, line); // header
    unsigned long long rx_total = 0, tx_total = 0;

    while (std::getline(f, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string iface = line.substr(0, colon);
        // trim leading spaces
        iface.erase(0, iface.find_first_not_of(" \t"));
        if (iface == "lo")
            continue; // loopback isn't real traffic

        std::istringstream ss(line.substr(colon + 1));
        std::array<unsigned long long, 16> c{};
        int n = 0;
        while (n < 16 && (ss >> c[n]))
            ++n;
        if (n >= 9) {
            rx_total += c[0]; // bytes received
            tx_total += c[8]; // bytes transmitted
        }
    }

    if (have_net_ && dt > 0.0) {
        s.net_rx = static_cast<double>(rx_total - prev_rx_) / dt;
        s.net_tx = static_cast<double>(tx_total - prev_tx_) / dt;
    }
    prev_rx_ = rx_total;
    prev_tx_ = tx_total;
    have_net_ = true;
}

void Collector::sample_battery(Snapshot& s) {
    glob_t g{};
    if (glob("/sys/class/power_supply/BAT*", GLOB_NOSORT, nullptr, &g) == 0 && g.gl_pathc > 0) {
        std::string base = g.gl_pathv[0];
        s.battery_present = true;
        if (auto cap = sysfs::read_ll(base + "/capacity"))
            s.battery_capacity = static_cast<int>(*cap);
        if (auto st = sysfs::read_line(base + "/status"); st && !st->empty())
            s.battery_status = *st;
    }
    globfree(&g);
}

Snapshot Collector::poll(double interval_seconds) {
    Snapshot s;
    sample_cpu(s);
    sample_memory(s);
    sample_network(s, interval_seconds);
    sample_battery(s);
    return s;
}

} // namespace amplitude
