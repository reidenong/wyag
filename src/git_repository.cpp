#include "wyag/git_repository.hpp"

#include <fstream>
#include <stdexcept>

#include "INIReader.h"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

GitRepository::GitRepository(const fs::path& path, bool ignore_checks)
    : worktree{path}, gitdir{path / ".git"} {
    // Path does not exist
    if (!ignore_checks && !fs::exists(path)) {
        throw std::runtime_error("Invalid path.");
    }

    // Find config file and check repositoryformatversion
    fs::path config = gitdir / "config";
    if (!ignore_checks && !fs::exists(config)) {
        throw std::runtime_error("Configuration file missing.");
    }

    conf = std::make_unique<INIReader>(config.string());
    if (!ignore_checks && conf->ParseError() != 0) {
        throw std::runtime_error("Failed to parse config.");
    }

    if (!ignore_checks) {
        const auto vers{
            conf->GetInteger("core", "repositoryformatversion", -1)};
        if (vers != 0) {
            throw std::runtime_error("Unsupported repositoryformatversion");
        }
    }
}

GitRepository::GitRepository(GitRepository&&) noexcept = default;
GitRepository& GitRepository::operator=(GitRepository&&) noexcept = default;
GitRepository::~GitRepository() = default;

GitRepository create_repo(const fs::path& path) {
    GitRepository repo{path, true};

    if (fs::exists(repo.get_worktree())) {
        if (!fs::is_directory(repo.get_worktree())) {
            throw std::runtime_error(path.string() + " is not a directory.");
        }
        if (fs::exists(repo.get_gitdir()) && !fs::is_empty(repo.get_gitdir())) {
            throw std::runtime_error(path.string() + " is not empty.");
        }
    } else {
        fs::create_directories(repo.get_worktree());
    }

    // Create directories
    repo_dir(repo, {"branches"}, true);
    repo_dir(repo, {"objects"}, true);
    repo_dir(repo, {"refs", "tags"}, true);
    repo_dir(repo, {"refs", "heads"}, true);

    // .git/description
    auto desc_filepath_opt{repo_file(repo, {"description"})};
    if (!desc_filepath_opt)
        throw std::runtime_error("Failed to create description path.");
    std::ofstream{desc_filepath_opt.value()}
        << "Unnamed repository; edit this file to name the repo.";

    // .git/HEAD
    auto head_filepath_opt{repo_file(repo, {"HEAD"})};
    if (!head_filepath_opt)
        throw std::runtime_error("Failed to create HEAD path.");
    std::ofstream{head_filepath_opt.value()} << "ref: refs/heads/master\n";

    // .git/config
    auto config_filepath_opt{repo_file(repo, {"config"})};
    if (!config_filepath_opt)
        throw std::runtime_error("Failed to create config path.");
    write_default_config(config_filepath_opt.value());

    return repo;
}