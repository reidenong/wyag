#include "wyag/branch.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <system_error>
#include <vector>

#include "wyag/git_objects.hpp"
#include "wyag/ignore.hpp"
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
                  std::map<std::string, std::string>& mp) {
    auto tree_sha = find_object(repo, tree_ref, "tree");
    if (!tree_sha) return;

    auto obj = read_object(repo, *tree_sha);
    if (!obj || obj->object_type() != "tree") return;
    auto tree = dynamic_cast<Tree*>(obj.get());
    if (!tree) return;
    for (const auto& lr : tree->get_records()) {
        fs::path fullpath = prefix / lr.path;
        bool is_subtree = !lr.mode.empty() && lr.mode[0] == '4';
        if (is_subtree) {
            flatten_tree(repo, lr.sha, fullpath, mp);
        } else {
            mp[fullpath.generic_string()] = lr.sha;
        }
    }
}

void print_staged(const GitRepository& repo, const Index& index) {
    std::cout << "Changes to be committed:" << std::endl;

    std::map<std::string, std::string> head{};
    flatten_tree(repo, "HEAD", "", head);
    for (const auto& entry : index.entries) {
        const std::string name{entry.object_name.generic_string()};
        auto head_entry = head.find(name);
        if (head_entry != head.end()) {
            if (head_entry->second != entry.sha) {
                std::cout << "\tmodified: " << name << std::endl;
            }
            head.erase(head_entry);
        } else {
            std::cout << "\t   added: " << name << std::endl;
        }
    }

    for (const auto& [path, _] : head) {
        std::cout << "\t deleted: " << path << std::endl;
    }
}

void print_unstaged(const GitRepository& repo, const Index& index) {
    std::cout << "Changes not staged for commit:" << std::endl;

    GitIgnore ignore = read_gitignore(repo);
    std::set<std::string> worktree_files{};

    const fs::path& worktree = repo.get_worktree();
    const fs::path& gitdir = repo.get_gitdir();

    std::error_code ec;
    fs::recursive_directory_iterator it{
        worktree, fs::directory_options::skip_permission_denied, ec};
    const fs::recursive_directory_iterator end{};

    // Increment iterator, if there is an issue write into ec
    for (; it != end; it.increment(ec)) {
        // Previous increment hit error, clear and skip
        if (ec) {
            ec.clear();
            continue;
        }

        const fs::path path = it->path();

        // Check if entry is directory, if fails, write into ec
        if (it->is_directory(ec)) {
            // Check if current directory is the same filesystem object as .git
            if (!ec && fs::equivalent(path, gitdir, ec)) {
                it.disable_recursion_pending();  // don't enter this directory
            }
            ec.clear();
            continue;
        }
        ec.clear();

        // If entry is not a normal file, continue
        if (!it->is_regular_file(ec)) {
            ec.clear();
            continue;
        }

        // Get path relative to worktree
        fs::path relative_path = fs::relative(path, worktree, ec);
        if (ec) {
            ec.clear();
            continue;
        }

        // Add discovered worktree file to set of all files present on disk
        worktree_files.insert(relative_path.generic_string());
    }

    // Traverse the index, compare real files with the cached versions.
    for (const auto& entry : index.entries) {
        const std::string name = entry.object_name.generic_string();
        const fs::path full_path = worktree / entry.object_name;

        if (!fs::exists(full_path)) {
            std::cout << "  deleted: " << name << std::endl;
        } else {
            const std::string new_sha = hash_object(full_path, "blob", nullptr);

            // If sha is different files are different
            if (entry.sha != new_sha) {
                std::cout << "  modified: " << name << std::endl;
            }
        }

        worktree_files.erase(name);
    }

    // Untracked files: files in worktree but not in index
    std::cout << std::endl;
    std::cout << "Untracked files:" << std::endl;
    for (const auto& path : worktree_files) {
        if (!check_ignore(ignore, path)) {
            std::cout << " " << path << std::endl;
        }
    }
}
