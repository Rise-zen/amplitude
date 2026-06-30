#include "sysfs.hpp"

#include <fstream>
#include <glob.h>
#include <sstream>

namespace amplitude::sysfs {

std::optional<std::string> read(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::optional<std::string> read_line(const std::string& path) {
    std::ifstream f(path);
    if (!f)
        return std::nullopt;
    std::string line;
    std::getline(f, line);
    return line;
}

std::optional<long long> read_ll(const std::string& path) {
    auto line = read_line(path);
    if (!line || line->empty())
        return std::nullopt;
    try {
        return std::stoll(*line);
    } catch (...) {
        return std::nullopt;
    }
}

bool glob_exists(const std::string& pattern) {
    glob_t g{};
    bool found = glob(pattern.c_str(), GLOB_NOSORT, nullptr, &g) == 0 && g.gl_pathc > 0;
    globfree(&g);
    return found;
}

} // namespace amplitude::sysfs
