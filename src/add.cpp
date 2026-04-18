#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

namespace {

struct PathInfo {
    fs::path absolute;
    fs::path relative;

    friend bool operator<(const PathInfo& lhs, const PathInfo& rhs) {
        return lhs.relative.generic_string() < rhs.relative.generic_string();
    }
};

fs::path normalized_absolute(const fs::path& path) {
    return fs::absolute(path).lexically_normal();
}

bool is_inside_worktree(const fs::path& path, const fs::path& worktree) {
    const fs::path relative = path.lexically_relative(worktree);
    return !relative.empty() && relative.native() != "." &&
           *relative.begin() != "..";
}

IndexEntry make_index_entry(const fs::path& absolute_path,
                            const fs::path& relative_path,
                            const std::string& sha) {
    struct stat st {};
    if (::stat(absolute_path.c_str(), &st) != 0) {
        throw std::runtime_error("Could not stat file: " +
                                 absolute_path.string());
    }

    IndexEntry entry{};
    entry.change_metadata.seconds = static_cast<int>(st.st_ctim.tv_sec);
    entry.change_metadata.nanoseconds = static_cast<int>(st.st_ctim.tv_nsec);
    entry.change_data.seconds = static_cast<int>(st.st_mtim.tv_sec);
    entry.change_data.nanoseconds = static_cast<int>(st.st_mtim.tv_nsec);
    entry.device_id = static_cast<int>(st.st_dev);
    entry.inode = static_cast<int>(st.st_ino);
    entry.mode_type = 0b1000;
    entry.mode_perms = 0644;
    entry.uid = static_cast<int>(st.st_uid);
    entry.gid = static_cast<int>(st.st_gid);
    entry.object_size = static_cast<std::size_t>(st.st_size);
    entry.sha = sha;
    entry.assume_valid = false;
    entry.flag_stage = 0;
    entry.object_name = relative_path;
    return entry;
}

}  // namespace

AddBinding register_add(CLI::App& app, AddOptions& options) {
    AddBinding binding{};
    binding.subcommand =
        app.add_subcommand("add", "Add file contents to the index.");
    binding.subcommand->add_option("path", options.paths, "Paths to add.")
        ->required();
    return binding;
}

void add(const GitRepository& repo, const std::vector<std::string>& paths) {
    rm(repo, paths, false, true);

    const fs::path worktree = normalized_absolute(repo.get_worktree());

    std::set<PathInfo> clean_paths{};
    for (const auto& path : paths) {
        const fs::path absolute_path = normalized_absolute(path);
        if (!is_inside_worktree(absolute_path, worktree) ||
            !fs::is_regular_file(absolute_path)) {
            throw std::runtime_error("Not a file, or outside the worktree: " +
                                     path);
        }

        clean_paths.insert(PathInfo{
            .absolute = absolute_path,
            .relative = absolute_path.lexically_relative(worktree),
        });
    }

    Index index = read_index(repo);
    for (const auto& clean_path : clean_paths) {
        const std::string sha = hash_object(clean_path.absolute, "blob", &repo);
        index.entries.push_back(
            make_index_entry(clean_path.absolute, clean_path.relative, sha));
    }

    std::ranges::sort(index.entries, {}, [](const IndexEntry& entry) {
        return entry.object_name.generic_string();
    });

    write_index(repo, index);
}

int run_add(const AddOptions& opts) {
    GitRepository repo = find_repo();
    add(repo, opts.paths);
    return 0;
}
