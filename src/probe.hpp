#pragma once
#include <string>

// Tool-backed probes (wpctl / nmcli / hyprctl / bluetoothctl). These mirror the
// shell watchers they replace, producing the same fields and glyph icons, but
// from a single process on one interval instead of many polling scripts.
namespace amplitude::probe {

struct Audio {
    int volume = 0;
    bool muted = false;
    std::string icon = "󰝟";
};

struct Network {
    std::string status = "disabled"; // "enabled" | "disabled"
    std::string ssid;
    std::string icon = "󰤮";
    std::string eth_status = "Disconnected";
};

struct Bluetooth {
    bool present = false;
    std::string status = "off"; // "on" | "off"
    std::string connected = "Off";
    std::string icon = "󰂲";
};

std::string battery_icon(int percent, const std::string& status);

Audio audio();
Network network();
Bluetooth bluetooth();
std::string keyboard_layout(); // two-letter, upper-case (e.g. "US")

} // namespace amplitude::probe
