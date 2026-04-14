#include <filesystem>
#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/ignore.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

StatusBinding register_status(CLI::App& app, StatusOptions& options) {
    StatusBinding binding{};
    binding.subcommand =
        app.add_subcommand("status", "Show the working tree status.");
    return binding;
}

int run_status(const StatusOptions& opts) {
    GitRepository repo = find_repo();
    Index index = read_index(repo);

    // cmd_status_branch(repo)
    // cmd_status_head_index
    std::cout << std::endl;
    // cmd_status_index_worktree
    return 0;
}
