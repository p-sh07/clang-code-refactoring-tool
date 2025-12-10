#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <print>
#include <string>

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Tooling.h"
#include "RefactorTool.h"

namespace fs = std::filesystem;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

//====== Matcher Test Suite ========
class MatcherTest : public testing::TestWithParam<std::tuple<std::string, std::string, int>> {
protected:
    int MatchAndGetCount(const std::string& code, std::string node_tag) {
        CounterCallback<CXXDestructorDecl> callback(node_tag);
        MatchFinder finder;

        //TODO: is there a better way?
        if (node_tag == NON_VIRTUAL_DTOR_TAG) {
            finder.addMatcher(IsBaseClassWithNvDtorMatcher(), &callback);
        }
        else if (node_tag == MISSING_OVERRIDE_TAG) {
            finder.addMatcher(NoOverrideMatcher(), &callback);
        }
        else if (node_tag == NO_REF_IN_LOOP_TAG) {
            finder.addMatcher(NoRefConstVarInRangeLoopMatcher(), &callback);
        }

        auto factory = newFrontendActionFactory(&finder);
        runToolOnCode(factory->create(), code);

        return callback.get_count();
    }

private:
    template <typename NodeType>
    class CounterCallback : public MatchFinder::MatchCallback {
    public:
        explicit CounterCallback(std::string tag) : match_tag_(tag) {}
        void run(const MatchFinder::MatchResult &Result) override {
            if (Result.Nodes.getNodeAs<NodeType>(match_tag_)) {
                count_++;
            }
        }
        int get_count() const { return count_; }
    private:
        int count_ = 0;
        std::string match_tag_;
    };
};

TEST_P(MatcherTest, CheckCorrectASTMatching) {
    const auto [code, node_tag, count] = GetParam();
        EXPECT_EQ(count, MatchAndGetCount(code, node_tag));
}

INSTANTIATE_TEST_SUITE_P(
    CheckCorrectASTMatching, MatcherTest, ::testing::Values(
        //Virtual destructor
        std::make_tuple(R"(
        class Base {
        public:
            ~Base() {}
        };
        class Derived : public Base {
        public:
            ~Derived() {}
        }; )", NON_VIRTUAL_DTOR_TAG, 1),

        std::make_tuple(R"(
        class Base {
        public:
            virtual ~Base() {}
        };
        class Derived : public Base {
        public:
            ~Derived() {}
        }; )", NON_VIRTUAL_DTOR_TAG, 0),

        //Missing Override
        std::make_tuple(R"(
        class Base {
        public:
            virtual void method1() {}
            virtual void method2() {}
            virtual ~Base() {}
        };
        class Derived : public Base {
        public:
            void method1() override {}
            void method2() {}
        };)", MISSING_OVERRIDE_TAG, 1),

        std::make_tuple(R"(
        class Base {
        public:
            virtual void method() {}
            virtual ~Base() {}
        };
        class Derived : public Base {
        public:
            void method() override {}
        };)", MISSING_OVERRIDE_TAG, 0),

        //No ref
        std::make_tuple(R"(
        struct MyObject {
            int data[100];
        };
        struct Container {
            MyObject* begin() { return items; }
            MyObject* end() { return items + 10; }
            MyObject items[10];
        };
        void process() {
            Container objects1;
            Container objects2;

            for (const MyObject s2 : objects2) {
            }
        })", NO_REF_IN_LOOP_TAG, 1),

        std::make_tuple(R"(
        struct MyObject {
           int data[100];
        };
        struct Container {
           MyObject* begin() { return items; }
           MyObject* end() { return items + 10; }
           MyObject items[10];
       };
       void process() {
           Container objects;
           for (const MyObject& obj : objects) {
           }
       })", NO_REF_IN_LOOP_TAG, 0)
));