#include "wyag/tree.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using Bytes = std::vector<std::uint8_t>;
namespace fs = std::filesystem;

// Tree write invariant: records are sorted by pathname, with directories
// compared as though they had a trailing slash.
bool record_comparator(const TreeLeafRecord& u, const TreeLeafRecord& v) {
    const bool u_is_dir = !u.mode.empty() && u.mode[0] == '4';
    const bool v_is_dir = !v.mode.empty() && v.mode[0] == '4';
    std::string u_path{u.path.string() + (u_is_dir ? "/" : "")};
    std::string v_path{v.path.string() + (v_is_dir ? "/" : "")};
    return u_path < v_path;
}

std::vector<TreeLeafRecord> parse_treeleaf(std::span<const std::uint8_t> raw) {
    auto start_it = raw.begin();
    auto end_it = raw.end();

    std::vector<TreeLeafRecord> records{};
    while (start_it < end_it) {
        // Get mode
        auto space_it = std::find(start_it, end_it, ' ');
        if (space_it == end_it)
            throw std::runtime_error("Malformed tree record (missing space).");
        std::vector<std::uint8_t> mode{start_it, space_it};

        // Get path
        auto path_start_it = std::next(space_it);
        auto path_end_it = std::find(path_start_it, end_it, 0);
        if (path_end_it == end_it)
            throw std::runtime_error("Malformed tree record (missing null).");
        fs::path path{std::string{path_start_it, path_end_it}};

        // Get sha
        auto sha_start_it = std::next(path_end_it);
        if (end_it - sha_start_it < 20)
            throw std::runtime_error("Malformed tree record");
        auto sha_end_it = sha_start_it + 20;
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (auto itr = sha_start_it; itr < sha_end_it; itr++) {
            oss << std::setw(2) << static_cast<int>(*itr);
        }
        records.emplace_back(std::move(mode), std::move(path), oss.str());
        start_it = sha_end_it;
    }
    return records;
}

Bytes serialize_tree_records(const std::vector<TreeLeafRecord>& records) {
    std::vector<TreeLeafRecord> sorted_records{records};
    std::sort(sorted_records.begin(), sorted_records.end(), record_comparator);

    Bytes result{};
    for (const auto& rec : sorted_records) {
        result.insert(result.end(), rec.mode.begin(), rec.mode.end());
        result.push_back(' ');

        std::string path_str{rec.path.string()};
        result.insert(result.end(), path_str.begin(), path_str.end());
        result.push_back(0);

        auto hex_value = [](char c) -> std::uint8_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            throw std::runtime_error("Invalid hex digit in SHA.");
        };

        if (rec.sha.size() != 40) {
            throw std::runtime_error("Tree SHA must be 40 hex characters.");
        }

        auto sha_hex{rec.sha};
        for (std::size_t i = 0; i < sha_hex.size(); i += 2) {
            std::uint8_t hi{hex_value(sha_hex[i])};
            std::uint8_t lo{hex_value(sha_hex[i + 1])};
            result.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
        }
    }
    return result;
}
