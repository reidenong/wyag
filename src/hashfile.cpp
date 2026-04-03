#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"

HashFileBinding register_hashfile(CLI::App& app) {
    HashFileBinding binding{};
    binding.subcommand = app.add_subcommand(
        "hash-file", "computes a hash and optionally writes to repository");
    binding.subcommand
        ->add_option("-t", binding.options.object_type, "Specify the type.")
        ->check(CLI::IsMember(GIT_OBJECT_TYPES));
    binding.subcommand->add_option("-w", binding.options.perform_write,
                                   "Actually write objects into file.");
    binding.subcommand->add_option("path", binding.options.path,
                                   "Read object from <file>");
    return binding;
}

int run_hashfile(const HashFileOptions& opts) {
    if (opts.perform_write) {
        GitRepository repo = find_repo();
        std::cout << hash_object(opts.path, opts.object_type, &repo) << '\n';
    } else {
        std::cout << hash_object(opts.path, opts.object_type, nullptr) << '\n';
    }
    return 0;
}
