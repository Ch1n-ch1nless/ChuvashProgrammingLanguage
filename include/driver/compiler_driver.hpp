#pragma once

#include <string>

namespace driver {

class CompilerDriver {
public:
    bool compile(const std::string& source,
                 const std::string& output_name);
};

}