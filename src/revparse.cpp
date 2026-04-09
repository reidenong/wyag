#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"
RevParseBinding register_revparse(CLI::App& app, RevParseOptions& options) {
    RevParseBinding binding{};
    binding.subcommand = app.add_subcommand(
        "rev-parse", "Parse revision (or other objects) identifiers");
    binding.subcommand
        ->add_option("-t", options.object_type, "Specify the type.")
        ->check(CLI::IsMember(GIT_OBJECT_TYPES));
    binding.subcommand->add_flag(
        "--no-follow", options.no_follow,
        "Do not follow tags or peel commits to trees.");
    binding.subcommand->add_option("name", options.name,
                                   "The name to parse");
    return binding;
}

int run_revparse(const RevParseOptions& opts) {
    GitRepository repo = find_repo();
    auto obj = find_object(repo, opts.name, opts.object_type, !opts.no_follow);
    if (!obj) {
        throw std::runtime_error(
            "Reference does not resolve to the requested object type.");
    }
    std::cout << *obj << std::endl;
    return 0;
}
