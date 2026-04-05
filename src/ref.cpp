#include "wyag/ref.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

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

// TODO: Implement
RefDirectory list_repo_refs(const GitRepository& repo,
                            std::filesystem::path path);