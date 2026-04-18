#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "wyag/git_repository.hpp"

struct Timestamp {
    int seconds{};
    int nanoseconds{};
};

struct IndexEntry {
    Timestamp change_metadata{};
    Timestamp change_data{};
    int device_id{};
    int inode{};
    int mode_type{};  // b1000 (regular), b1010 (symlink), b1110 (gitlink)
    int mode_perms{};
    int uid{};
    int gid{};
    std::size_t object_size{};
    std::string sha{};
    bool assume_valid{};
    int flag_stage{};
    std::filesystem::path object_name{};
};

struct Index {
    int version{2};
    std::vector<IndexEntry> entries{};
};

Index read_index(const GitRepository& repo);

void write_index(const GitRepository& repo, const Index& index);