#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

struct TreeLeafRecord {
    std::vector<std::uint8_t> mode{};
    std::filesystem::path path{};
    std::string sha{};
};

std::vector<TreeLeafRecord> parse_treeleaf(std::span<const std::uint8_t> raw);
std::vector<std::uint8_t> serialize_tree_records(
    const std::vector<TreeLeafRecord>& records);