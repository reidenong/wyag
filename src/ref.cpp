#include "wyag/ref.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>

#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

void create_ref(const GitRepository& repo, std::string_view ref_name,
                std::string_view sha) {
    auto filepath = repo_file(repo, {"refs/", ref_name}, true);
    if (!filepath) return;
    std::string content = std::string{sha};
    content.push_back('\n');
    write_file(*filepath, {content.begin(), content.end()});
}

std::optional<std::string> resolve_ref(const GitRepository& repo,
                                       fs::path path) {
    if (!fs::is_regular_file(path)) return {};
    Bytes bytes = read_file(path);

    while (!bytes.empty() && bytes.back() == '\n') bytes.pop_back();
    std::size_t length = bytes.size();

    const auto* start = reinterpret_cast<const char*>(bytes.data());
    if (length >= 5 && std::string_view{start, 5} == "ref: ") {
        return resolve_ref(
            repo, repo.get_gitdir() /
                      fs::path{std::string_view{start + 5, length - 5}});
    }
    return std::string{bytes.begin(), bytes.end()};
}

RefDirectory list_repo_refs(const GitRepository& repo,
                            std::filesystem::path dir_path) {
    RefDirectory refdir{dir_path.filename()};

    if (!fs::is_directory(dir_path))
        throw std::runtime_error("Not a directory");

    std::vector<fs::path> entries{};
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        entries.push_back(entry.path());
    }

    std::sort(entries.begin(), entries.end(),
              [](const fs::path& a, const fs::path& b) {
                  return a.filename().string() < b.filename().string();
              });

    for (const auto& entry_path : entries) {
        if (fs::is_directory(entry_path)) {
            refdir.subdir.push_back(
                std::move(list_repo_refs(repo, entry_path)));
        } else if (fs::is_regular_file(entry_path)) {
            auto sha_opt = resolve_ref(repo, entry_path);
            if (!sha_opt) continue;
            refdir.refs.push_back(
                DirectRef{entry_path.filename().string(), sha_opt.value()});
        }
    }
    return refdir;
}
