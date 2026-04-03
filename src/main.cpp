#include <iostream>
#include <string>

#include "CLI11.hpp"
#include "wyag/cli.hpp"

int main(int argc, char** argv) {
    CLI::App app{"wyag: A (Modern) C++ implementation of a subset of git"};
    app.require_subcommand(1);

    // Register commands
    auto init = register_init(app);
    auto catfile = register_catfile(app);
    auto hashfile = register_hashfile(app);

    // Run command
    try {
        CLI11_PARSE(app, argc, argv);

        if (*init.subcommand) return run_init(init.options);
        if (*catfile.subcommand) return run_catfile(catfile.options);
        if (*hashfile.subcommand) return run_hashfile(hashfile.options);
    } catch (const std::exception& e) {
        std::cerr << "wyag: " << e.what() << '\n';
        return 1;
    }
    std::cerr << "wyag: No valid command was executed.\n";
    return 1;
}