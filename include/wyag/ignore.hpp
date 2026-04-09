#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "wyag/git_repository.hpp"

struct IgnoreRule {
    std::string path{};
    bool to_ignore{};
};

struct ScopedRule {
    std::string dir_name{};
    IgnoreRule rule{};
};

struct GitIgnore {
    std::vector<IgnoreRule> absolutes{};
    std::vector<ScopedRule> scoped{};
};

std::optional<IgnoreRule> parse1_gitignore(std::string_view s);

std::vector<IgnoreRule> parse_gitignore(std::string_view content);

GitIgnore read_gitignore(const GitRepository& repo);
