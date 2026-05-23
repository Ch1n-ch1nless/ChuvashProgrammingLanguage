#include "driver/linker_driver.hpp"

#include <cstdlib>
#include <iostream>

namespace driver {

bool LinkerDriver::linkExecutable(
    const std::string& object_file,
    const std::string& output_file
) {
    std::string runtime_command =
        "as "
        "../runtime/runtime.s "
        "-o "
        "runtime.o";

    if (std::system(runtime_command.c_str()) != 0) {
        std::cerr << "Failed to assemble runtime.s\n";
        return false;
    }

    std::string command =
        "ld "
        "runtime.o "
        + object_file +
        " -o " +
        output_file;

    std::cout << "[Linker] " << command << "\n";

    return std::system(command.c_str()) == 0;
}

}