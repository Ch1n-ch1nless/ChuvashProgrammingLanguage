#include "driver/compiler_driver.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: compiler.out <file>\n";
        return 1;
    }

    std::ifstream file(argv[1]);

    if (!file.is_open()) {
        std::cerr << "Cannot open file\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    driver::CompilerDriver driver;

    if (!driver.compile(
            buffer.str(),
            "program.out"
        )) {
        return 1;
    }

    std::cout << "Compilation successful\n";

    return 0;
}