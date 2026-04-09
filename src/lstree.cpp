#include <iostream>
#include <set>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"

LsTreeBinding register_lstree(CLI::App& app, LsTreeOptions& options) {
    LsTreeBinding binding{};
    binding.subcommand =
        app.add_subcommand("ls-tree", "Display history of a given commit");
    binding.subcommand->add_flag("-r", options.recursive,
                                 "Recurse into sub-trees.");
    binding.subcommand->add_option("tree", options.tree_sha,
                                   "tree sha.");
    return binding;
}

void print_tree(std::string_view tree_sha, int level, bool recurse,
                const GitRepository& repo) {
    auto obj = read_object(repo, tree_sha);
    if (!obj) throw std::runtime_error("tree object does not exist.");

    auto* tree = dynamic_cast<Tree*>(obj.get());
    if (!tree) throw std::runtime_error("object is not a tree.");

    std::string prefix{};
    for (int i = 0; i < level; i++) {
        prefix += '\t';
    }
    for (auto rec : tree->get_records()) {
        std::string mode{rec.mode.begin(), rec.mode.end()};
        std::cout << prefix << mode << ' ' << rec.path.string() << ' '
                  << rec.sha << std::endl;
        if (recurse && mode[0] == '4') {
            print_tree(rec.sha, level + 1, recurse, repo);
        }
    }
}

int run_lstree(const LsTreeOptions& opts) {
    GitRepository repo = find_repo();
    print_tree(opts.tree_sha, 0, opts.recursive, repo);
    return 0;
}
