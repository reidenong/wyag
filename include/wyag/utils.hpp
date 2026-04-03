#pragma once
#include <filesystem>
#include <optional>
#include <vector>

#include "wyag/git_repository.hpp"

// Constants
constexpr std::size_t K_CHUNK_SIZE{4096};

// Finds a git repository in the path or its parents
std::optional<GitRepository> find_repo(const std::filesystem::path& path,
                                       bool throw_exception = true);

// General path building function
std::filesystem::path repo_path(
    const GitRepository& repo, const std::vector<std::filesystem::path>& parts);

// Builds filepath, optionally creates the parent dir if it doesn't exist
std::optional<std::filesystem::path> repo_file(
    const GitRepository& repo, const std::vector<std::filesystem::path>& parts,
    bool mkdir = false);

// Builds filepath, optionally creates directory if it doesn't exist
std::optional<std::filesystem::path> repo_dir(
    const GitRepository& repo, const std::vector<std::filesystem::path>& parts,
    bool mkdir = false);

// Write default config
void write_default_config(std::filesystem::path path);

// Read a file into bytes
std::vector<std::uint8_t> read_file(const std::filesystem::path& path);

// Writes bytes into a file
void write_file(const std::filesystem::path& path,
                const std::vector<std::uint8_t>& content);

// Perform streaming decompression with zlib
std::vector<std::uint8_t> streaming_decompress(
    const std::vector<std::uint8_t>& compressed);

// Get sha1 hexidecimal hash for some Bytes
std::string get_sha1_hex(const std::vector<std::uint8_t>& bytes);