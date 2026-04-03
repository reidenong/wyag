#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct KvlmEntry {
    std::string key{};
    std::vector<std::uint8_t> value{};
};

struct Kvlm {
    std::vector<KvlmEntry> headers{};
    std::vector<std::uint8_t> message;
};

// Bytes to KVLM (Key value List with message)
Kvlm kvlm_parse(std::span<const std::uint8_t> data);

// KVLM to bytes
std::vector<std::uint8_t> kvlm_serialize(const Kvlm& kvlm);