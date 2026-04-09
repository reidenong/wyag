#include <iostream>

#include "CLI11.hpp"
#include "wyag/cli.hpp"
#include "wyag/git_objects.hpp"
#include "wyag/git_repository.hpp"
#include "wyag/index.hpp"
#include "wyag/utils.hpp"

LsFilesBinding register_lsfiles(CLI::App& app, LsFilesOptions& options) {
    LsFilesBinding binding{};
    binding.subcommand =
        app.add_subcommand("ls-files", "List all the stage files.");
    binding.subcommand->add_flag("-v", options.is_verbose, "Show metadata.");
    return binding;
}

int run_lsfiles(const LsFilesOptions& opts) {
    GitRepository repo = find_repo();
    Index index = read_index(repo);

    if (opts.is_verbose) {
        std::cout << "Index file format v" << index.version << ", containing "
                  << index.entries.size() << " entries" << std::endl;
    }

    for (const auto& entry : index.entries) {
        std::cout << entry.object_name.string() << '\n';

        if (opts.is_verbose) {
            if (entry.mode_type == 0b1000)
                std::cout << "\tregular file ";
            else if (entry.mode_type == 0b1010)
                std::cout << "\tsymlink file ";
            else if (entry.mode_type == 0b1110)
                std::cout << "\tgit link file ";
            std::cout << '\n';

            std::cout << "\twith perms: " << entry.mode_perms << '\n';

            std::cout << "\ton blob: " << entry.sha << '\n';
            std::cout << "\tcreated: " << entry.change_data.seconds << '\n';
            std::cout << "\tdevice: " << entry.device_id
                      << "\n\tinode: " << entry.inode << '\n';
        }
    }
    return 0;
}
