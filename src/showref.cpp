#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/ref.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;

ShowRefBinding register_showref(CLI::App& app) {
    ShowRefBinding binding{};
    binding.subcommand =
        app.add_subcommand("show-ref", "List references in a repository.");
    return binding;
}

void print_refdir(const RefDirectory& refdir, fs::path prefix, bool with_hash) {
    for (const auto& dr : refdir.refs) {
        fs::path ref_name = prefix / dr.name;
        if (with_hash) {
            std::cout << dr.sha << ' ' << ref_name.string() << std::endl;
        } else {
            std::cout << ref_name.string() << std::endl;
        }
    }
    for (const auto& dir : refdir.subdir) {
        print_refdir(dir, prefix / dir.name, with_hash);
    }
}

int run_showref(const ShowRefOptions& opts) {
    GitRepository repo = find_repo();
    RefDirectory refs = list_repo_refs(repo, repo.get_gitdir() / "refs");
    print_refdir(refs, "refs");
    return 0;
}
