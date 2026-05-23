#pragma once

#include <string>

namespace llvm {
class Module;
}

namespace codegen {

bool emitObjectFile(
    llvm::Module* module,
    const std::string& outputPath
);

}