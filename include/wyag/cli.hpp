#pragma once
#include <string>

namespace CLI {
class App;
}

// Init Command
struct InitOptions {
    std::string path{"."};
};
struct InitBinding {
    CLI::App* subcommand{};
    InitOptions options{};
};
InitBinding register_init(CLI::App& app);
int run_init(const InitOptions& opts);
