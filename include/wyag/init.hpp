#pragma once

#include <string>

namespace wyag::commands {

struct InitOptions {
    std::string path{"."};
};

int run_init(const InitOptions& opts);

}  // namespace wyag::commands