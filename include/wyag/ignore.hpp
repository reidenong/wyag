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

enum class CheckResult {
    unmatched,
    ignore,
    no_ignore,
};

std::optional<IgnoreRule> parse1_gitignore(std::string_view s);

std::vector<IgnoreRule> parse_gitignore(std::string_view content);

CheckResult ignore1_check(const IgnoreRule& rule, std::string_view path);

CheckResult check_scoped_ignore(const std::vector<ScopedRule>& rules, std::string_view path);

CheckResult check_absolute_ignore(const std::vector<IgnoreRule>& rules, std::string_view path);

GitIgnore read_gitignore(const GitRepository& repo);
