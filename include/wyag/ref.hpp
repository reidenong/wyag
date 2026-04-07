#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "git_repository.hpp"

struct DirectRef {
    std::string name{};
    std::string sha{};
};

struct RefDirectory {
    std::string name{};
    std::vector<RefDirectory> subdir{};
    std::vector<DirectRef> refs{};
};

// Create a reference
void create_ref(const GitRepository& repo, std::string_view path,
                std::string_view sha);

// Recursively resolves references to a sha identifier if it exists
std::optional<std::string> resolve_ref(const GitRepository& repo,
                                       std::filesystem::path path);

// Lists all references in a directory
RefDirectory list_repo_refs(const GitRepository& repo,
                            std::filesystem::path path);