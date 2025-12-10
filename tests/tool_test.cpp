#include <filesystem>
#include <fstream>
#include <iostream>

// #include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

// Helper class to manage test lifecycle
class RefactorToolTest : public testing::TestWithParam<std::string> {
    const fs::path temp_dir_ = fs::temp_directory_path() / "refactor_test_temp";
    const fs::path test_files_path_ = fs::current_path().parent_path() / "tests" / "tests_data";
    const std::string expected_file_prefix = "expected_";

    // Helper: check if char is whitespace (space, tab, newline, etc.)
    static inline bool IsWhitespace(char c) { return std::isspace(static_cast<unsigned char>(c)); }

    // Normalize content: remove all whitespace characters
    static std::string RemoveWhitespace(const std::string &content) {
        std::string normalized;
        for (const char c : content) {
            if (!IsWhitespace(c)) {
                normalized += c;
            }
        }
        return normalized;
    }

    // Read file content
    static std::string ReadFile(const fs::path &path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path.string() << std::endl;
            return "";
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

protected:
    void SetUp() override {
        // Set up temp directory
        if (!fs::exists(temp_dir_)) {
            fs::create_directories(temp_dir_);
        }
    }

    void TearDown() override {
        if (fs::exists(temp_dir_)) {
            fs::remove_all(temp_dir_);
        }
    }

    std::pair<fs::path, fs::path> SetUpFile(const std::string &file_name) {
        const fs::path test_file = test_files_path_ / (file_name + ".cpp");
        const fs::path expected_result_file = test_files_path_ / (expected_file_prefix + file_name + ".cpp");
        const fs::path test_file_temp = temp_dir_ / (file_name + ".cpp");

        if (!fs::exists(test_file)) {
            std::cerr << "Unable to find file: " << test_file.string();
        }
        if (!fs::exists(expected_result_file)) {
            std::cerr << "Unable to find file: " << expected_result_file.string();
        }
            fs::copy_file(test_file, test_file_temp, fs::copy_options::overwrite_existing);
            return std::make_pair(test_file_temp, expected_result_file);
    }

    // Run refactor_tool on temp file
    bool RunRefactorTool(const fs::path &input_path) {
        std::string cmd = "./refactor_tool \"" + input_path.string() + "\"";

        int result = std::system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Failed to Run tool on file: " << input_path.string() << std::endl;
            return false;
        }
        return true;
    }

    bool CompareResultAndExpected(const fs::path &result, const fs::path &expected) {
        std::string result_normalized_str = RemoveWhitespace(ReadFile(result));
        std::string expected_normalized_str = RemoveWhitespace(ReadFile(expected));

        // DEBUG:
        if (result_normalized_str != expected_normalized_str) {
            std::cerr << "Files not equal! :\n"
                      << result_normalized_str << '\n'
                      << expected_normalized_str << std::endl;
        }

        return result_normalized_str == expected_normalized_str;
    }

    bool RunToolOnFileAndCheckResult(const std::string &file_name) {
        const auto [test_file, expected_result] = SetUpFile(file_name);

        if (!RunRefactorTool(test_file)) {
            return false;
        }
        return CompareResultAndExpected(test_file, expected_result);
    }
};

// Parameterized test: one per input file
TEST_P(RefactorToolTest, CheckCorrectRefactor) {
    EXPECT_TRUE(RunToolOnFileAndCheckResult(GetParam()));
}

// Generate test cases: one per .cpp file in tests/tests_data/
INSTANTIATE_TEST_SUITE_P(CheckCorrectRefactor, RefactorToolTest, ::testing::Values(
    "for_refactor.cpp"
));
