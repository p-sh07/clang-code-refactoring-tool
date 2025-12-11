//
// Created by Pavel on 09.12.2025.
//
#include "RefactorTool.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

int main(int argc, const char **argv) {
    // Add log file option
    static llvm::cl::opt<std::string> logFile(
        "log-file",
        llvm::cl::desc("Path to log file for recording changes"),
        llvm::cl::value_desc("filename"),
        llvm::cl::init("")
    );

    // Парсер опций: Обрабатывает флаги командной строки, компиляционные базы данных.
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    // Создаем ClangTool
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    // Запускаем RefactorAction.
    auto factory = std::make_unique<CodeRefactorActionFactory>(logFile);
    return Tool.run(factory.get());
}