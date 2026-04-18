#include "wyag/index.hpp"

#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

std::uint16_t read_u16(const Bytes& bytes, std::size_t offset) {
    if (offset + 2 > bytes.size()) {
        throw std::runtime_error("Unexpected end of index file.");
    }

    return (static_cast<std::uint16_t>(bytes[offset]) << 8) |
           static_cast<std::uint16_t>(bytes[offset + 1]);
}

std::uint32_t read_u32(const Bytes& bytes, std::size_t offset) {
    if (offset + 4 > bytes.size()) {
        throw std::runtime_error("Unexpected end of index file.");
    }

    return (static_cast<std::uint32_t>(bytes[offset]) << 24) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
           static_cast<std::uint32_t>(bytes[offset + 3]);
}

void append_u16(Bytes& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

void append_u32(Bytes& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value >> 24));
    bytes.push_back(static_cast<std::uint8_t>(value >> 16));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

std::uint32_t to_u32(std::size_t value, std::string_view field_name) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("Index field is too large: " +
                                 std::string(field_name));
    }
    return static_cast<std::uint32_t>(value);
}

std::uint32_t to_u32(int value, std::string_view field_name) {
    if (value < 0) {
        throw std::runtime_error("Index field is negative: " +
                                 std::string(field_name));
    }
    return static_cast<std::uint32_t>(value);
}

std::uint8_t hex_digit(char c) {
    if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
    throw std::runtime_error("Invalid SHA-1 hex digit in index entry.");
}

Bytes hex_to_bytes(std::string_view hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Invalid SHA-1 hex length in index entry.");
    }

    Bytes bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>((hex_digit(hex[i]) << 4) |
                                                  hex_digit(hex[i + 1])));
    }
    return bytes;
}

std::string bytes_to_hex(const Bytes& bytes, std::size_t offset,
                         std::size_t n) {
    if (offset + n > bytes.size()) {
        throw std::runtime_error("Unexpected end of index file.");
    }

    static constexpr char HEX[] = "0123456789abcdef";
    std::string out;
    out.reserve(n * 2);
    for (std::size_t i = 0; i < n; ++i) {
        auto value = bytes[offset + i];
        out.push_back(HEX[value >> 4]);
        out.push_back(HEX[value & 0x0F]);
    }
    return out;
}

std::string read_name(const Bytes& bytes, std::size_t start, std::size_t end,
                      std::uint16_t name_length) {
    constexpr std::uint16_t kMaxInlineNameLength = 0x0FFF;

    if (name_length < kMaxInlineNameLength) {
        if (start + name_length >= end) {
            throw std::runtime_error("Malformed index entry name.");
        }
        if (bytes[start + name_length] != 0) {
            throw std::runtime_error(
                "Index entry name is not null-terminated.");
        }
        return std::string(
            bytes.begin() + static_cast<std::ptrdiff_t>(start),
            bytes.begin() + static_cast<std::ptrdiff_t>(start + name_length));
    }

    auto name_end =
        std::find(bytes.begin() + static_cast<std::ptrdiff_t>(start),
                  bytes.begin() + static_cast<std::ptrdiff_t>(end), 0);
    if (name_end == bytes.begin() + static_cast<std::ptrdiff_t>(end)) {
        throw std::runtime_error("Index entry name is not null-terminated.");
    }

    return std::string(bytes.begin() + static_cast<std::ptrdiff_t>(start),
                       name_end);
}

