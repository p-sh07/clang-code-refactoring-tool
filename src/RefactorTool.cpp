#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/CommandLine.h"

#include <unordered_set>

#include "RefactorTool.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

// Метод run вызывается для каждого совпадения с матчем. 
// Мы проверяем тип совпадения по bind-именам и применяем рефакторинг.
void RefactorHandler::run(const MatchFinder::MatchResult &Result) {
    auto& Diag = Result.Context->getDiagnostics();
    auto& SM = *Result.SourceManager; // Получаем SourceManager для проверки isInMainFile
    
    if (const auto *Dtor = Result.Nodes.getNodeAs<CXXDestructorDecl>(NON_VIRTUAL_DTOR_TAG)) {
        handle_nv_dtor(Dtor, Diag, SM);
    }

    if (const auto *Method = Result.Nodes.getNodeAs<CXXMethodDecl>(MISSING_OVERRIDE_TAG);
        Method && Method->size_overridden_methods() > 0 && !Method->hasAttr<OverrideAttr>()) {
        handle_miss_override(Method, Diag, SM);
    }

    if (const auto *LoopVar = Result.Nodes.getNodeAs<VarDecl>(NO_REF_IN_LOOP_TAG)) {
        handle_crange_for(LoopVar, Diag, SM);
    }
}

void RefactorHandler::handle_nv_dtor(const CXXDestructorDecl* Dtor, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!Dtor || SM.isInMainFile(Dtor->getLocation())) {
        return;
    }
    auto DtorLoc = Dtor->getLocation();

    // Avoid processing the same destructor multiple times
    unsigned locHash = DtorLoc.getRawEncoding();
    if (virtualDtorLocations.count(locHash)) {
        return;
    }
    virtualDtorLocations.insert(locHash);

    // Insert "virtual " before the destructor
    std::string text = "virtual ";
    Rewrite.InsertTextBefore(DtorLoc, text);
    LogInsertion(SM, DtorLoc, text);

    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Деструктор изменен на виртуальный -> virtual"
        );
    Diag.Report(Dtor->getLocation(), DiagID);
}

void RefactorHandler::handle_miss_override(const CXXMethodDecl* Method, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!Method || !SM.isInMainFile(Method->getLocation())) {
        return;
    }

    // Get the location after the function declarator
    SourceLocation insertLoc = Lexer::findLocationAfterToken(
        Method->getEndLoc(),
        tok::r_paren,
        SM,
        Method->getASTContext().getLangOpts(),
        false
    );

    if (insertLoc.isValid()) {
        // Insert " override" after the closing parenthesis
        std::string text = " override";
        Rewrite.InsertTextAfter(insertLoc, " override");
        LogInsertion(SM, insertLoc, text);

        const unsigned DiagID = Diag.getCustomDiagID(
                DiagnosticsEngine::Remark,
                "К реализации метода добавлен -> override"
            );
        Diag.Report(Method->getLocation(), DiagID);
    }
}

void RefactorHandler::handle_crange_for(const VarDecl* LoopVar, DiagnosticsEngine &Diag, SourceManager &SM) {
    if (!LoopVar || !SM.isInMainFile(LoopVar->getLocation())) {
        return;
    }

    QualType varType = LoopVar->getType();

    // Skip if already a reference
    if (varType->isReferenceType()) {
        return;
    }

    // Get the end of the type
    TypeSourceInfo *TSI = LoopVar->getTypeSourceInfo();
    if (!TSI) {
        return;
    }

    SourceLocation typeEndLoc = TSI->getTypeLoc().getEndLoc();

    // Find the actual end of the type in source (after any spaces/qualifiers)
    SourceLocation insertLoc = Lexer::getLocForEndOfToken(
        typeEndLoc, 0, SM, LoopVar->getASTContext().getLangOpts()
    );

    if (insertLoc.isValid()) {
        std::string text = "&";
        Rewrite.InsertTextAfter(insertLoc, text);
        LogInsertion(SM, insertLoc, text);

        const unsigned DiagID = Diag.getCustomDiagID(
                DiagnosticsEngine::Remark,
                "Переменная изменена на ref -> &"
            );
        Diag.Report(LoopVar->getLocation(), DiagID);
    }
}

internal::Matcher<Decl> NvDtorMatcher() {
    //Match any non-v destructor declaration
    return cxxDestructorDecl(
        isExpansionInMainFile(),
        isDefinition(),
        unless(isImplicit()),
        unless(isVirtual())
    ).bind(NON_VIRTUAL_DTOR_TAG);
}

internal::Matcher<Decl> IsBaseClassWithNvDtorMatcher() {
    return cxxRecordDecl(
        isExpansionInMainFile(),
        hasDirectBase(hasType(cxxRecordDecl(has(NvDtorMatcher()))))
    );
}

internal::Matcher<Decl> NoOverrideMatcher() {
    return cxxMethodDecl(
        isExpansionInMainFile(),
        isOverride(),
        unless(hasAttr(attr::Override)),
        unless(cxxDestructorDecl())
    ).bind(MISSING_OVERRIDE_TAG);
}

internal::BindableMatcher<Stmt> NoRefConstVarInRangeLoopMatcher() {
//return cxxForRangeStmt(hasLoopVariable(varDecl(hasType(isConstQualified())).bind(NO_REF_IN_LOOP_TAG)));
    return cxxForRangeStmt(
      hasLoopVariable(
          varDecl(
            hasType(qualType(isConstQualified())),
            unless(hasType(referenceType())),
            unless(hasType(builtinType()))
          ).bind(NO_REF_IN_LOOP_TAG)
      )
    );
}

// Конструктор принимает Rewriter для изменения кода.
ComplexConsumer::ComplexConsumer(Rewriter &Rewrite, const std::string& logPath)
: Handler(Rewrite, logPath) {
    // Создаем MatchFinder и добавляем матчеры.
    Finder.addMatcher(NvDtorMatcher(), &Handler);
    Finder.addMatcher(NoOverrideMatcher(), &Handler);
    Finder.addMatcher(NoRefConstVarInRangeLoopMatcher(), &Handler);
}

// Метод HandleTranslationUnit вызывается для каждого файла.
void ComplexConsumer::HandleTranslationUnit(ASTContext &Context) { Finder.matchAST(Context); }

CodeRefactorAction::CodeRefactorAction(const std::string &logPath) : logPath(logPath) {}
std::unique_ptr<ASTConsumer> CodeRefactorAction::CreateASTConsumer(CompilerInstance &CI,
                                                StringRef file) {
    RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<ComplexConsumer>(
        RewriterForCodeRefactor, logPath);
}

bool CodeRefactorAction::BeginSourceFileAction( CompilerInstance &CI) {
// Инициализируем Rewriter для рефакторинга.
RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(),
                                        CI.getLangOpts());
    return true;  // Возвращаем true, чтобы продолжить обработку файла.
}

void CodeRefactorAction::EndSourceFileAction() {
    // Применяем изменения в файле.
    if (RewriterForCodeRefactor.overwriteChangedFiles()) {
        llvm::errs() << "Error applying changes to files.\n";
    }
}