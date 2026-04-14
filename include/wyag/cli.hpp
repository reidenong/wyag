#pragma once
#include <string>
#include <string_view>

#include "wyag/git_repository.hpp"
#include "wyag/ref.hpp"

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
};
InitBinding register_init(CLI::App& app, InitOptions& options);
int run_init(const InitOptions& opts);

// cat-file Command
struct CatFileOptions {
    std::string object_type{"blob"};
    std::string object_id{};
};
struct CatFileBinding {
    CLI::App* subcommand{};
};
CatFileBinding register_catfile(CLI::App& app, CatFileOptions& options);
int run_catfile(const CatFileOptions& opts);

// hash-file Command
struct HashFileOptions {
    std::string object_type{"blob"};
    bool perform_write{false};
    std::string path{"."};
};
struct HashFileBinding {
    CLI::App* subcommand{};
};
HashFileBinding register_hashfile(CLI::App& app, HashFileOptions& options);
int run_hashfile(const HashFileOptions& opts);

// log command
struct LogOptions {
    std::string commit{"HEAD"};
};
struct LogBinding {
    CLI::App* subcommand{};
};
LogBinding register_log(CLI::App& app, LogOptions& options);
int run_log(const LogOptions& opts);

// ls-tree command
struct LsTreeOptions {
    bool recursive{false};
    std::string tree_sha{};
};
struct LsTreeBinding {
    CLI::App* subcommand{};
};
LsTreeBinding register_lstree(CLI::App& app, LsTreeOptions& options);
int run_lstree(const LsTreeOptions& opts);

// checkout command
struct CheckoutOptions {
    std::string sha{};
    std::string path{};
};
struct CheckoutBinding {
    CLI::App* subcommand{};
};
CheckoutBinding register_checkout(CLI::App& app, CheckoutOptions& options);
int run_checkout(const CheckoutOptions& opts);

// show-ref command
struct ShowRefOptions {};
struct ShowRefBinding {
    CLI::App* subcommand{};
    ShowRefOptions options{};
};
ShowRefBinding register_showref(CLI::App& app);
void print_refdir(const RefDirectory& refdir, std::filesystem::path prefix = {},
                  bool with_hash = true);
int run_showref(const ShowRefOptions& opts);

// tag command
struct TagOptions {
    std::string name{};
    std::string object_sha{};
    bool is_tag_object{false};
};
struct TagBinding {
    CLI::App* subcommand{};
};
TagBinding register_tag(CLI::App& app, TagOptions& options);
int run_tag(const TagOptions& opts);

// rev-parse command
struct RevParseOptions {
    std::string object_type{};
    std::string name{};
    bool no_follow{false};
};
struct RevParseBinding {
    CLI::App* subcommand{};
};
RevParseBinding register_revparse(CLI::App& app, RevParseOptions& options);
int run_revparse(const RevParseOptions& opts);

// ls-files command
struct LsFilesOptions {
    bool is_verbose{};
};
struct LsFilesBinding {
    CLI::App* subcommand{};
};
LsFilesBinding register_lsfiles(CLI::App& app, LsFilesOptions& options);
int run_lsfiles(const LsFilesOptions& opts);

// check-ignore command
struct CheckIgnoreOptions {
    std::vector<std::string> paths{};
};
struct CheckIgnoreBinding {
    CLI::App* subcommand{};
};
CheckIgnoreBinding register_checkignore(CLI::App& app,
                                        CheckIgnoreOptions& options);
int run_checkignore(const CheckIgnoreOptions& opts);

// status command
struct StatusOptions {};
struct StatusBinding {
    CLI::App* subcommand{};
};
StatusBinding register_status(CLI::App& app, StatusOptions& options);
int run_status(const StatusOptions& opts);