Index read_index(const GitRepository& repo) {
    auto filepath = repo_file(repo, {"index"});
    if (!filepath || !fs::exists(*filepath)) return {};

    auto bytes = read_file(*filepath);
    if (bytes.size() < 12 + 20) {
        throw std::runtime_error("Index file is too short.");
    }

    std::string signature(bytes.begin(), bytes.begin() + 4);
    auto version = read_u32(bytes, 4);
    auto count = read_u32(bytes, 8);

    if (signature != "DIRC") {
        throw std::runtime_error("Index file is not DirCache (DIRC).");
    }
    if (version != 2) {
        throw std::runtime_error("Unsupported index version: " +
                                 std::to_string(version));
    }

    Index index{};
    index.version = static_cast<int>(version);
    index.entries.reserve(count);

    std::size_t curr = 12;
    const std::size_t checksum_offset = bytes.size() - 20;

    for (std::uint32_t i = 0; i < count; ++i) {
        const std::size_t entry_start = curr;
        if (entry_start + 62 > checksum_offset) {
            throw std::runtime_error("Index entry extends past checksum.");
        }

        IndexEntry entry{};
        entry.change_metadata.seconds = static_cast<int>(read_u32(bytes, curr));
        entry.change_metadata.nanoseconds =
            static_cast<int>(read_u32(bytes, curr + 4));
        entry.change_data.seconds = static_cast<int>(read_u32(bytes, curr + 8));
        entry.change_data.nanoseconds =
            static_cast<int>(read_u32(bytes, curr + 12));
        entry.device_id = static_cast<int>(read_u32(bytes, curr + 16));
        entry.inode = static_cast<int>(read_u32(bytes, curr + 20));

        const auto mode = read_u32(bytes, curr + 24);
        entry.mode_type = static_cast<int>((mode >> 12) & 0x000F);
        entry.mode_perms = static_cast<int>(mode & 0x01FF);

        entry.uid = static_cast<int>(read_u32(bytes, curr + 28));
        entry.gid = static_cast<int>(read_u32(bytes, curr + 32));
        entry.object_size = read_u32(bytes, curr + 36);
        entry.sha = bytes_to_hex(bytes, curr + 40, 20);

        const auto flags = read_u16(bytes, curr + 60);
        entry.assume_valid = (flags & 0x8000U) != 0;
        const bool flag_extended = (flags & 0x4000U) != 0;
        if (flag_extended) {
            throw std::runtime_error("Extended index flags are not supported.");
        }
        entry.flag_stage = static_cast<int>((flags & 0x3000U) >> 12);
        const auto name_length = static_cast<std::uint16_t>(flags & 0x0FFFU);

        const std::size_t name_start = curr + 62;
        entry.object_name = fs::path{
            read_name(bytes, name_start, checksum_offset, name_length)};

        auto name_end = std::find(
            bytes.begin() + static_cast<std::ptrdiff_t>(name_start),
            bytes.begin() + static_cast<std::ptrdiff_t>(checksum_offset), 0);
        if (name_end ==
            bytes.begin() + static_cast<std::ptrdiff_t>(checksum_offset)) {
            throw std::runtime_error(
                "Index entry name is not null-terminated.");
        }

        curr = static_cast<std::size_t>(std::distance(bytes.begin(), name_end) +
                                        1);

        while ((curr - entry_start) % 8 != 0) {
            if (curr >= checksum_offset) {
                throw std::runtime_error(
                    "Index entry padding overruns checksum.");
            }
            ++curr;
        }

        index.entries.push_back(std::move(entry));
    }

    return index;
}

void write_index(const GitRepository& repo, const Index& index) {
    auto filepath = repo_file(repo, {"index"}, true);
    if (!filepath) throw std::runtime_error("Could not create index file.");

    Bytes bytes;

    bytes.insert(bytes.end(), {'D', 'I', 'R', 'C'});
    append_u32(bytes, to_u32(index.version, "version"));
    append_u32(bytes, to_u32(index.entries.size(), "entry count"));

    for (const auto& entry : index.entries) {
        const std::size_t entry_start = bytes.size();

        append_u32(bytes,
                   to_u32(entry.change_metadata.seconds, "ctime seconds"));
        append_u32(bytes, to_u32(entry.change_metadata.nanoseconds,
                                 "ctime nanoseconds"));
        append_u32(bytes, to_u32(entry.change_data.seconds, "mtime seconds"));
        append_u32(bytes,
                   to_u32(entry.change_data.nanoseconds, "mtime nanoseconds"));
        append_u32(bytes, to_u32(entry.device_id, "device id"));
        append_u32(bytes, to_u32(entry.inode, "inode"));

        const auto mode_type = to_u32(entry.mode_type, "mode type");
        const auto mode_perms = to_u32(entry.mode_perms, "mode permissions");
        append_u32(bytes, (mode_type << 12) | mode_perms);

        append_u32(bytes, to_u32(entry.uid, "uid"));
        append_u32(bytes, to_u32(entry.gid, "gid"));
        append_u32(bytes, to_u32(entry.object_size, "object size"));

        const Bytes sha = hex_to_bytes(entry.sha);
        if (sha.size() != 20) {
            throw std::runtime_error("Index entry SHA-1 must be 20 bytes.");
        }
        bytes.insert(bytes.end(), sha.begin(), sha.end());

        const std::string name = entry.object_name.generic_string();
        const auto name_length = static_cast<std::uint16_t>(
            std::min<std::size_t>(name.size(), 0x0FFF));
        const auto assume_valid =
            entry.assume_valid ? static_cast<std::uint16_t>(0x8000U) : 0U;
        const auto stage = to_u32(entry.flag_stage, "flag stage");
        if (stage > 3) {
            throw std::runtime_error("Index entry stage must be in [0, 3].");
        }
        append_u16(bytes, static_cast<std::uint16_t>(
                              assume_valid | (stage << 12) | name_length));

        bytes.insert(bytes.end(), name.begin(), name.end());
        bytes.push_back(0);

        while ((bytes.size() - entry_start) % 8 != 0) {
            bytes.push_back(0);
        }
    }

    std::array<std::uint8_t, SHA_DIGEST_LENGTH> digest{};
    SHA1(bytes.data(), bytes.size(), digest.data());
    bytes.insert(bytes.end(), digest.begin(), digest.end());

    write_file(*filepath, bytes);
}
