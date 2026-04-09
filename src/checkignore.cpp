#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

CheckIgnoreBinding register_checkignore(CLI::App& app,
                                        CheckIgnoreOptions& options) {
    CheckIgnoreBinding binding{};
    binding.subcommand =
        app.add_subcommand("check-ignore", "Check whether paths are ignored.");
    binding.subcommand->add_option("path", options.paths, "Paths to check.")
        ->required();
    return binding;
}

int run_checkignore(const CheckIgnoreOptions& opts) {
    GitRepository repo = find_repo();
    // rules = read_gitignore(repo);
    // for (const auto& path : opts.paths) {
    //     if (check_ignore(path, rules)) std::cout << path << std::endl;
    // }
    return 0;
}
