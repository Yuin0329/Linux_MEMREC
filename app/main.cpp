#include "linxray/memory.hpp"

#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void print_help() {
    std::cout
        << "linxray - Linux system diagnostics\n\n"
        << "Usage:\n"
        << "  linxray memory [--meminfo <path>]\n"
        << "  linxray --help\n\n"
        << "Commands:\n"
        << "  memory     Show total, available, and used RAM in GiB\n\n"
        << "Options:\n"
        << "  --meminfo <path>  Read a saved meminfo file (default: /proc/meminfo)\n"
        << "  -h, --help        Show this help\n\n"
        << "Used RAM = MemTotal - MemAvailable. No root privileges required.\n";
}

int usage_error(std::string_view message) {
    std::cerr << "linxray: " << message << "\nTry 'linxray --help' for usage.\n";
    return 2;
}

void print_memory(const linxray::MemoryInfo& memory) {
    constexpr double kib_per_gib = 1024.0 * 1024.0;
    std::cout << std::fixed << std::setprecision(2) << std::left
              << std::setw(12) << "Total:"
              << static_cast<double>(memory.total_kib) / kib_per_gib << " GiB\n"
              << std::setw(12) << "Available:"
              << static_cast<double>(memory.available_kib) / kib_per_gib << " GiB\n"
              << std::setw(12) << "Used:"
              << static_cast<double>(memory.used_kib()) / kib_per_gib << " GiB\n"
              << std::setw(12) << "Usage:"
              << std::setprecision(1) << memory.usage_percent() << "%\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc == 1 || std::string_view(argv[1]) == "--help"
                  || std::string_view(argv[1]) == "-h") {
        print_help();
        return 0;
    }
    if (std::string_view(argv[1]) != "memory") {
        return usage_error("unknown command: " + std::string(argv[1]));
    }

    std::filesystem::path meminfo_path = "/proc/meminfo";
    bool custom_source = false;
    for (int index = 2; index < argc; ++index) {
        const std::string_view option(argv[index]);
        if (option == "--help" || option == "-h") {
            print_help();
            return 0;
        }
        if (option != "--meminfo") {
            return usage_error("unknown option: " + std::string(option));
        }
        if (custom_source) {
            return usage_error("--meminfo may only be provided once");
        }
        if (index + 1 >= argc || std::string_view(argv[index + 1]).empty()
            || std::string_view(argv[index + 1]).starts_with('-')) {
            return usage_error("--meminfo requires a file path");
        }
        meminfo_path = argv[++index];
        custom_source = true;
    }

    try {
        print_memory(linxray::read_memory(meminfo_path));
    } catch (const std::exception& error) {
        std::cerr << "linxray: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
