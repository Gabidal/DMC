#ifndef _JSON_PARSER_H_
#define _JSON_PARSER_H_

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>

// Parses simple json file while keeping the chronic order of the commits, where the index of commit is the same as index at the time delta.

namespace jsonParser {
    
    class FastJsonParser {
    private:
        const char* data;
        size_t pos;
        size_t length;
        
        // Helper methods for parsing
        void skipWhitespace();
        bool match(char c);
        bool match(const std::string& str);
        std::string parseString();
        std::vector<std::string> parseStringArray();
        types::commit parseCommit();
        
    public:
        FastJsonParser(const std::string& jsonData);
        
        // Main parsing method
        std::vector<types::commit> parseCommitSummaries();
        
        // Static convenience method to parse from file
        static std::vector<types::commit> parseFromFile(const std::string& filepath);
    };
    
    // Utility functions
    namespace utils {
        std::string unescapeString(const std::string& str);
        bool isValidJson(const std::string& data);
        
        // Additional utility functions
        void writeCommitsToJson(const std::vector<types::commit>& commits, const std::string& filepath);
        std::vector<types::commit> filterCommitsByKeyword(const std::vector<types::commit>& commits, const std::string& keyword);
        void printCommitStatistics(const std::vector<types::commit>& commits);
        types::commit findCommitById(const std::vector<types::commit>& commits, const std::string& id);
    }

}

#endif