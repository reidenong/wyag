#pragma once
#include <string>

namespace CLI {
class App;
}

// init Command
struct InitOptions {
    std::string path{"."};
};
struct InitBinding {
    CLI::App* subcommand{};
    InitOptions options{};
};
InitBinding register_init(CLI::App& app);
int run_init(const InitOptions& opts);

// cat-file Command
struct CatFileOptions {
    std::string type{};
    std::string object_id{};
};
struct CatFileBinding {
    CLI::App* subcommand{};
    CatFileOptions options{};
};
CatFileBinding register_catfile(CLI::App& app);
int run_catfile(const CatFileOptions& opts);