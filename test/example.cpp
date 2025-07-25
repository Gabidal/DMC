#include "../src/jsonParser.h"
#include <iostream>
#include <algorithm>
#include <fstream>

void printCommitDetails(const types::commit& commit, int index) {
    std::cout << "=== Commit " << index << " ===\n";
    std::cout << "ID: " << commit.id << "\n";
    std::cout << "Original Message: \"" << commit.originalMessage << "\"\n";
    std::cout << "New Message: \"" << commit.newMessage << "\"\n";
    
    std::cout << "\nHunk Summaries (" << commit.hunkSummaries.size() << "):\n";
    for (size_t i = 0; i < commit.hunkSummaries.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << commit.hunkSummaries[i] << "\n";
    }
    
    std::cout << "\nCTag Definitions (" << commit.ctagDefinitions.size() << "):\n";
    for (size_t i = 0; i < commit.ctagDefinitions.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << commit.ctagDefinitions[i] << "\n";
    }
    
    std::cout << "\nRegex Definitions (" << commit.regexDefinitions.size() << "):\n";
    for (size_t i = 0; i < commit.regexDefinitions.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << commit.regexDefinitions[i] << "\n";
    }
    
    std::cout << "\n" << std::string(60, '-') << "\n\n";
}

void analyzeCommits(const std::vector<types::commit>& commits) {
    std::cout << "=== Commit Analysis ===\n";
    
    // Find commits with most hunk summaries
    auto maxHunks = std::max_element(commits.begin(), commits.end(),
        [](const types::commit& a, const types::commit& b) {
            return a.hunkSummaries.size() < b.hunkSummaries.size();
        });
    
    // Find commits with most definitions
    auto maxDefs = std::max_element(commits.begin(), commits.end(),
        [](const types::commit& a, const types::commit& b) {
            return a.ctagDefinitions.size() < b.ctagDefinitions.size();
        });
    
    // Find commits with most key points
    auto maxKeys = std::max_element(commits.begin(), commits.end(),
        [](const types::commit& a, const types::commit& b) {
            return a.regexDefinitions.size() < b.regexDefinitions.size();
        });
    
    std::cout << "Commit with most hunk summaries: " << maxHunks->id.substr(0, 12) 
              << " (" << maxHunks->hunkSummaries.size() << " hunks)\n";
    std::cout << "Commit with most definitions: " << maxDefs->id.substr(0, 12) 
              << " (" << maxDefs->ctagDefinitions.size() << " definitions)\n";
    std::cout << "Commit with most key points: " << maxKeys->id.substr(0, 12) 
              << " (" << maxKeys->regexDefinitions.size() << " key points)\n\n";
}

void searchCommits(const std::vector<types::commit>& commits, const std::string& query) {
    std::cout << "=== Search Results for: \"" << query << "\" ===\n";
    
    int found = 0;
    for (size_t i = 0; i < commits.size(); ++i) {
        const auto& commit = commits[i];
        bool matches = false;
        
        // Search in original message
        if (commit.originalMessage.find(query) != std::string::npos) {
            matches = true;
        }
        
        // Search in new message
        if (commit.newMessage.find(query) != std::string::npos) {
            matches = true;
        }
        
        // Search in definitions
        for (const auto& def : commit.ctagDefinitions) {
            if (def.find(query) != std::string::npos) {
                matches = true;
                break;
            }
        }
        
        // Search in key points
        for (const auto& key : commit.regexDefinitions) {
            if (key.find(query) != std::string::npos) {
                matches = true;
                break;
            }
        }
        
        if (matches) {
            std::cout << "Found in commit " << (i + 1) << ": " << commit.id.substr(0, 12) 
                      << " - " << commit.originalMessage.substr(0, 50) << "...\n";
            found++;
        }
    }
    
    std::cout << "Total matches: " << found << "\n\n";
}

int main() {
    try {
        std::cout << "DMC - Advanced JSON Parser Demo\n";
        std::cout << "===============================\n\n";
        
        const std::string filepath = "test/data/commit_summaries.json";
        
        // Parse commits
        auto commits = jsonParser::FastJsonParser::parseFromFile(filepath);
        std::cout << "Successfully parsed " << commits.size() << " commits\n\n";
        
        // Show detailed analysis
        analyzeCommits(commits);
        
        // Show a detailed commit (find one with content)
        auto detailedCommit = std::find_if(commits.begin(), commits.end(),
            [](const types::commit& c) {
                return !c.hunkSummaries.empty() && !c.ctagDefinitions.empty();
            });
        
        if (detailedCommit != commits.end()) {
            int index = std::distance(commits.begin(), detailedCommit) + 1;
            printCommitDetails(*detailedCommit, index);
        }
        
        // Search functionality demo
        searchCommits(commits, "class");
        searchCommits(commits, "Guard");
        
        std::cout << "Demo completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
