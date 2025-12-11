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
    const bool remove_temp_dir_after_test_ = false;
    const fs::path refactor_tool_path = fs::current_path() / "refactor_tool";
    const fs::path test_files_path_ = fs::current_path().parent_path() / "tests" / "tests_data";
    const fs::path temp_dir_ = test_files_path_.parent_path() / "refactor_test_temp";
    const std::string ref_file_suffix_ = "_ref";

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

    static fs::path AddSuffixToFilename(const fs::path& fp, std::string suffix) {
        std::string stem(fp.stem());
        std::string ext(fp.extension());

        return fp.parent_path() / (stem + suffix + ext);
    }

protected:
    void SetUp() override {
        std::cerr << "==== Starting Tool Test =====\n Settings:\n\t-temp_dir=[" << temp_dir_
            << "]\n\t-refactor_tool=[" << refactor_tool_path << "]\n";

        // Set up temp directory
        if (fs::exists(temp_dir_)) {
            std::cerr << "->removing previous test files" << std::endl;
            fs::remove_all(temp_dir_);
        } 
        fs::create_directories(temp_dir_);
        std::cerr << "->created temp_dir successfully" << std::endl;
    }

    void TearDown() override {
        if (remove_temp_dir_after_test_ && fs::exists(temp_dir_)) {
            fs::remove_all(temp_dir_);
        }
    }

    using TestFileRefFile = std::pair<fs::path, fs::path>;
    TestFileRefFile SetUpFile(const std::string &file_name) {
        const fs::path test_file = test_files_path_ / file_name;
        const fs::path ref_file = AddSuffixToFilename(test_file, ref_file_suffix_);
        const fs::path test_file_temp = temp_dir_ / file_name;

        if (!fs::exists(test_file)) {
            std::cerr << "Unable to find file: " << test_file.string();
        }
        if (!fs::exists(ref_file)) {
            std::cerr << "Unable to find file: " << ref_file.string();
        }
            fs::copy_file(test_file, test_file_temp, fs::copy_options::overwrite_existing);
            return std::make_pair(test_file_temp, ref_file);
    }

    // Run refactor_tool on temp file
    bool RunRefactorTool(const fs::path &input_path) {
#ifdef __APPLE__
        std::string cmd = refactor_tool_path.string() + " --extra-arg=-isysroot \"$SDKROOT\" --extra-arg=-I\"$SDKROOT/usr/include\" \"" + input_path.string() + "\"";
        //DEBUG:
        std::cerr << "******* running with command: " << cmd << std::endl << std::endl;
#else

        std::string cmd = refactor_tool_path.string() + " \"" + input_path.string() + "\"";
#endif
        int result = std::system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Failed to Run tool on file: " << input_path.string() << std::endl;
            return false;
        }
        return true;
    }

    bool CompareResultAndExpected(const fs::path &result, const fs::path &ref) {
        std::string result_nowp_str = RemoveWhitespace(ReadFile(result));
        std::string ref_nowp_str = RemoveWhitespace(ReadFile(ref));

        // DEBUG:
        if (result_nowp_str != ref_nowp_str) {
            std::cerr << "Files not equal! :\n"
                      << result_nowp_str << '\n'
                      << ref_nowp_str << std::endl;
        }

        return result_nowp_str == ref_nowp_str;
    }

    bool RunToolOnFileAndCheckResult(const std::string &file_name) {
        const auto [test_file, expected_ref] = SetUpFile(file_name);

        if (!RunRefactorTool(test_file)) {
            return false;
        }
        return CompareResultAndExpected(test_file, expected_ref);
    }
};

// Parameterized test: one per input file
TEST_P(RefactorToolTest, CheckCorrectRefactor) {
    EXPECT_TRUE(RunToolOnFileAndCheckResult(GetParam()));
}

// Generate test cases: one per .cpp file in tests/tests_data/
INSTANTIATE_TEST_SUITE_P(CheckCorrectRefactor, RefactorToolTest, ::testing::Values(
    "for_refactor.cpp",
    "test1.cpp",
    "test2.cpp",
    "test3.cpp"
));
