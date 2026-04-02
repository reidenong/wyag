#pragma once
#include <filesystem>
#include <optional>
#include <vector>

#include "wyag/git_repository.hpp"

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