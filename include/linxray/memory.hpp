#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>

namespace linxray {

// Linux labels these values "kB", but each unit represents 1024 bytes (KiB).
struct MemoryInfo {
    std::uint64_t total_kib{};
    std::uint64_t available_kib{};

    [[nodiscard]] std::uint64_t used_kib() const noexcept;
    [[nodiscard]] double usage_percent() const noexcept;
};

// Parsing is separate from file I/O so tests can supply an in-memory stream.
// Throws std::runtime_error for unreadable, incomplete, or invalid data.
[[nodiscard]] MemoryInfo parse_meminfo(std::istream& input);
[[nodiscard]] MemoryInfo read_memory(
    const std::filesystem::path& path = "/proc/meminfo");

}  // namespace linxray
