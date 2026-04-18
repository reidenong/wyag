#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "CLI11.hpp"
#include "INIReader.h"
#include "wyag/branch.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/ref.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

namespace {

struct TreeNode {
    std::map<std::string, TreeNode> directories{};
    std::map<std::string, const IndexEntry*> files{};
};

struct UserConfig {
    std::optional<std::string> name{};
    std::optional<std::string> email{};
};

Bytes to_bytes(std::string_view value) {
    return Bytes{value.begin(), value.end()};
}

std::optional<fs::path> home_path() {
    const char* home = std::getenv("HOME");
    if (!home) return std::nullopt;
    return fs::path{home};
}

fs::path expand_user(std::string_view path) {
    if (!path.starts_with("~/")) return fs::path{path};
    auto home = home_path();
    if (!home) return fs::path{path};
    return *home / fs::path{path.substr(2)};
}

void merge_user_config(const fs::path& path, UserConfig& user) {
    if (!fs::is_regular_file(path)) return;

    INIReader config{path.string()};
    if (config.ParseError() != 0) return;

    const std::string name = config.Get("user", "name", "");
    const std::string email = config.Get("user", "email", "");
    if (!name.empty()) user.name = name;
    if (!email.empty()) user.email = email;
}

std::optional<std::string> gitconfig_user_get() {
    UserConfig user{};
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const fs::path xdg_config_home =
        xdg ? fs::path{xdg} : expand_user("~/.config");
    merge_user_config(xdg_config_home / "git" / "config", user);

    if (auto home = home_path()) {
        merge_user_config(*home / ".gitconfig", user);
    }

    if (!user.name || !user.email) return std::nullopt;
    return *user.name + " <" + *user.email + ">";
}

std::vector<std::string> path_parts(const fs::path& path) {
    std::vector<std::string> parts{};
    for (const auto& part : path) {
        parts.push_back(part.generic_string());
    }
    return parts;
}

void insert_entry(TreeNode& root, const IndexEntry& entry) {
    const auto parts = path_parts(entry.object_name);
    if (parts.empty()) {
        throw std::runtime_error("Index entry has empty path.");
    }

    TreeNode* node = &root;
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        node = &node->directories[parts[i]];
    }
    node->files[parts.back()] = &entry;
}

Bytes file_mode(const IndexEntry& entry) {
    std::ostringstream mode{};
    mode << std::oct << std::setfill('0') << std::setw(2) << entry.mode_type
         << std::setw(4) << entry.mode_perms;
    const std::string value = mode.str();
    return Bytes{value.begin(), value.end()};
}

std::string write_tree_node(const GitRepository& repo, const TreeNode& node) {
    std::vector<TreeLeafRecord> records{};

    for (const auto& [name, child] : node.directories) {
        const std::string sha = write_tree_node(repo, child);
        records.push_back(TreeLeafRecord{
            .mode = Bytes{'0', '4', '0', '0', '0', '0'},
            .path = fs::path{name},
            .sha = sha,
        });
    }

    for (const auto& [name, entry] : node.files) {
        records.push_back(TreeLeafRecord{
            .mode = file_mode(*entry),
            .path = fs::path{name},
            .sha = entry->sha,
        });
    }

    return write_object(Tree{std::move(records)}, &repo);
}

std::string tree_from_index(const GitRepository& repo, const Index& index) {
    TreeNode root{};
    for (const auto& entry : index.entries) {
        insert_entry(root, entry);
    }
    return write_tree_node(repo, root);
}

std::string timezone_offset(std::time_t timestamp) {
    std::tm local_tm{};
    if (std::tm* local = std::localtime(&timestamp)) {
        local_tm = *local;
    } else {
        throw std::runtime_error("Could not read local time.");
    }

    char buffer[6]{};
    if (std::strftime(buffer, sizeof(buffer), "%z", &local_tm) == 0) {
        throw std::runtime_error("Could not format timezone offset.");
    }
    return buffer;
}

std::string commit_create(const GitRepository& repo, std::string_view tree,
                          const std::optional<std::string>& parent,
                          std::string_view author, std::time_t timestamp,
                          std::string message) {
    Commit commit{};
    commit.add_header("tree", to_bytes(tree));
    if (parent) {
        commit.add_header("parent", to_bytes(*parent));
    }

    while (!message.empty() &&
           (message.back() == '\n' || message.back() == '\r' ||
            message.back() == ' ' || message.back() == '\t')) {
        message.pop_back();
    }
    message.push_back('\n');

    const std::string identity = std::string{author} + " " +
                                 std::to_string(timestamp) + " " +
                                 timezone_offset(timestamp);
    commit.add_header("author", to_bytes(identity));
    commit.add_header("committer", to_bytes(identity));
    commit.set_message(to_bytes(message));

    return write_object(commit, &repo);
}

void update_head(const GitRepository& repo, std::string_view commit_sha) {
    if (auto branch = get_active_branch(repo)) {
        create_ref(repo, "heads/" + *branch, commit_sha);
        return;
    }

    auto head_path = repo_file(repo, {"HEAD"});
    if (!head_path) throw std::runtime_error("Could not find HEAD.");
    std::string content{commit_sha};
    content.push_back('\n');
    write_file(*head_path, Bytes{content.begin(), content.end()});
}

}  // namespace

CommitBinding register_commit(CLI::App& app, CommitOptions& options) {
    CommitBinding binding{};
    binding.subcommand =
        app.add_subcommand("commit", "Record changes to the repository.");
    binding.subcommand
        ->add_option("-m,--message", options.message,
                     "Message to associate with this commit.")
        ->required();
    return binding;
}

int run_commit(const CommitOptions& opts) {
    GitRepository repo = find_repo();
    Index index = read_index(repo);

    const std::string tree = tree_from_index(repo, index);
    const std::optional<std::string> parent =
        resolve_ref(repo, repo.get_gitdir() / "HEAD");
    const auto author = gitconfig_user_get();
    if (!author) {
        throw std::runtime_error(
            "Could not find user.name and user.email in git config.");
    }

    const std::time_t now = std::time(nullptr);
    const std::string commit =
        commit_create(repo, tree, parent, *author, now, opts.message);
    update_head(repo, commit);
    std::cout << commit << '\n';
    return 0;
}
