#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/ignore.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

namespace {

fs::path normalized_absolute(const fs::path& path) {
    return fs::absolute(path).lexically_normal();
}

bool is_inside_worktree(const fs::path& path, const fs::path& worktree) {
    const fs::path relative = path.lexically_relative(worktree);
    return !relative.empty() && relative.native() != "." &&
           *relative.begin() != "..";
}

}  // namespace

RmBinding register_rm(CLI::App& app, RmOptions& options) {
    RmBinding binding{};
    binding.subcommand = app.add_subcommand(
        "rm", "Remove files from the working tree and index.");
    binding.subcommand->add_option("path", options.paths, "Paths to remove.")
        ->required();
    binding.subcommand->add_flag("--cached", options.cached,
                                 "Only remove paths from the index.");
    binding.subcommand->add_flag("--ignore-unmatch", options.skip_missing,
                                 "Do not fail if a path is not in the index.");
    return binding;
}

void rm(const GitRepository& repo, const std::vector<std::string>& paths,
        bool delete_files, bool skip_missing) {
    Index index = read_index(repo);

    const fs::path worktree = normalized_absolute(repo.get_worktree());

    std::set<fs::path> requested_paths{};
    for (const auto& path : paths) {
        const fs::path absolute_path = normalized_absolute(path);
        if (!is_inside_worktree(absolute_path, worktree)) {
            throw std::runtime_error("Cannot remove paths outside worktree: " +
                                     path);
        }
        requested_paths.insert(absolute_path);
    }

    std::vector<IndexEntry> kept_entries{};
    kept_entries.reserve(index.entries.size());
    std::vector<fs::path> remove_paths{};

    for (const auto& entry : index.entries) {
        const fs::path full_path =
            normalized_absolute(worktree / entry.object_name);

        if (requested_paths.erase(full_path) > 0) {
            remove_paths.push_back(full_path);
        } else {
            kept_entries.push_back(entry);
        }
    }

    if (!requested_paths.empty() && !skip_missing) {
        throw std::runtime_error("Cannot remove paths not in the index: " +
                                 requested_paths.begin()->string());
    }

    if (delete_files) {
        for (const auto& path : remove_paths) {
            if (!fs::remove(path)) {
                throw std::runtime_error("Failed to remove path: " +
                                         path.string());
            }
        }
    }

    index.entries = std::move(kept_entries);
    write_index(repo, index);
}

int run_rm(const RmOptions& opts) {
    GitRepository repo = find_repo();
    rm(repo, opts.paths, !opts.cached, opts.skip_missing);
    return 0;
}
