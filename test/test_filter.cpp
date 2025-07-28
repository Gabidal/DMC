#include "../src/filter.h"
#include "../src/types.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

namespace test_filter {

    void testBasicFiltering() {
        std::cout << "Testing basic filtering..." << std::endl;
        
        std::vector<std::string> testDefinitions = {
            "Guard::Shared",           // Should keep - valid class method
            "Add_Class",               // Should keep - valid function name
            "lambda[bffc4b000202]",    // Should keep - lambda expression
            "the",                     // Should filter - stop word
            "and",                     // Should filter - stop word
            "function",                // Should filter - programming keyword
            "template",                // Should filter - programming keyword
            "x",                       // Should filter - too short
            "tmp",                     // Should filter - noise word
            "123",                     // Should filter - purely numeric
            "MyClass",                 // Should keep - valid identifier
            "calculateSum",            // Should keep - valid identifier
            "CONSTANT_VALUE",          // Should keep - valid identifier
            "",                        // Should filter - empty
            "if",                      // Should filter - programming keyword
            "Report_Stack"             // Should keep - valid function name
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(testDefinitions);
        
        // Debug output
        std::cout << "Original count: " << testDefinitions.size() << std::endl;
        std::cout << "Filtered count: " << filtered.size() << std::endl;
        std::cout << "Filtered results:" << std::endl;
        for (const std::string& def : filtered) {
            std::cout << "  \"" << def << "\"" << std::endl;
        }
        
        // Expected to keep: Guard::Shared, Add_Class, lambda[bffc4b000202], MyClass, calculateSum, CONSTANT_VALUE, Report_Stack
        assert(filtered.size() == 7);
        
        // Check specific items are present
        bool hasGuardShared = false, hasAddClass = false, hasLambda = false, hasMyClass = false;
        for (const std::string& def : filtered) {
            if (def == "Guard::Shared") hasGuardShared = true;
            if (def == "Add_Class") hasAddClass = true;
            if (def == "lambda[bffc4b000202]") hasLambda = true;
            if (def == "MyClass") hasMyClass = true;
        }
        
        assert(hasGuardShared && hasAddClass && hasLambda && hasMyClass);
        
        std::cout << "✓ Basic filtering test passed!" << std::endl;
    }

    void testStopWords() {
        std::cout << "Testing stop word filtering..." << std::endl;
        
        std::vector<std::string> stopWordTests = {
            "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
            "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
            "to", "was", "will", "with", "or", "but", "not", "this", "they",
            "have", "had", "what", "when", "where", "who", "which", "why", "how"
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(stopWordTests);
        
        // All should be filtered out
        assert(filtered.empty());
        
        std::cout << "✓ Stop word filtering test passed!" << std::endl;
    }

    void testProgrammingKeywords() {
        std::cout << "Testing programming keyword filtering..." << std::endl;
        
        std::vector<std::string> keywordTests = {
            "class", "struct", "template", "function", "method", "variable",
            "int", "string", "vector", "bool", "void", "return", "if", "else",
            "for", "while", "switch", "case", "break", "continue"
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(keywordTests);
        
        // All should be filtered out
        assert(filtered.empty());
        
        std::cout << "✓ Programming keyword filtering test passed!" << std::endl;
    }

    void testValidIdentifiers() {
        std::cout << "Testing valid identifier preservation..." << std::endl;
        
        std::vector<std::string> validIdentifiers = {
            "MyClass",
            "calculateSum",
            "CONSTANT_VALUE",
            "m_memberVariable",
            "g_globalVar",
            "someFunction",
            "ClassName::methodName",
            "namespace::function",
            "operator++",
            "lambda[abc123def]",
            "_privateFunction",
            "__internalHelper",
            "CamelCaseFunction",
            "snake_case_function",
            "SCREAMING_SNAKE_CASE"
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(validIdentifiers);
        
        // Debug output
        std::cout << "Original count: " << validIdentifiers.size() << std::endl;
        std::cout << "Filtered count: " << filtered.size() << std::endl;
        
        // Show which ones got filtered
        for (const std::string& orig : validIdentifiers) {
            bool found = std::find(filtered.begin(), filtered.end(), orig) != filtered.end();
            if (!found) {
                std::cout << "FILTERED OUT: \"" << orig << "\"" << std::endl;
            }
        }
        
        // All should be preserved
        assert(filtered.size() == validIdentifiers.size());
        
        std::cout << "✓ Valid identifier preservation test passed!" << std::endl;
    }

    void testCommitFiltering() {
        std::cout << "Testing commit structure filtering..." << std::endl;
        
        types::commit testCommit;
        testCommit.id = "test123";
        testCommit.originalMessage = "Test commit";
        testCommit.newMessage = "Filtered test commit";
        
        testCommit.ctagDefinitions = {
            "Guard::Shared",        // Keep
            "Add_Class",            // Keep
            "the",                  // Filter
            "function",             // Filter
            "MyClass",              // Keep
            "template",             // Filter
            "calculateValue"        // Keep
        };
        
        testCommit.regexDefinitions = {
            "string",               // Filter
            "vector",               // Filter
            "CustomType",           // Keep
            "the",                  // Filter
            "processData",          // Keep
            "x",                    // Filter
            "IMPORTANT_CONSTANT"    // Keep
        };
        
        filter::DefinitionFilter::filterCommitDefinitions(testCommit);
        
        // Check ctag definitions - should have 4 remaining
        assert(testCommit.ctagDefinitions.size() == 4);
        
        // Check regex definitions - should have 3 remaining  
        assert(testCommit.regexDefinitions.size() == 3);
        
        std::cout << "✓ Commit filtering test passed!" << std::endl;
    }

    void testFilterStats() {
        std::cout << "Testing filter statistics..." << std::endl;
        
        std::vector<std::string> original = {
            "MyClass", "the", "function", "calculateSum", "and", "template", "CONSTANT"
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(original);
        
        filter::DefinitionFilter::FilterStats stats = 
            filter::DefinitionFilter::getFilterStats(original, filtered);
        
        assert(stats.totalWords == 7);
        assert(stats.remainingWords == 3);  // MyClass, calculateSum, CONSTANT
        assert(stats.filteredWords == 4);   // the, function, and, template
        assert(stats.filterRatio > 0.5 && stats.filterRatio < 0.6);  // ~0.57
        
        std::cout << "✓ Filter statistics test passed!" << std::endl;
    }

    void testEdgeCases() {
        std::cout << "Testing edge cases..." << std::endl;
        
        std::vector<std::string> edgeCases = {
            "",                     // Empty string
            " ",                    // Whitespace only
            "\t\n",                 // Tab and newline
            "123",                  // Pure numbers
            "a1b2c3",              // Mixed alphanumeric
            "___",                  // Only underscores
            "ABC123",               // All caps with numbers
            "camelCase123",         // Camel case with numbers
            "operator<<",           // Operator overload
            "lambda[1a2b3c4d]",    // Lambda with hex
            "std::vector",          // Namespace qualified
            "Class::~Destructor"    // Destructor
        };
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(edgeCases);
        
        // Debug output
        std::cout << "Original count: " << edgeCases.size() << std::endl;
        std::cout << "Filtered count: " << filtered.size() << std::endl;
        
        // Show what was kept and what was filtered
        for (const std::string& orig : edgeCases) {
            bool found = std::find(filtered.begin(), filtered.end(), orig) != filtered.end();
            if (found) {
                std::cout << "KEPT: \"" << orig << "\"" << std::endl;
            } else {
                std::cout << "FILTERED: \"" << orig << "\"" << std::endl;
            }
        }
        
        // Should keep: a1b2c3, ABC123, camelCase123, operator<<, lambda[1a2b3c4d], std::vector, Class::~Destructor
        // Should filter: "", " ", "\t\n", "123", "___"
        assert(filtered.size() == 7);
        
        std::cout << "✓ Edge cases test passed!" << std::endl;
    }

    void printFilterResults() {
        std::cout << "\n=== Filter Results Demo ===" << std::endl;
        
        std::vector<std::string> sampleDefinitions = {
            "Guard::Shared", "the", "Add_Class", "function", "template", 
            "MyRenderer", "and", "calculateBounds", "string", "ProcessEvent",
            "x", "WINDOW_WIDTH", "tmp", "lambda[abc123]", "if"
        };
        
        std::cout << "Original definitions (" << sampleDefinitions.size() << "):" << std::endl;
        for (const std::string& def : sampleDefinitions) {
            std::cout << "  \"" << def << "\"" << std::endl;
        }
        
        std::vector<std::string> filtered = filter::DefinitionFilter::filterDefinitions(sampleDefinitions);
        
        std::cout << "\nFiltered definitions (" << filtered.size() << "):" << std::endl;
        for (const std::string& def : filtered) {
            std::cout << "  \"" << def << "\"" << std::endl;
        }
        
        filter::DefinitionFilter::FilterStats stats = 
            filter::DefinitionFilter::getFilterStats(sampleDefinitions, filtered);
        
        std::cout << "\nFilter Statistics:" << std::endl;
        std::cout << "  Total words: " << stats.totalWords << std::endl;
        std::cout << "  Filtered words: " << stats.filteredWords << std::endl;
        std::cout << "  Remaining words: " << stats.remainingWords << std::endl;
        std::cout << "  Filter ratio: " << (stats.filterRatio * 100) << "%" << std::endl;
    }

} // namespace test_filter

int main() {
    std::cout << "=== DMC Filter Test Suite ===" << std::endl;
    std::cout << "Testing definition filtering functionality...\n" << std::endl;
    
    try {
        test_filter::testBasicFiltering();
        test_filter::testStopWords();
        test_filter::testProgrammingKeywords();
        test_filter::testValidIdentifiers();
        test_filter::testCommitFiltering();
        test_filter::testFilterStats();
        test_filter::testEdgeCases();
        
        std::cout << "\n=== All Tests Passed! ===" << std::endl;
        
        test_filter::printFilterResults();
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
    
    return 0;
}
