#include "codegen/object_emitter.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <system_error>

#include <llvm/IR/LegacyPassManager.h>

#include <llvm/MC/TargetRegistry.h>

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <llvm/TargetParser/Host.h>

namespace codegen {

bool emitObjectFile(
    llvm::Module* module,
    const std::string& outputPath
) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto targetTriple =
        llvm::sys::getDefaultTargetTriple();

    module->setTargetTriple(targetTriple);

    std::string error;

    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(
            targetTriple,
            error
        );

    if (!target) {
        std::cerr << error << "\n";
        return false;
    }

    llvm::TargetOptions options;

    auto relocModel =
        std::optional<llvm::Reloc::Model>();

    auto targetMachine =
        std::unique_ptr<llvm::TargetMachine>(
            target->createTargetMachine(
                targetTriple,
                "generic",
                "",
                options,
                relocModel
            )
        );

    module->setDataLayout(
        targetMachine->createDataLayout()
    );

    std::error_code ec;

    llvm::raw_fd_ostream dest(
        outputPath,
        ec,
        llvm::sys::fs::OF_None
    );

    if (ec) {
        std::cerr << ec.message() << "\n";
        return false;
    }

    llvm::legacy::PassManager pass;

    if (targetMachine->addPassesToEmitFile(
            pass,
            dest,
            nullptr,
            llvm::CodeGenFileType::ObjectFile
        )) {

        std::cerr << "Cannot emit object file\n";
        return false;
    }

    pass.run(*module);

    dest.flush();

    return true;
}

}