#include "wyag/utils.hpp"

#include <openssl/sha.h>
#include <zlib.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;

std::optional<GitRepository> try_find_repo(const fs::path& path) {
    if (fs::is_directory(path / ".git")) return GitRepository{path};

    const fs::path parent{path / ".."};
    if (parent != path) return try_find_repo(parent);

    return std::nullopt;
}

GitRepository find_repo(const fs::path& path) {
    if (auto repo = try_find_repo(path)) return std::move(*repo);
    throw std::runtime_error("No git directory found.");
}

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

Bytes read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path.string());
    return Bytes(std::istreambuf_iterator<char>(file),
                 std::istreambuf_iterator<char>());
}

void write_file(const fs::path& path, const Bytes& content) {
    std::ofstream file{path, std::ios::binary};
    if (!file)
        throw std::runtime_error("Failed to create file: " + path.string());
    file.write(reinterpret_cast<const char*>(content.data()),
               static_cast<std::streamsize>(content.size()));
    if (!file)
        throw std::runtime_error("Failed to create file: " + path.string());
}

Bytes streaming_decompress(const Bytes& compressed) {
    z_stream stream{};
    stream.next_in =
        const_cast<Bytef*>(reinterpret_cast<const Bytef*>(compressed.data()));
    stream.avail_in = static_cast<uInt>(compressed.size());

    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error(
            "inflateInit failed during streaming decompression.");
    }

    Bytes result;
    std::uint8_t buffer[K_CHUNK_SIZE];

    int ret{Z_OK};
    do {
        stream.next_out = reinterpret_cast<Bytef*>(buffer);
        stream.avail_out = static_cast<uInt>(K_CHUNK_SIZE);

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            throw std::runtime_error("inflate failed");
        }

        const std::size_t produced = K_CHUNK_SIZE - stream.avail_out;
        result.insert(result.end(), buffer, buffer + produced);

    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return result;
}

std::string get_sha1_hex(const Bytes& bytes) {
    uint8_t digest[SHA_DIGEST_LENGTH];
    SHA1(bytes.data(), bytes.size(), digest);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < SHA_DIGEST_LENGTH; i++) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}
