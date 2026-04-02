#include <iostream>
#include <string>

#include "CLI11.hpp"
#include "wyag/init.hpp"

int main(int argc, char** argv) {
    CLI::App app{"wyag: A C++ implementation of a subset of git"};

    bool verbose{false};
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");

    CLI11_PARSE(app, argc, argv);
    if (verbose) {
        std::cout << "Verbose mode ENABLED\n";
    }
    return 0;
}