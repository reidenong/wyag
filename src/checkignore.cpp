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

CheckIgnoreBinding register_checkignore(CLI::App& app,
                                        CheckIgnoreOptions& options) {
    CheckIgnoreBinding binding{};
    binding.subcommand =
        app.add_subcommand("check-ignore", "Check whether paths are ignored.");
    binding.subcommand->add_option("path", options.paths, "Paths to check.")
        ->required();
    return binding;
}

bool check_ignore(const GitIgnore& ignore, std::string_view path) {
    if (fs::path{path}.is_absolute())
        throw std::runtime_error(
            "Checking ignore requires a path relative to repo.");

    CheckResult result = check_scoped_ignore(ignore.scoped, path);
    if (result != CheckResult::unmatched) return result == CheckResult::ignore;
    return check_absolute_ignore(ignore.absolutes, path) == CheckResult::ignore;
}

int run_checkignore(const CheckIgnoreOptions& opts) {
    GitRepository repo = find_repo();
    GitIgnore ignore = read_gitignore(repo);
    for (const auto& path : opts.paths) {
        if (check_ignore(ignore, path)) std::cout << path << std::endl;
    }
    return 0;
}
