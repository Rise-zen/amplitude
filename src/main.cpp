#include "metrics.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_running{true};
void on_signal(int) { g_running = false; }

std::string round1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\t': out += "\\t"; break;
        default: out += c;
        }
    }
    return out;
}

std::string serialize(const amplitude::Snapshot& s) {
    std::string j = "{";
    j += "\"cpu\":" + round1(s.cpu_usage) + ",";
    j += "\"memUsed\":" + std::to_string(s.mem_used) + ",";
    j += "\"memTotal\":" + std::to_string(s.mem_total) + ",";
    j += "\"mem\":" + round1(s.mem_usage) + ",";
    j += "\"netRx\":" + std::to_string(static_cast<long long>(std::llround(s.net_rx))) + ",";
    j += "\"netTx\":" + std::to_string(static_cast<long long>(std::llround(s.net_tx))) + ",";
    j += "\"batteryPresent\":" + std::string(s.battery_present ? "true" : "false") + ",";
    j += "\"battery\":" + std::to_string(s.battery_capacity) + ",";
    j += "\"batteryStatus\":\"" + json_escape(s.battery_status) + "\"";
    j += "}";
    return j;
}

// Write atomically: a partial read can never see a half-written file, because
// the reader either sees the old inode or the fully-renamed new one.
bool write_atomic(const std::string& path, const std::string& data) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc | std::ios::binary);
        if (!f)
            return false;
        f << data;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

} // namespace

int main(int argc, char** argv) {
    std::string out = "/tmp/amplitude.json";
    long interval_ms = 1000;
    bool once = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--out" && i + 1 < argc)
            out = argv[++i];
        else if (a == "--interval" && i + 1 < argc)
            interval_ms = std::strtol(argv[++i], nullptr, 10);
        else if (a == "--once")
            once = true;
        else if (a == "-h" || a == "--help") {
            std::printf("usage: amplitude [--out PATH] [--interval MS] [--once]\n");
            return 0;
        }
    }
    if (interval_ms < 100)
        interval_ms = 100;

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    amplitude::Collector collector;
    const double dt = static_cast<double>(interval_ms) / 1000.0;

    // First poll seeds the CPU/net deltas; its rates read 0, which is correct.
    collector.poll(dt);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        auto snap = collector.poll(dt);
        if (!write_atomic(out, serialize(snap)))
            std::fprintf(stderr, "[amplitude] failed to write %s\n", out.c_str());
        if (once)
            break;
    }
    return 0;
}
