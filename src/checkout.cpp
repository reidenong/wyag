#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

CheckoutBinding register_checkout(CLI::App& app, CheckoutOptions& options) {
    CheckoutBinding binding{};
    binding.subcommand = app.add_subcommand(
        "checkout", "Checkout a commit inside of a directory");
    binding.subcommand->add_option("sha", options.sha,
                                   "Commit/tree to checkout.");

    binding.subcommand
        ->add_option("path", options.path,
                     "The EMPTY directory to checkout on.")
        ->default_val(".");
    return binding;
}

void checkout_tree(const GitRepository& repo, const Tree& tree, fs::path path) {
    for (const auto& record : tree.get_records()) {
        auto obj = read_object(repo, record.sha);
        if (!obj) continue;

        if (obj->object_type() == "tree") {
            auto* tree = dynamic_cast<Tree*>(obj.get());
            fs::create_directories(path / record.path);
            checkout_tree(repo, *tree, path / record.path);

        } else if (obj->object_type() == "blob") {
            auto* blob = dynamic_cast<Blob*>(obj.get());
            if (!blob) continue;
            write_file(path / record.path, blob->read_data());
        }
    }
}

void checkout(const GitRepository& repo, std::string sha_or_ref,
              fs::path path) {
    if (fs::exists(path) && !fs::is_directory(path))
        throw std::runtime_error("Not a directory.");
    if (fs::exists(path) && !fs::is_empty(path))
        throw std::runtime_error("Directory is not empty.");

    auto tree_sha = find_object(repo, sha_or_ref, "tree");
    if (!tree_sha) throw std::runtime_error("commit/tree is not resolvable.");

    auto obj = read_object(repo, *tree_sha);
    if (!obj) throw std::runtime_error("commit/tree is not resolvable.");

    if (!fs::exists(path)) fs::create_directories(path);
    auto* tree = dynamic_cast<Tree*>(obj.get());
    if (!tree) throw std::runtime_error("commit/tree is not resolvable.");
    checkout_tree(repo, *tree, path);
}

int run_checkout(const CheckoutOptions& opts) {
    GitRepository repo = find_repo();
    checkout(repo, opts.sha, fs::path{opts.path});
    return 0;
}
