#include "linxray/memory.hpp"

#include <charconv>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace linxray {

std::uint64_t MemoryInfo::used_kib() const noexcept {
    return total_kib - available_kib;
}

double MemoryInfo::usage_percent() const noexcept {
    if (total_kib == 0) {
        return 0.0;
    }
    return 100.0 * (static_cast<double>(used_kib()) / static_cast<double>(total_kib));
}

MemoryInfo parse_meminfo(std::istream& input) {
    std::optional<std::uint64_t> total;
    std::optional<std::uint64_t> available;
    std::string line;

    while (std::getline(input, line)) {
        std::istringstream fields(line);
        std::string key;
        fields >> key;

        // Some unrelated fields, such as HugePages_Total, have no unit.
        if (key != "MemTotal:" && key != "MemAvailable:") {
            continue;
        }

        std::string number;
        std::string unit;
        std::string extra;
        if (!(fields >> number >> unit) || unit != "kB" || (fields >> extra)) {
            throw std::runtime_error("invalid " + key + " expected an integer followed by kB");
        }

        // from_chars rejects negative numbers and reports integer overflow.
        std::uint64_t value{};
        const auto [end, error] = std::from_chars(
            number.data(), number.data() + number.size(), value);
        if (error != std::errc{} || end != number.data() + number.size()) {
            throw std::runtime_error("invalid numeric value for " + key);
        }

        auto& destination = (key == "MemTotal:") ? total : available;
        if (destination.has_value()) {
            throw std::runtime_error("duplicate field " + key);
        }
        destination = value;
    }

    if (input.bad() || (input.fail() && !input.eof())) {
        throw std::runtime_error("failed to read meminfo data");
    }
    if (!total.has_value()) {
        throw std::runtime_error("missing MemTotal field");
    }
    if (!available.has_value()) {
        throw std::runtime_error("missing MemAvailable field");
    }
    if (*total == 0) {
        throw std::runtime_error("MemTotal must be greater than zero");
    }
    if (*available > *total) {
        throw std::runtime_error("MemAvailable cannot exceed MemTotal");
    }

    return MemoryInfo{*total, *available};
}

MemoryInfo read_memory(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("cannot open meminfo file: " + path.string());
    }
    return parse_meminfo(input);
}

}  // namespace linxray
