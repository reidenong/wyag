#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_repository.hpp"

InitBinding register_init(CLI::App& app, InitOptions& options) {
    InitBinding binding{};
    binding.subcommand =
        app.add_subcommand("init", "Create an empty Git repository");
    binding.subcommand
        ->add_option("path", options.path, "Directory to initialize.")
        ->default_val(".");
    return binding;
}

int run_init(const InitOptions& opt) {
    create_repo(opt.path);
    return 0;
}
