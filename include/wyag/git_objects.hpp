#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "wyag/git_repository.hpp"
#include "wyag/kvlm.hpp"
#include "wyag/tree.hpp"

class GitObject {
   public:
    using Bytes = std::vector<std::uint8_t>;

    virtual ~GitObject() = default;
    virtual std::string_view object_type() const = 0;
    virtual Bytes serialize() const = 0;
};

class Blob : public GitObject {
   private:
    Bytes data{};

   public:
    explicit Blob(Bytes data) : data(std::move(data)) {}
    std::string_view object_type() const noexcept override { return "blob"; }
    Bytes serialize() const override { return data; }
    static Blob from_bytes(std::span<const std::uint8_t> data);
    const Bytes& read_data() const { return data; }
};

class Commit : public GitObject {
   private:
    Kvlm kvlm{};

   public:
    explicit Commit(Kvlm kvlm) : kvlm(std::move(kvlm)) {}
    std::string_view object_type() const noexcept override { return "commit"; }
    Bytes serialize() const override;
    static Commit from_bytes(std::span<const std::uint8_t> data);
    const Kvlm& read_kvlm() const { return kvlm; }
};

class Tree : public GitObject {
   private:
    std::vector<TreeLeafRecord> records{};

   public:
    explicit Tree(std::vector<TreeLeafRecord> records)
        : records(std::move(records)) {}
    std::string_view object_type() const noexcept override { return "tree"; }
    Bytes serialize() const override;
    static Tree from_bytes(std::span<const std::uint8_t> raw);
    const std::vector<TreeLeafRecord>& get_records() const { return records; }
};

// Read object sha from repo, returning the relevant object
std::unique_ptr<GitObject> read_object(const GitRepository& repo,
                                       std::string_view sha);

// Compute the hash and write object in the correct location
std::string write_object(const GitObject& obj, const GitRepository* repo);

// Find an object based on its identifier
std::string find_object(const GitRepository& repo, std::string_view name,
                        std::string_view object_type);

// Compute object hash and optionally create a blob from a file
std::string hash_object(const std::filesystem::path& path,
                        std::string_view object_type,
                        const GitRepository* repo);
