#include "wyag/git_objects.hpp"

#include <openssl/sha.h>
#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <ranges>

#include "wyag/ref.hpp"
#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

namespace {

std::string_view trim_ascii_whitespace(std::string_view sv) {
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };

    while (!sv.empty() && is_space(static_cast<unsigned char>(sv.front()))) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && is_space(static_cast<unsigned char>(sv.back()))) {
        sv.remove_suffix(1);
    }
    return sv;
}

}  // namespace

// Git Objects

Blob Blob::from_bytes(std::span<const std::uint8_t> data) {
    return Blob{Bytes{data.begin(), data.end()}};
}

Bytes Commit::serialize() const { return kvlm_serialize(kvlm); }

Commit Commit::from_bytes(std::span<const std::uint8_t> data) {
    return Commit{kvlm_parse(data)};
}

void Commit::add_header(std::string_view key,
                        std::span<const std::uint8_t> data) {
    kvlm.headers.emplace_back(std::string{key},
                              Bytes{data.begin(), data.end()});
}

void Commit::set_message(std::span<const std::uint8_t> message) {
    kvlm.message = Bytes{message.begin(), message.end()};
}

Bytes Tree::serialize() const { return serialize_tree_records(records); }

Tree Tree::from_bytes(std::span<const std::uint8_t> raw) {
    return Tree{parse_treeleaf(raw)};
}

// Object utilities

std::unique_ptr<GitObject> read_object(const GitRepository& repo,
                                       std::string_view sha) {
    auto path_opt{
        repo_file(repo, {"objects", sha.substr(0, 2), sha.substr(2)})};
    if (!path_opt) return nullptr;

    Bytes compressed = read_file(path_opt.value());
    Bytes raw = streaming_decompress(compressed);

    // Get object type
    auto type_end =
        std::find(raw.begin(), raw.end(), static_cast<std::uint8_t>(' '));
    if (type_end == raw.end())
        throw std::runtime_error("Error finding object type.");
    std::string object_type{raw.begin(), type_end};

    // Get object size
    auto size_begin = std::next(type_end);
    auto size_end = std::find(size_begin, raw.end(), std::uint8_t{0});
    if (size_end == raw.end()) throw std::runtime_error("Error finding size.");

    // Checking content size
    size_t osize = std::stoul(std::string{size_begin, size_end});
    auto content_begin = std::next(size_end);
    size_t content_size = raw.end() - content_begin;
    if (osize != content_size)
        throw std::runtime_error(
            "Actual content size is different from size stated in header.");

    // TODO: Implement for other object types
    // Object dispatch
    if (object_type == "blob") {
        return std::make_unique<Blob>(
            Blob::from_bytes(std::span<const std::uint8_t>{raw}.subspan(
                content_begin - raw.begin())));
    }
    if (object_type == "commit") {
        return std::make_unique<Commit>(
            Commit::from_bytes(std::span<const std::uint8_t>{raw}.subspan(
                content_begin - raw.begin())));
    }
    if (object_type == "tree") {
        return std::make_unique<Tree>(
            Tree::from_bytes(std::span<const std::uint8_t>{raw}.subspan(
                content_begin - raw.begin())));
    }
    if (object_type == "tag") {
        return std::make_unique<Tag>(
            Tag::from_bytes(std::span<const std::uint8_t>{raw}.subspan(
                content_begin - raw.begin())));
    }

    throw std::runtime_error("Unknown object type: " + object_type);
}

std::string write_object(const GitObject& obj, const GitRepository* repo) {
    Bytes content = obj.serialize();

    std::string header = std::string{obj.object_type()} + ' ' +
                         std::to_string(content.size()) + '\0';

    Bytes result{};
    result.reserve(header.size() + content.size());
    result.insert(result.end(), header.begin(), header.end());
    result.insert(result.end(), content.begin(), content.end());

    std::string sha = get_sha1_hex(result);

    if (repo) {
        std::string_view sha_sv{sha};
        auto path_opt = repo_file(
            *repo, {"objects", sha_sv.substr(0, 2), sha_sv.substr(2)}, true);
        if (!path_opt) throw std::runtime_error("Cannot create repo file.");

        // Perform zlib compression
        uLongf output_size{compressBound(result.size())};
        Bytes compressed_result(output_size);
        int rc = compress(reinterpret_cast<Bytef*>(compressed_result.data()),
                          &output_size,
                          reinterpret_cast<const Bytef*>(result.data()),
                          static_cast<uLong>(result.size()));
        compressed_result.resize(output_size);

        if (rc != Z_OK) throw std::runtime_error("zlib compression failed.");
        write_file(path_opt.value(), compressed_result);
    }
    return sha;
}

std::vector<std::string> resolve_object(const GitRepository& repo,
                                        std::string_view name) {
    name = trim_ascii_whitespace(name);

    if (name.empty()) return {};

    if (name == "HEAD") {
        auto ref = resolve_ref(repo, repo.get_gitdir() / "HEAD");
        if (ref) return {*ref};
        return {};
    }

    std::vector<std::string> candidates{};

    // Try for hash
    auto is_hex = [](unsigned char ch) { return std::isxdigit(ch) != 0; };

    if (name.size() >= 4 && name.size() <= 40 &&
        std::ranges::all_of(name, is_hex)) {
        std::string lowered{name};
        std::ranges::transform(lowered, lowered.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        const std::string prefix = lowered.substr(0, 2);
        const std::string remainder = lowered.substr(2);

        auto object_dir = repo_dir(repo, {"objects", prefix}, false);
        if (object_dir) {
            for (const auto& entry : fs::directory_iterator(*object_dir)) {
                if (!entry.is_regular_file()) continue;

                const std::string filename = entry.path().filename().string();
                if (filename.starts_with(remainder)) {
                    candidates.push_back(prefix + filename);
                }
            }
        }
    }

    // Try references
    auto tag_object =
        resolve_ref(repo, repo.get_gitdir() / "refs" / "tags" / fs::path{name});
    if (tag_object) candidates.push_back(*tag_object);

    // Try branches
    auto branch_object = resolve_ref(
        repo, repo.get_gitdir() / "refs" / "heads" / fs::path{name});
    if (branch_object) candidates.push_back(*branch_object);

    // Try remote branches
    auto remote_branch_object = resolve_ref(
        repo, repo.get_gitdir() / "refs" / "remotes" / fs::path{name});
    if (remote_branch_object) candidates.push_back(*remote_branch_object);

    return candidates;
}

std::string find_object(const GitRepository& repo, std::string_view name,
                        std::string_view object_type) {
    // TODO: Expand resolution
    return std::string{name};
}

std::string hash_object(const fs::path& path, std::string_view object_type,
                        const GitRepository* repo) {
    Bytes data = read_file(path);

    // TODO: Implement for other object types
    if (object_type == "blob") {
        return write_object(
            Blob::from_bytes(std::span<const std::uint8_t>{data}), repo);
    } else {
        throw std::runtime_error("Unknown object type detected: " +
                                 std::string{object_type});
    }
}
