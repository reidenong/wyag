#include "wyag/ignore.hpp"

#include <fnmatch.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

namespace {

bool matches(std::string_view pattern, std::string_view target) {
    std::string pattern_str{pattern};
    std::string target_str{target};
    return fnmatch(pattern_str.c_str(), target_str.c_str(), 0) == 0;
}

std::string_view trim_ascii_whitespace(std::string_view s) {
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };

    while (!s.empty() && is_space(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && is_space(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::string_view blob_data_view(const Blob& blob) {
    const Bytes& data = blob.read_data();
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

}  // namespace

std::optional<IgnoreRule> parse1_gitignore(std::string_view s) {
    s = trim_ascii_whitespace(s);

    if (s.empty() || s.front() == '#') return {};
    if (s.front() == '!') return IgnoreRule{std::string{s.substr(1)}, false};
    if (s.front() == '\\') return IgnoreRule{std::string{s.substr(1)}, true};
    return IgnoreRule{std::string{s}, true};
}

std::vector<IgnoreRule> parse_gitignore(std::string_view content) {
    auto start = content.begin();
    auto end = content.end();

    std::vector<IgnoreRule> rules{};
    while (start < end) {
        auto newl = std::find(start, end, '\n');
        auto rule_opt = parse1_gitignore({start, newl});
        if (rule_opt) rules.push_back(std::move(*rule_opt));
        if (newl == end) break;
        start = std::next(newl);
    }
    return rules;
}

CheckResult ignore1_check(const IgnoreRule& rule, std::string_view path) {
    if (!matches(rule.path, path)) return CheckResult::unmatched;
    return rule.to_ignore ? CheckResult::ignore : CheckResult::no_ignore;
}

CheckResult check_ignore1(const std::vector<IgnoreRule>& rules,
                          std::string_view path) {
    CheckResult result = CheckResult::unmatched;
    for (const auto& rule : rules) {
        CheckResult current = ignore1_check(rule, path);
        if (current != CheckResult::unmatched) result = current;
    }
    return result;
}

CheckResult check_scoped_ignore(const std::vector<ScopedRule>& rules,
                                std::string_view path) {
    std::string parent = fs::path{path}.parent_path().generic_string();
    while (true) {
        for (const auto& sr : rules) {
            if (parent != sr.dir_name) continue;
            CheckResult result = check_ignore1(sr.rules, path);
            if (result != CheckResult::unmatched) return result;
        }
        if (parent.empty()) break;
        parent = fs::path{parent}.parent_path().generic_string();
    }
    return CheckResult::unmatched;
}

CheckResult check_absolute_ignore(
    const std::vector<std::vector<IgnoreRule>>& rules, std::string_view path) {
    for (const auto& ruleset : rules) {
        CheckResult result = check_ignore1(ruleset, path);
        if (result != CheckResult::unmatched) return result;
    }
    return CheckResult::no_ignore;  // reasonable default
}

GitIgnore read_gitignore(const GitRepository& repo) {
    GitIgnore ignore{};

    // Local configuration in .git/info/exclude
    fs::path local = repo.get_gitdir() / "info/exclude";
    if (fs::exists(local)) {
        Bytes data = read_file(local);
        std::string_view sv{reinterpret_cast<const char*>(data.data()),
                            data.size()};
        ignore.absolutes.push_back(parse_gitignore(sv));
    }

    // Global configuration
    fs::path global_file{};
    if (const char* home = std::getenv("XDG_CONFIG_HOME")) {
        global_file = fs::path{home} / "git" / "ignore";
    } else if (const char* home = std::getenv("HOME")) {
        global_file = fs::path{home} / ".config" / "git" / "ignore";
    }
    if (fs::exists(global_file)) {
        Bytes data = read_file(global_file);
        std::string_view sv{reinterpret_cast<const char*>(data.data()),
                            data.size()};
        ignore.absolutes.push_back(parse_gitignore(sv));
    }

    // .gitignore files in the index
    Index index = read_index(repo);
    for (const auto& entry : index.entries) {
        std::string name{entry.object_name.string()};
        if (name != ".gitignore" && !name.ends_with("/.gitignore")) continue;
        auto obj = read_object(repo, entry.sha);
        if (!obj || obj->object_type() != "blob") continue;
        const auto* blob = dynamic_cast<const Blob*>(obj.get());
        if (!blob) continue;

        std::string dir_name{entry.object_name.parent_path().generic_string()};
        ignore.scoped.push_back(ScopedRule{
            std::move(dir_name), parse_gitignore(blob_data_view(*blob))});
    }

    return ignore;
}
