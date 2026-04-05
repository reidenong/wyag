#pragma once
#include <string>
#include <string_view>

#include "wyag/git_repository.hpp"

namespace CLI {
class App;
}

// Constants
inline constexpr std::array<std::string_view, 4> GIT_OBJECT_TYPES{
    "blob", "commit", "tag", "tree"};

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
    std::string object_type{"blob"};
    std::string object_id{};
};
struct CatFileBinding {
    CLI::App* subcommand{};
    CatFileOptions options{};
};
CatFileBinding register_catfile(CLI::App& app);
int run_catfile(const CatFileOptions& opts);

// hash-file Command
struct HashFileOptions {
    std::string object_type{"blob"};
    bool perform_write{false};
    std::string path{"."};
};
struct HashFileBinding {
    CLI::App* subcommand{};
    HashFileOptions options{};
};
HashFileBinding register_hashfile(CLI::App& app);
int run_hashfile(const HashFileOptions& opts);

// log command
struct LogOptions {
    std::string commit{"HEAD"};
};
struct LogBinding {
    CLI::App* subcommand{};
    LogOptions options{};
};
LogBinding register_log(CLI::App& app);
int run_log(const LogOptions& opts);

// ls-tree command
struct LsTreeOptions{
    bool recursive {false};
    std::string tree_sha {};
};
struct LsTreeBinding {
    CLI:: App* subcommand {};
    LsTreeOptions options{};
};
LsTreeBinding register_lstree(CLI::App& app);
int run_lstree(const LsTreeOptions& opts);