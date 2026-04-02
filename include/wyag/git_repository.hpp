#pragma once
#include <filesystem>
#include <memory>

// Configuration reader
class INIReader;

class GitRepository {
   private:
    std::filesystem::path worktree{};
    std::filesystem::path gitdir{};
    std::unique_ptr<INIReader> conf{};

   public:
    explicit GitRepository(const std::filesystem::path& path,
                           bool ignore_checks = false);

    // Prevent copying
    GitRepository(const GitRepository&) = delete;
    GitRepository& operator=(const GitRepository&) = delete;

    // Allow moving
    GitRepository(GitRepository&&) noexcept;
    GitRepository& operator=(GitRepository&&) noexcept;

    ~GitRepository();

    const std::filesystem::path& get_gitdir() const { return gitdir; }
    const std::filesystem::path& get_worktree() const { return worktree; }
};

GitRepository create_repo(const std::filesystem::path& path);