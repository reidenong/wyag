#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/utils.hpp"

CatFileBinding register_catfile(CLI::App& app, CatFileOptions& options) {
    CatFileBinding binding{};
    binding.subcommand =
        app.add_subcommand("cat-file", "Provide content of repository object");
    binding.subcommand
        ->add_option("object_type", options.object_type, "Specify the type.")
        ->check(CLI::IsMember(GIT_OBJECT_TYPES));
    binding.subcommand->add_option("object_id", options.object_id,
                                   "The identifier of the object to display.");
    return binding;
}

void cat_file(const GitRepository& repo, std::string_view obj_id,
              std::string_view object_type) {
    auto sha = find_object(repo, obj_id, object_type);
    if (!sha) {
        throw std::runtime_error("No object found with type " +
                                 std::string{object_type} + ".");
    }

    std::unique_ptr<GitObject> obj = read_object(repo, *sha);

    if (!obj) throw std::runtime_error("Object does not exist.");
    auto bytes = obj->serialize();
    std::cout.write(reinterpret_cast<const char*>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size()));
    if (!std::cout)
        throw std::runtime_error("cat-file failed to write to stdout.");
}

int run_catfile(const CatFileOptions& opt) {
    GitRepository repo = find_repo();
    cat_file(repo, opt.object_id, opt.object_type);
    return 0;
}
