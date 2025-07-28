#include "../src/jsonParser.h"
#include <iostream>
#include <chrono>
#include <algorithm>

void testBasicParsing() {
    std::cout << "=== Basic Parsing Test ===\n";
    
    try {
        auto start = std::chrono::high_resolution_clock::now();
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json");
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✓ Successfully parsed " << commits.size() << " commits\n";
        std::cout << "✓ Parsing time: " << duration.count() << " microseconds\n";
        std::cout << "✓ Average: " << (duration.count() / double(commits.size())) << " μs per commit\n\n";
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n\n";
    }
}

void testUtilityFunctions() {
    std::cout << "=== Utility Functions Test ===\n";
    
    try {
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json");
        
        // Test statistics
        std::cout << "Statistics:\n";
        jsonParser::utils::printCommitStatistics(commits);
        
        // Test filtering
        auto guardCommits = jsonParser::utils::filterCommitsByKeyword(commits, "Guard");
        std::cout << "Commits containing 'Guard': " << guardCommits.size() << "\n";
        
        auto classCommits = jsonParser::utils::filterCommitsByKeyword(commits, "class");
        std::cout << "Commits containing 'class': " << classCommits.size() << "\n\n";
        
        // Test finding by ID (use the first commit's ID)
        if (!commits.empty()) {
            auto foundCommit = jsonParser::utils::findCommitById(commits, commits[0].id);
            std::cout << "✓ Successfully found commit by ID: " << foundCommit.id.substr(0, 12) << "...\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void testErrorHandling() {
    std::cout << "=== Error Handling Test ===\n";
    
    // Test invalid file
    try {
        jsonParser::FastJsonParser::parseFromFile("nonexistent.json");
        std::cout << "✗ Should have thrown an exception\n";
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly handled missing file: " << e.what() << "\n";
    }
    
    // Test invalid JSON
    std::string invalidJson = "{invalid json}";
    if (!jsonParser::utils::isValidJson(invalidJson)) {
        std::cout << "✓ Correctly identified invalid JSON\n";
    } else {
        std::cout << "✗ Failed to identify invalid JSON\n";
    }
    
    // Test valid JSON
    std::string validJson = "[{\"id\":\"test\",\"message\":\"test\",\"summaries\":[],\"commit_summary\":\"test\",\"definitions\":[],\"key_points\":[]}]";
    if (jsonParser::utils::isValidJson(validJson)) {
        std::cout << "✓ Correctly identified valid JSON\n";
    } else {
        std::cout << "✗ Failed to identify valid JSON\n";
    }
    
    std::cout << "\n";
}

void testPerformance() {
    std::cout << "=== Performance Test ===\n";
    
    try {
        const int iterations = 10;
        std::vector<double> times;
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json");
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            times.push_back(duration.count());
        }
        
        // Calculate statistics
        double total = 0;
        double min_time = times[0];
        double max_time = times[0];
        
        for (double time : times) {
            total += time;
            if (time < min_time) min_time = time;
            if (time > max_time) max_time = time;
        }
        
        double avg_time = total / iterations;
        
        std::cout << "Performance over " << iterations << " iterations:\n";
        std::cout << "  Average: " << avg_time << " μs\n";
        std::cout << "  Minimum: " << min_time << " μs\n";
        std::cout << "  Maximum: " << max_time << " μs\n";
        std::cout << "  Std dev: ~" << (max_time - min_time) / 2 << " μs\n\n";
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n\n";
    }
}

void testDataIntegrity() {
    std::cout << "=== Data Integrity Test ===\n";
    
    try {
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json");
        
        // Check that we have the expected data structure
        bool hasCommitsWithData = false;
        int validCommits = 0;
        
        for (const auto& commit : commits) {
            if (!commit.id.empty()) {
                validCommits++;
            }
            
            if (!commit.hunkSummaries.empty() && !commit.ctagDefinitions.empty() && !commit.regexDefinitions.empty()) {
                hasCommitsWithData = true;
            }
        }
        
        std::cout << "✓ Total commits parsed: " << commits.size() << "\n";
        std::cout << "✓ Commits with valid IDs: " << validCommits << "\n";
        
        if (hasCommitsWithData) {
            std::cout << "✓ Found commits with comprehensive data (hunks, definitions, key points)\n";
        } else {
            std::cout << "! No commits found with all data types (this might be expected)\n";
        }
        
        // Verify specific known commit structure from the JSON we saw
        auto testCommit = std::find_if(commits.begin(), commits.end(),
            [](const types::commit& c) {
                return c.id == "fb35fd84097500ea2f14c6004a951d235466157a";
            });
        
        if (testCommit != commits.end()) {
            std::cout << "✓ Found specific test commit with ID fb35fd84...\n";
            std::cout << "  - Hunk summaries: " << testCommit->hunkSummaries.size() << "\n";
            std::cout << "  - CTag definitions: " << testCommit->ctagDefinitions.size() << "\n";
            std::cout << "  - Regex definitions: " << testCommit->regexDefinitions.size() << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "DMC JSON Parser - Comprehensive Test Suite\n";
    std::cout << "==========================================\n\n";
    
    testBasicParsing();
    testUtilityFunctions();
    testErrorHandling();
    testPerformance();
    testDataIntegrity();
    
    std::cout << "=== Test Suite Complete ===\n";
    std::cout << "All tests finished. Check output above for any failures.\n";
    
    return 0;
}
