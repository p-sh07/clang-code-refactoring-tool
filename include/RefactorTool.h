#pragma once
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <unordered_set>

class RefactorHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    explicit RefactorHandler(clang::Rewriter &Rewrite, const std::string &logPath)
        : Rewrite(Rewrite), log_file_name_(logPath) {
        if (!log_file_name_.empty()) {
            logFile.open(log_file_name_, std::ios::app);
            if (logFile.is_open()) {
                logFile << "=== Refactor Log (UTC: " << std::put_time(std::gmtime(nullptr), "%Y-%m-%d %H:%M:%S")
                          << ") ===\n";
                logEnabled = true;
            }
        }
    }
    // Метод run вызывается для каждого совпадения с матчем.
    // Мы проверяем тип совпадения по bind-именам и применяем рефакторинг.
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

    ~RefactorHandler() override {
        if (logEnabled && logFile.is_open()) {
            logFile << "=== End of Log ===\n";
            logFile.close();
        }
    }

private:
    void LogInsertion(clang::SourceManager &SM, clang::SourceLocation Loc, std::string text) {
        if (Loc.isInvalid())
            return;

        bool Invalid = false;
        std::string FileName = SM.getFilename(Loc).str();
        unsigned Line = SM.getSpellingLineNumber(Loc, &Invalid);
        unsigned Col = SM.getSpellingColumnNumber(Loc, &Invalid);

        if (Invalid)
            return;

        if (logEnabled && logFile.is_open()) {
            logFile << "[INSERT] " << FileName << ": " << Line << ":" << Col << " | " << '\'' << text << "\'\n";
        }
    }

    // 1. Невиртуальные деструкторы
    void handle_nv_dtor(const clang::CXXDestructorDecl *Dtor, clang::DiagnosticsEngine &Diag, clang::SourceManager &SM);

    // 2. Методы без override
    void handle_miss_override(const clang::CXXMethodDecl *Method, clang::DiagnosticsEngine &Diag,
                              clang::SourceManager &SM);

    // 3. range-for без &
    void handle_crange_for(const clang::VarDecl *LoopVar, clang::DiagnosticsEngine &Diag, clang::SourceManager &SM);

    clang::Rewriter &Rewrite;
    std::unordered_set<unsigned>
        virtualDtorLocations;  // Для хранения позиций деструкторов, к которым уже добавлен virtual

    bool logEnabled = false;
    std::string log_file_name_;
    std::ofstream logFile;
};

class ComplexConsumer : public clang::ASTConsumer {
public:
    // Конструктор принимает Rewriter для изменения кода.
    explicit ComplexConsumer(clang::Rewriter &Rewrite, const std::string& logPath);
    // Метод HandleTranslationUnit вызывается для каждого файла.
    void HandleTranslationUnit(clang::ASTContext &Context) override;

private:
    RefactorHandler Handler;                  // Обработчик матчеров.
    clang::ast_matchers::MatchFinder Finder;  // MatchFinder для поиска узлов AST.
};

class CodeRefactorAction : public clang::ASTFrontendAction {
public:
    explicit CodeRefactorAction(const std::string &logPath);

    // Returns our ASTConsumer per translation unit.
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
                                                                  clang::StringRef file) override;
    virtual bool BeginSourceFileAction(clang::CompilerInstance &CI) override;
    virtual void EndSourceFileAction() override;

private:
    std::string logPath;
    clang::Rewriter RewriterForCodeRefactor;
};

class CodeRefactorActionFactory : public clang::tooling::FrontendActionFactory {
public:
    explicit CodeRefactorActionFactory(const std::string& logPath)
        : logPath(logPath) {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<CodeRefactorAction>(logPath);
    }

private:
    std::string logPath;
};


//======== Matcher decls to use in tests directly =======
using std::literals::operator""s;
static const std::string NON_VIRTUAL_DTOR_TAG = "nonVDtor"s;
static const std::string MISSING_OVERRIDE_TAG = "missingOverride"s;
static const std::string NO_REF_IN_LOOP_TAG = "loopVar"s;

static llvm::cl::OptionCategory ToolCategory("refactor-tool options");

clang::ast_matchers::internal::Matcher<clang::Decl> NvDtorMatcher();
clang::ast_matchers::internal::Matcher<clang::Decl> IsBaseClassWithNvDtorMatcher();
clang::ast_matchers::internal::Matcher<clang::Decl> NoOverrideMatcher();
clang::ast_matchers::internal::BindableMatcher<clang::Stmt> NoRefConstVarInRangeLoopMatcher();