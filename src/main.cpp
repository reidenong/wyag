#include <iostream>
#include <string>

#include "CLI11.hpp"
#include "wyag/cli.hpp"

int main(int argc, char** argv) {
    CLI::App app{"wyag: A (Modern) C++ implementation of a subset of git"};
    app.require_subcommand(1);

    // Register commands
    InitOptions init_options{};
    CatFileOptions catfile_options{};
    HashFileOptions hashfile_options{};
    LsTreeOptions lstree_options{};
    CheckoutOptions checkout_options{};
    TagOptions tag_options{};
    RevParseOptions revparse_options{};
    LsFilesOptions lsfiles_options{};
    CheckIgnoreOptions checkignore_options{};
    StatusOptions status_options{};

    auto init = register_init(app, init_options);
    auto catfile = register_catfile(app, catfile_options);
    auto hashfile = register_hashfile(app, hashfile_options);
    auto lstree = register_lstree(app, lstree_options);
    auto checkout = register_checkout(app, checkout_options);
    auto showref = register_showref(app);
    auto tag = register_tag(app, tag_options);
    auto revparse = register_revparse(app, revparse_options);
    auto lsfiles = register_lsfiles(app, lsfiles_options);
    auto checkignore = register_checkignore(app, checkignore_options);
    auto status = register_status(app, status_options);

    // Run command
    try {
        CLI11_PARSE(app, argc, argv);

        if (*init.subcommand) return run_init(init_options);
        if (*catfile.subcommand) return run_catfile(catfile_options);
        if (*hashfile.subcommand) return run_hashfile(hashfile_options);
        if (*lstree.subcommand) return run_lstree(lstree_options);
        if (*checkout.subcommand) return run_checkout(checkout_options);
        if (*showref.subcommand) return run_showref(showref.options);
        if (*tag.subcommand) return run_tag(tag_options);
        if (*revparse.subcommand) return run_revparse(revparse_options);
        if (*lsfiles.subcommand) return run_lsfiles(lsfiles_options);
        if (*checkignore.subcommand)
            return run_checkignore(checkignore_options);
        if (*status.subcommand) return run_status(status_options);
    } catch (const std::exception& e) {
        std::cerr << "wyag: " << e.what() << '\n';
        return 1;
    }
    std::cerr << "wyag: No valid command was executed.\n";
    return 1;
}
