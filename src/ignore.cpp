#include "wyag/ignore.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
std::optional<IgnoreRule> parse1_gitignore(std::string_view s) {
    while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
    while (!s.empty() && s.back() == ' ') s.remove_suffix(1);

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

GitIgnore read_gitignore(const GitRepository& repo) {
    GitIgnore ignore{};

    // Local configuration in .git/info/exclude
    fs::path local = repo.get_gitdir() / "info/exclude";
    if (fs::exists(local)) {
        Bytes data = read_file(local);
        std::string_view sv{reinterpret_cast<const char*>(data.data()),
                            data.size()};
        ignore.absolutes = parse_gitignore(sv);
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
        auto rules = parse_gitignore(sv);
        ignore.absolutes.insert(ignore.absolutes.end(), rules.begin(),
                                rules.end());
    }

    // .gitignore files in the index
    Index index = read_index(repo);
    for (const auto& entry : index.entries) {
        std::string name{entry.object_name.string()};
        if (name != ".gitignore" && !name.ends_with("/.gitignore")) continue;
        auto obj = read_object(repo, entry.sha);
        if (!obj || obj->object_type() != "blob") continue;
        Blob* blob = dynamic_cast<Blob*>(obj.get());
        std::string_view sv{
            reinterpret_cast<const char*>(blob->read_data().data()),
            blob->read_data().size()};
        auto scoped_rules = parse_gitignore(sv);

        std::string dir_name{entry.object_name.parent_path().string()};
        for (auto& rule : scoped_rules) {
            ignore.scoped.push_back(ScopedRule{dir_name, std::move(rule)});
        }
    }

    return ignore;
}