#include "wyag/kvlm.hpp"

#include <algorithm>
#include <span>
#include <string>

using Bytes = std::vector<std::uint8_t>;

// Byte constants
constexpr std::uint8_t SPACE{' '};
constexpr std::uint8_t NEWL{'\n'};

Kvlm kvlm_parse(std::span<const std::uint8_t> data) {
    Kvlm kvlm{};

    auto start_it = data.begin();
    auto end_it = data.end();

    while (true) {
        auto space_it = std::find(start_it, end_it, SPACE);
        auto newl_it = std::find(start_it, end_it, NEWL);

        // Base case
        if (space_it == data.end() || newl_it < space_it) {
            if (start_it != data.end() && *start_it == NEWL) start_it++;
            kvlm.message = Bytes{start_it, end_it};
            return kvlm;
        }

        std::string key{start_it, space_it};
        start_it = std::next(space_it);

        // Find value
        auto value_end_it = start_it;
        while (true) {
            value_end_it = std::find(value_end_it, end_it, NEWL);
            if (value_end_it == data.end() || *(++value_end_it) != SPACE) break;
        }

        // Fold away spaces in line-break values
        Bytes value{};
        value.reserve(value_end_it - start_it);
        for (auto itr{start_it}; itr < value_end_it; itr++) {
            value.push_back(*itr);
            if (*itr == NEWL && itr + 1 < value_end_it && *(itr + 1) == SPACE)
                itr++;
            if (itr + 1 == value_end_it) value.pop_back();
        }

        kvlm.headers.push_back(KvlmEntry{std::move(key), std::move(value)});
        start_it = value_end_it;
    }
}

Bytes kvlm_serialize(const Kvlm& kvlm) {
    Bytes result{};
    for (const auto& entry : kvlm.headers) {
        result.insert(result.end(), entry.key.begin(), entry.key.end());
        result.push_back(SPACE);
        for (const auto& b : entry.value) {
            result.push_back(b);
            if (b == NEWL) result.push_back(' ');
        }
        result.push_back(NEWL);
    }
    result.push_back(NEWL);
    result.insert(result.end(), kvlm.message.begin(), kvlm.message.end());
    return result;
}