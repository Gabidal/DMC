#include "jsonParser.h"
#include "types.h"
#include <iostream>
#include <chrono>
#include <iomanip>

int main() {
    try {
        std::cout << "DMC - JSON Parser Demo\n";
        std::cout << "=====================\n\n";
        
        const std::string filepath = "test/data/commit_summaries.json";
        
        std::cout << "Parsing commit summaries from: " << filepath << "\n";
        
        // Measure parsing time
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<types::commit> commits = jsonParser::FastJsonParser::parseFromFile(filepath);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Parsed " << commits.size() << " commits in " << duration.count() << " ms\n\n";
        
        // Display statistics
        size_t totalHunks = 0;
        size_t totalDefinitions = 0;
        size_t totalKeyPoints = 0;
        
        for (const auto& commit : commits) {
            totalHunks += commit.hunkSummaries.size();
            totalDefinitions += commit.ctagDefinitions.size();
            totalKeyPoints += commit.regexDefinitions.size();
        }
        
        std::cout << "Statistics:\n";
        std::cout << "- Total commits: " << commits.size() << "\n";
        std::cout << "- Total hunk summaries: " << totalHunks << "\n";
        std::cout << "- Total ctag definitions: " << totalDefinitions << "\n";
        std::cout << "- Total regex definitions: " << totalKeyPoints << "\n\n";
        
        // Display first few commits as examples
        std::cout << "First 3 commits:\n";
        std::cout << std::string(50, '-') << "\n";
        
        for (size_t i = 0; i < std::min(size_t(3), commits.size()); ++i) {
            const auto& commit = commits[i];
            
            std::cout << "Commit " << (i + 1) << ":\n";
            std::cout << "  ID: " << commit.id.substr(0, 12) << "...\n";
            std::cout << "  Original Message: " << commit.originalMessage.substr(0, 80);
            if (commit.originalMessage.length() > 80) std::cout << "...";
            std::cout << "\n";
            
            std::cout << "  Hunk Summaries: " << commit.hunkSummaries.size() << " items\n";
            std::cout << "  New Message: " << commit.newMessage.substr(0, 80);
            if (commit.newMessage.length() > 80) std::cout << "...";
            std::cout << "\n";
            
            std::cout << "  CTag Definitions: " << commit.ctagDefinitions.size() << " items\n";
            std::cout << "  Regex Definitions: " << commit.regexDefinitions.size() << " items\n";
            
            if (i < 2) std::cout << "\n";
        }
        
        std::cout << std::string(50, '-') << "\n";
        std::cout << "JSON parsing completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}