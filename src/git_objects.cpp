#include "wyag/git_objects.hpp"

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
