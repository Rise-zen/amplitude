#include "probe.hpp"

#include <array>
#include <cstdio>
#include <memory>
#include <sstream>

namespace amplitude::probe {

namespace {

struct PipeCloser {
    void operator()(FILE* f) const {
        if (f)
            pclose(f);
    }
};

// Run a command, capture stdout. Empty string on failure. Commands that may
// hang (bluetoothctl) are wrapped in `timeout` by the caller.
std::string run(const char* cmd) {
    std::unique_ptr<FILE, PipeCloser> pipe(popen(cmd, "r"));
    if (!pipe)
        return {};
    std::string out;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), buf.size(), pipe.get()))
        out += buf.data();
    return out;
}

std::string trim(std::string s) {
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return {};
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool contains(const std::string& hay, const char* needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

std::string battery_icon(int p, const std::string& status) {
    if (status == "Charging" || status == "Full") {
        if (p >= 90) return "󰂅";
        if (p >= 80) return "󰂋";
        if (p >= 60) return "󰂊";
        if (p >= 40) return "󰢞";
        if (p >= 20) return "󰂆";
        return "󰢜";
    }
    if (p >= 90) return "󰁹";
    if (p >= 80) return "󰂂";
    if (p >= 70) return "󰂁";
    if (p >= 60) return "󰂀";
    if (p >= 50) return "󰁿";
    if (p >= 40) return "󰁾";
    if (p >= 30) return "󰁽";
    if (p >= 20) return "󰁼";
    if (p >= 10) return "󰁻";
    return "󰁺";
}

Audio audio() {
    Audio a;
    // `wpctl get-volume @DEFAULT_AUDIO_SINK@` -> "Volume: 0.55 [MUTED]"
    std::string out = run("LC_ALL=C wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null");
    if (!out.empty()) {
        double vol = 0.0;
        std::istringstream ss(out);
        std::string label;
        ss >> label >> vol; // "Volume:" 0.55
        a.volume = static_cast<int>(vol * 100.0 + 0.5);
        a.muted = contains(out, "MUTED");
    }
    if (a.muted || a.volume == 0)
        a.icon = "󰝟";
    else if (a.volume >= 70)
        a.icon = "󰕾";
    else if (a.volume >= 30)
        a.icon = "󰖀";
    else
        a.icon = "󰕿";
    return a;
}

Network network() {
    Network n;
    std::string iface = trim(run("ip route show default 2>/dev/null | awk '/default/ {print $5; exit}'"));
    std::string type;
    if (!iface.empty()) {
        std::string cmd =
            "LC_ALL=C nmcli -t -f DEVICE,TYPE d 2>/dev/null | awk -F: -v dev='" + iface +
            "' '$1==dev {print $2; exit}'";
        type = trim(run(cmd.c_str()));
    }

    if (type == "ethernet") {
        n.status = "enabled";
        n.ssid = "Ethernet";
        n.icon = "󰈀";
        n.eth_status = "Connected";
    } else if (type == "wifi") {
        n.status = "enabled";
        n.ssid = trim(run("LC_ALL=C iw dev 2>/dev/null | awk '/\\s+ssid/ {$1=\"\"; sub(/^ /,\"\"); print; exit}'"));
        int signal = 0;
        try {
            signal = std::stoi(trim(run(
                "awk 'NR==3 {gsub(/\\./,\"\",$3); print int($3*100/70)}' /proc/net/wireless 2>/dev/null")));
        } catch (...) {
        }
        if (signal >= 75) n.icon = "󰤨";
        else if (signal >= 50) n.icon = "󰤥";
        else if (signal >= 25) n.icon = "󰤢";
        else n.icon = "󰤟";
        if (!run("LC_ALL=C nmcli -t -f DEVICE,TYPE,STATE d 2>/dev/null | awk -F: '$2==\"ethernet\" && "
                 "$3==\"connected\" && $1!=\"lo\" {print $1; exit}'")
                 .empty())
            n.eth_status = "Connected";
    } else {
        std::string radio = trim(run("LC_ALL=C nmcli radio wifi 2>/dev/null"));
        std::string wifi_dev = trim(run(
            "LC_ALL=C nmcli -t -f DEVICE,TYPE d 2>/dev/null | awk -F: '$2==\"wifi\" {print $1; exit}'"));
        if (wifi_dev.empty()) {
            n.status = "disabled";
            n.icon = "󰈂";
        } else if (radio == "disabled") {
            n.status = "disabled";
            n.icon = "󰤮";
        } else {
            n.status = "enabled";
            n.icon = "󰤯";
        }
    }
    return n;
}

Bluetooth bluetooth() {
    Bluetooth b;
    if (!contains(run("LC_ALL=C timeout 0.5 bluetoothctl list 2>/dev/null"), "Controller"))
        return b; // no adapter
    b.present = true;

    bool on = contains(run("LC_ALL=C timeout 0.5 bluetoothctl show 2>/dev/null"), "Powered: yes");
    b.status = on ? "on" : "off";
    if (!on) {
        b.connected = "Off";
        b.icon = "󰂲";
        return b;
    }

    std::string connected = run("LC_ALL=C timeout 0.5 bluetoothctl devices Connected 2>/dev/null");
    if (contains(connected, "Device")) {
        b.icon = "󰂱";
        // first line: "Device AA:BB:.. Name..." -> take the name (3rd field on)
        std::string first = connected.substr(0, connected.find('\n'));
        std::istringstream ss(first);
        std::string dev, mac, name, rest;
        ss >> dev >> mac;
        std::getline(ss, rest);
        b.connected = trim(rest);
        if (b.connected.empty())
            b.connected = "Connected";
    } else {
        b.icon = "󰂯";
        b.connected = "Disconnected";
    }
    return b;
}

std::string keyboard_layout() {
    std::string l = trim(run(
        "LC_ALL=C hyprctl devices -j 2>/dev/null | jq -r "
        "'(.keyboards[] | select(.main==true) | .active_keymap) // .keyboards[0].active_keymap // empty'"));
    if (l.empty() || l == "null")
        l = "US";
    l = l.substr(0, 2);
    for (char& c : l)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return l;
}

} // namespace amplitude::probe
