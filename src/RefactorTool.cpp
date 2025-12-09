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

static llvm::cl::OptionCategory ToolCategory("refactor-tool options");

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
    if (!Dtor) {
        return;
    }

    // Make sure it is in main file
    if (!SM.isInMainFile(Dtor->getLocation())) {
        return;
    }

    // Check that destructor is of base class that has derived classes


    //Find position for inserting virtual kw (before ~)
}

//todo: необходимо реализовать обработку случая отсутствие override
void RefactorHandler::handle_miss_override(const CXXMethodDecl *Method,
                            DiagnosticsEngine &Diag,
                            SourceManager &SM) {
    //Реализуйте Ваш код ниже
    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Объявлен метод"
        );
    Diag.Report(Method->getLocation(), DiagID);
}

//todo: необходимо реализовать обработку случая отсутствие & в range-for
void RefactorHandler::handle_crange_for(const VarDecl *LoopVar,
                                        DiagnosticsEngine &Diag,
                                        SourceManager &SM){
    // Реализуйте Ваш код ниже
    const unsigned DiagID = Diag.getCustomDiagID(
            DiagnosticsEngine::Remark,
            "Объявлена переменная"
        );
    Diag.Report(LoopVar->getLocation(), DiagID);
}

//todo: ниже необходимо реализовать матчеры для поиска узлов AST
//note: синтаксис написания матчеров точно такой же как и для использования clang-query
/*
    Пример того, как может выглядеть реализация:
    auto AllClassesMatcher()
    {
        return cxxRecordDecl().bind("classDecl");
    }
*/
internal::Matcher<Decl> NvDtorMatcher() {
    //Match any non-v destructor declaration
    return cxxDestructorDecl(
        isDefinition(),
        unless(isImplicit()),
        unless(isVirtual())
    ).bind(NON_VIRTUAL_DTOR_TAG);
}

internal::Matcher<Decl> IsBaseClassWithNvDtorMatcher() {
    return cxxRecordDecl(
        hasDirectBase(hasType(cxxRecordDecl(has(NvDtorMatcher()))))
    );
}

internal::Matcher<Decl> NoOverrideMatcher() {
    return cxxMethodDecl(
        isOverride(),
        unless(hasAttr(attr::Override))
    ).bind(MISSING_OVERRIDE_TAG);
}

internal::BindableMatcher<Stmt> NoRefConstVarInRangeLoopMatcher() {
    return cxxForRangeStmt(
      hasLoopVariable(
          varDecl(
            hasType(qualType(isConstQualified())),
            unless(hasType(referenceType())),
            unless(hasType(builtinType()))
          ).bind(NO_REF_IN_LOOP_TAG)
      )
    );

    // cxxForRangeStmt(
    //     hasLoopVariable(
    //         varDecl(
    //             hasType(
    //                 qualType(
    //                     isConstQualified(),
    //                     unless(referenceType()),
    //                     unless(builtinType())
    //                 )
    //             )).bind(NO_REF_IN_LOOP_TAG)
    //     )
    //  );
}

// Конструктор принимает Rewriter для изменения кода.
ComplexConsumer::ComplexConsumer(Rewriter &Rewrite) : Handler(Rewrite) {
    // Создаем MatchFinder и добавляем матчеры.
    Finder.addMatcher(NvDtorMatcher(), &Handler);
    Finder.addMatcher(NoOverrideMatcher(), &Handler);
    Finder.addMatcher(NoRefConstVarInRangeLoopMatcher(), &Handler);
}

// Метод HandleTranslationUnit вызывается для каждого файла.
void ComplexConsumer::HandleTranslationUnit(ASTContext &Context) {
    Finder.matchAST(Context);
}


std::unique_ptr<ASTConsumer> CodeRefactorAction::CreateASTConsumer(CompilerInstance &CI,
                                                StringRef file) {
    RewriterForCodeRefactor.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<ComplexConsumer>(
        RewriterForCodeRefactor);
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