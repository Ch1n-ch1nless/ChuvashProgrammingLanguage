#pragma once

#include <string>

namespace driver {

class LinkerDriver {
public:
    static bool linkExecutable(
        const std::string& object_file,
        const std::string& output_file
    );
};

}