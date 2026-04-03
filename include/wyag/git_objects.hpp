#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "wyag/git_repository.hpp"

class GitObject {
   public:
    using Bytes = std::vector<std::uint8_t>;

    virtual ~GitObject() = default;
    virtual std::string_view type() const = 0;
    virtual Bytes serialize() const = 0;
};

class Blob : public GitObject {
   private:
    Bytes data{};

   public:
    explicit Blob(Bytes data) : data(std::move(data)) {}
    std::string_view type() const noexcept override { return "blob"; }
    Bytes serialize() const override { return data; }
    static Blob from_bytes(Bytes::const_iterator begin,
                           Bytes::const_iterator end);
};

// Read object sha from repo, returning the relevant object
std::unique_ptr<GitObject> read_object(const GitRepository& repo,
                                       std::string_view sha);

// Compute the hash and write object in the correct location
std::string write_object(const GitObject& obj, const GitRepository* repo);
