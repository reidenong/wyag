#include <iostream>
#include <set>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"

LogBinding register_log(CLI::App& app) {
    LogBinding binding{};
    binding.subcommand =
        app.add_subcommand("log", "Display history of a given commit");
    binding.subcommand
        ->add_option("commit", binding.options.commit, "Commit to start at.")
        ->default_val("HEAD");
    return binding;
}

void recurse_log(std::string commit_sha, int level,
                 std::set<std::string>& seen_commits,
                 const GitRepository& repo) {
    if (seen_commits.count(commit_sha)) return;
    seen_commits.insert(commit_sha);

    auto obj = read_object(repo, commit_sha);
    if (!obj) throw std::runtime_error("Commit does not exist.");

    auto* commit = dynamic_cast<Commit*>(obj.get());
    if (!commit) throw std::runtime_error("Object is not a commit");

    for (int i = 0; i < level; i++) std::cout << '\t';
    std::cout << "commit: " << std::string_view{commit_sha}.substr(0, 8)
              << std::endl;

    for (const auto& entry : commit->read_kvlm().headers) {
        if (entry.key != "parent") continue;
        recurse_log(std::string{entry.value.begin(), entry.value.end()},
                    level + 1, seen_commits, repo);
    }
}

int run_log(const LogOptions& opts) {
    GitRepository repo = find_repo();
    std::set<std::string> seen_commits{};

    // TODO: Resolve commit to the real SHA (could be HEAD)
    recurse_log(opts.commit, 0, seen_commits, repo);
    return 0;
}
