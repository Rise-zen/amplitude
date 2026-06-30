#pragma once
#include <optional>
#include <string>

// Small helpers for reading Linux /proc and /sys text nodes.
namespace amplitude::sysfs {

// Read an entire file into a string. nullopt if it can't be opened.
std::optional<std::string> read(const std::string& path);

// Read the first line of a file, trimmed of trailing newline. nullopt on error.
std::optional<std::string> read_line(const std::string& path);

// Read a file as an integer. nullopt if missing or unparseable.
std::optional<long long> read_ll(const std::string& path);

// True if any path matching the glob exists (e.g. "/sys/class/power_supply/BAT*").
bool glob_exists(const std::string& pattern);

} // namespace amplitude::sysfs
