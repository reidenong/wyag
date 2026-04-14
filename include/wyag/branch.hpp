#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "wyag/git_repository.hpp"

// Check if we're on a branch, and if so which
std::optional<std::string> get_active_branch(const GitRepository& repo);

// Print the name of the active branch, or the hash of the detached HEAD
void print_branch_status(const GitRepository& repo);

// Find diff between staging area and HEAD
void flatten_tree(const GitRepository& repo, std::string_view tree_ref,
                  const std::filesystem::path& prefix,
                  std::unordered_map<std::string, std::string>& mp);