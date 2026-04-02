#include "wyag/utils.hpp"

#include <fstream>
#include <stdexcept>
namespace fs = std::filesystem;

fs::path repo_path(const GitRepository& repo,
                   const std::vector<fs::path>& parts) {
    fs::path result{repo.get_gitdir()};
    for (const auto& p : parts) {
        result /= p;
    }
    return result;
}

std::optional<fs::path> repo_dir(const GitRepository& repo,
                                 const std::vector<fs::path>& parts,
                                 bool mkdir) {
    fs::path path{repo_path(repo, parts)};

    if (fs::exists(path)) {
        if (fs::is_directory(path)) return path;
        throw std::runtime_error("Not a directory: " + path.string());
    }

    if (mkdir) {
        fs::create_directories(path);
        return path;
    }
    return std::nullopt;
}

std::optional<fs::path> repo_file(const GitRepository& repo,
                                  const std::vector<fs::path>& parts,
                                  bool mkdir) {
    if (parts.empty()) throw std::invalid_argument("Invalid repo_file path.");
    std::vector<fs::path> parent(parts.begin(), parts.end() - 1);
    if (repo_dir(repo, parent, mkdir)) {
        return repo_path(repo, parts);
    }
    return std::nullopt;
}

void write_default_config(fs::path path) {
    std::ofstream file{path};
    if (!file) throw std::runtime_error("Failed to create config file.");

    file << "[core]\n";
    file << "\trepositoryformatversion = 0\n";
    file << "\tfilemode = false\n";
    file << "\tbare = false\n";
}