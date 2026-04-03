#include "wyag/git_objects.hpp"

#include <openssl/sha.h>
#include <zlib.h>

#include <algorithm>

#include "wyag/utils.hpp"

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

Blob Blob::from_bytes(Bytes::const_iterator begin, Bytes::const_iterator end) {
    return Blob{Bytes{begin, end}};
}

std::unique_ptr<GitObject> read_object(const GitRepository& repo,
                                       std::string_view sha) {
    auto path_opt{
        repo_file(repo, {"objects", sha.substr(0, 2), sha.substr(2)})};
    if (!path_opt) return nullptr;

    Bytes compressed = read_file(path_opt.value());
    Bytes raw = streaming_decompress(compressed);

    // Get format
    auto type_end =
        std::find(raw.begin(), raw.end(), static_cast<std::uint8_t>(' '));
    if (type_end == raw.end())
        throw std::runtime_error("Error finding format.");
    std::string format{raw.begin(), type_end};

    // Get object size
    auto size_begin = std::next(type_end);
    auto size_end = std::find(size_begin, raw.end(), std::uint8_t{0});
    if (size_end == raw.end()) throw std::runtime_error("Error finding size.");
    size_t osize = std::stoul(std::string{size_begin, size_end});

    // Object dispatch
    auto content_begin = std::next(size_end);
    if (format == "blob") {
        return std::make_unique<Blob>(
            Blob::from_bytes(content_begin, raw.end()));
    }
    throw std::runtime_error("Unknown object type: " + format);
}

std::string write_object(const GitObject& obj, const GitRepository* repo) {
    Bytes content = obj.serialize();

    std::string header =
        std::string{obj.type()} + ' ' + std::to_string(content.size()) + '\0';

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