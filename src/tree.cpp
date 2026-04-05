#include "wyag/tree.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

using Bytes = std::vector<std::uint8_t>;
namespace fs = std::filesystem;

// Tree invariant: records are sorted by pathname (directories with / suffix)
int record_comparator(const TreeLeafRecord& u, const TreeLeafRecord& v) {
    std::string u_path{u.path.string() + ((u.mode[0] == '4') ? "/" : "")};
    std::string v_path{v.path.string() + ((v.mode[0] == '4') ? "/" : "")};
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

    // Tree invariant: records sorted by directory path name
    sort(records.begin(), records.end(), record_comparator);

    return records;
}

Bytes serialize_tree_records(const std::vector<TreeLeafRecord>& records) {
    Bytes result{};
    for (auto rec : records) {
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

        auto sha_hex{rec.sha};
        for (std::size_t i = 0; i < sha_hex.size(); i += 2) {
            std::uint8_t hi{hex_value(sha_hex[i])};
            std::uint8_t lo{hex_value(sha_hex[i + 1])};
            result.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
        }
    }
    return result;
}