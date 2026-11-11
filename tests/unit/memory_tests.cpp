#include "linxray/memory.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    // Unlike assert(), this check also runs in Release builds.
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

linxray::MemoryInfo parse(std::string_view contents) {
    std::istringstream input{std::string(contents)};
    return linxray::parse_meminfo(input);
}

void expect_invalid(std::string_view name, std::string_view contents,
                    std::string_view expected_message) {
    try {
        static_cast<void>(parse(contents));
    } catch (const std::runtime_error& error) {
        require(std::string_view(error.what()).find(expected_message) != std::string_view::npos,
                std::string(name) + ": unexpected error: " + error.what());
        return;
    }
    throw std::runtime_error(std::string(name) + ": invalid input was accepted");
}

void test_valid_input() {
    const auto memory = parse(
        "HugePages_Total: 0\r\n"
        "MemAvailable:\t6291456 kB  \r\n"
        "\n"
        "MemFree: 1048576 kB\n"
        "  MemTotal:    8388608 kB\n"
        "UnknownFutureField: 42\n");
    require(memory.total_kib == 8388608, "total RAM must be parsed in KiB");
    require(memory.available_kib == 6291456, "available RAM must come from MemAvailable");
    require(memory.used_kib() == 2097152, "used RAM must exclude reclaimable memory");
    require(std::abs(memory.usage_percent() - 25.0) < 0.0001, "usage must be 25 percent");

    const auto full = parse("MemTotal: 1024 kB\nMemAvailable: 0 kB");
    require(full.used_kib() == 1024 && full.usage_percent() == 100.0,
            "zero available RAM and a final line without newline must work");

    const auto empty = parse("MemTotal: 1024 kB\nMemAvailable: 1024 kB\n");
    require(empty.used_kib() == 0 && empty.usage_percent() == 0.0,
            "fully available RAM must have zero usage");

    const auto large = parse(
        "MemTotal: 18446744073709551615 kB\nMemAvailable: 0 kB\n");
    require(large.total_kib == std::numeric_limits<std::uint64_t>::max(),
            "values must not be truncated to 32 bits");
    require(std::isfinite(large.usage_percent()), "large values must produce finite usage");
}

void test_invalid_input() {
    expect_invalid("empty input", "", "missing MemTotal");
    expect_invalid("missing total", "MemAvailable: 1 kB\n", "missing MemTotal");
    expect_invalid("missing available", "MemTotal: 1024 kB\n", "missing MemAvailable");
    expect_invalid("zero total", "MemTotal: 0 kB\nMemAvailable: 0 kB\n", "greater than zero");
    expect_invalid("available exceeds total",
                   "MemTotal: 1024 kB\nMemAvailable: 2048 kB\n", "cannot exceed");
    expect_invalid("negative value", "MemTotal: -1 kB\n", "invalid numeric");
    expect_invalid("explicit positive sign", "MemTotal: +1 kB\n", "invalid numeric");
    expect_invalid("text value", "MemTotal: lots kB\n", "invalid numeric");
    expect_invalid("fractional value", "MemTotal: 1.5 kB\n", "invalid numeric");
    expect_invalid("overflow", "MemTotal: 18446744073709551616 kB\n", "invalid numeric");
    expect_invalid("wrong unit", "MemTotal: 1 MB\n", "expected an integer followed by kB");
    expect_invalid("missing unit", "MemTotal: 1\n", "expected an integer followed by kB");
    expect_invalid("trailing garbage", "MemTotal: 1 kB extra\n", "expected an integer");
    expect_invalid("duplicate total", "MemTotal: 1 kB\nMemTotal: 2 kB\n", "duplicate");
    expect_invalid("duplicate available",
                   "MemAvailable: 0 kB\nMemAvailable: 1 kB\n", "duplicate");
    expect_invalid("negative available",
                   "MemTotal: 1024 kB\nMemAvailable: -1 kB\n", "invalid numeric");
}

void test_read_failure() {
    std::istringstream input;
    input.setstate(std::ios::badbit);
    try {
        static_cast<void>(linxray::parse_meminfo(input));
    } catch (const std::runtime_error& error) {
        require(std::string_view(error.what()).find("failed to read") != std::string_view::npos,
                "a stream read error must be reported as an I/O failure");
        return;
    }
    throw std::runtime_error("a failed input stream was accepted");
}

}  // namespace

int main() {
    try {
        test_valid_input();
        test_invalid_input();
        test_read_failure();
        std::cout << "All memory parser tests passed.\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
