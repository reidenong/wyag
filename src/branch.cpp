#include "wyag/branch.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#include "wyag/git_objects.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

// Check if we're on a branch, and if so which
std::optional<std::string> get_active_branch(const GitRepository& repo) {
    auto path_opt = repo_file(repo, {"HEAD"});
    if (!path_opt) return {};

    Bytes bytes = read_file(*path_opt);
    std::string_view sv{reinterpret_cast<const char*>(bytes.data()),
                        bytes.size()};

    if (sv.starts_with("ref: refs/heads/")) {
        auto branch = sv.substr(16);
        if (!branch.empty() && branch.back() == '\n') branch.remove_suffix(1);
        return std::string{branch};
    }
    return {};
}

// Print the name of the active branch, or the hash of the detached HEAD
void print_branch_status(const GitRepository& repo) {
    auto branch = get_active_branch(repo);
    if (branch) {
        std::cout << "On branch " << *branch << "." << std::endl;
    } else {
        auto obj_str = find_object(repo, "HEAD");
        if (!obj_str) throw std::runtime_error("Invalid branch.");
        std::cout << "HEAD detached at " << *obj_str << std::endl;
    }
}

void flatten_tree(const GitRepository& repo, std::string_view tree_ref,
                  const fs::path& prefix,
                  std::unordered_map<std::string, std::string>& mp) {
    auto tree_sha = find_object(repo, tree_ref, "tree");
    if (!tree_sha) return;

    auto obj = read_object(repo, *tree_sha);
    if (!obj || obj->object_type() != "tree") return;
    auto tree = dynamic_cast<Tree*>(obj.get());
    if (!tree) return;
    for (const auto& lr : tree->get_records()) {
        fs::path fullpath = prefix / lr.path;
        bool is_subtree =
            lr.mode.size() >= 2 && lr.mode[0] == '0' && lr.mode[1] == '4';
        if (is_subtree) {
            flatten_tree(repo, lr.sha, fullpath, mp);
        } else {
            mp[fullpath.string()] = lr.sha;
        }
    }
}