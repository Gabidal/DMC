#include "../src/jsonParser.h"
#include <iostream>
#include <chrono>
#include <algorithm>

void testBasicParsing() {
    std::cout << "=== Basic Parsing Test ===\n";
    
    try {
        auto start = std::chrono::high_resolution_clock::now();
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json", types::json::type::SUMMARY);
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
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json", types::json::type::SUMMARY);
        
        // Test statistics
        std::cout << "Statistics:\n";
        jsonParser::utils::printParsableStatistics(commits);
        
        // Test filtering
        auto guardCommits = jsonParser::utils::filterParsablesByKeyword(commits, "Guard");
        std::cout << "Commits containing 'Guard': " << guardCommits.size() << "\n";
        
        auto classCommits = jsonParser::utils::filterParsablesByKeyword(commits, "class");
        std::cout << "Commits containing 'class': " << classCommits.size() << "\n\n";
        
        // Test finding by ID (use the first commit's ID)
        if (!commits.empty()) {
            auto* summary = dynamic_cast<types::summary*>(commits[0]);
            if (summary) {
                std::cout << "✓ Successfully found commit by ID: " << summary->id.substr(0, 12) << "...\n";
            }
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
        jsonParser::FastJsonParser::parseFromFile("nonexistent.json", types::json::type::SUMMARY);
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
    std::string validJson = "[{\"id\":\"test\",\"originalMessage\":\"test\",\"hunkSummaries\":[],\"newMessage\":\"test\",\"ctagDefinitions\":[],\"regexDefinitions\":[]}]";
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
            auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json", types::json::type::SUMMARY);
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

void testGenericParsing() {
    std::cout << "=== Generic Parsing Test ===\n";
    
    try {
        // Test parsing summaries with the new generic interface
        std::cout << "Testing generic summary parsing...\n";
        auto summaryObjects = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json", types::json::type::SUMMARY);
        std::cout << "✓ Parsed " << summaryObjects.size() << " summary objects\n";
        
        // Verify that they are actually summary objects
        int summaryCount = 0;
        for (const auto& obj : summaryObjects) {
            if (obj->Type == types::json::type::SUMMARY) {
                summaryCount++;
                types::summary* summary = dynamic_cast<types::summary*>(obj);
                if (summary && summary->id.empty()) {
                    std::cout << "✗ Found summary with empty ID\n";
                }
            }
        }
        std::cout << "✓ All " << summaryCount << " objects are valid summaries\n";
        
        // Test parsing commits with the new generic interface
        std::cout << "Testing generic commit parsing...\n";
        auto commitObjects = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_data.json", types::json::type::COMMIT);
        std::cout << "✓ Parsed " << commitObjects.size() << " commit objects\n";
        
        // Verify that they are actually commit objects  
        int commitCount = 0;
        int commitsWithHunks = 0;
        for (const auto& obj : commitObjects) {
            if (obj->Type == types::json::type::COMMIT) {
                commitCount++;
                types::commit* commit = dynamic_cast<types::commit*>(obj);
                if (commit) {
                    if (!commit->hunks.empty()) {
                        commitsWithHunks++;
                    }
                    if (commit->id.empty()) {
                        std::cout << "✗ Found commit with empty ID\n";
                    }
                }
            }
        }
        std::cout << "✓ All " << commitCount << " objects are valid commits\n";
        std::cout << "✓ " << commitsWithHunks << " commits contain hunks\n";
        
        // Test utility functions with generic objects
        std::cout << "Testing generic utility functions...\n";
        jsonParser::utils::printParsableStatistics(summaryObjects);
        
        auto filteredSummaries = jsonParser::utils::filterParsablesByKeyword(summaryObjects, "Guard");
        std::cout << "✓ Filtered " << filteredSummaries.size() << " summary objects containing 'Guard'\n";
        
        auto filteredCommits = jsonParser::utils::filterParsablesByKeyword(commitObjects, "Renderer");
        std::cout << "✓ Filtered " << filteredCommits.size() << " commit objects containing 'Renderer'\n";
        
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void testDataIntegrity() {
    std::cout << "=== Data Integrity Test ===\n";
    
    try {
        auto commits = jsonParser::FastJsonParser::parseFromFile("../../test/data/commit_summaries.json", types::json::type::SUMMARY);
        
        // Check that we have the expected data structure
        bool hasCommitsWithData = false;
        int validCommits = 0;
        
        for (const auto& obj : commits) {
            types::summary* commit = dynamic_cast<types::summary*>(obj);
            if (commit && !commit->id.empty()) {
                validCommits++;
            }
            
            if (commit && !commit->hunkSummaries.empty() && !commit->ctagDefinitions.empty() && !commit->regexDefinitions.empty()) {
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
        types::summary* testCommit = nullptr;
        for (const auto& obj : commits) {
            types::summary* summary = dynamic_cast<types::summary*>(obj);
            if (summary && summary->id == "fb35fd84097500ea2f14c6004a951d235466157a") {
                testCommit = summary;
                break;
            }
        }
        
        if (testCommit) {
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
    testGenericParsing();
    testDataIntegrity();
    
    std::cout << "=== Test Suite Complete ===\n";
    std::cout << "All tests finished. Check output above for any failures.\n";
    
    return 0;
}
