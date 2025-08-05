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
        types::json::type targetType;
        
        // Helper methods for parsing
        void skipWhitespace();
        bool match(char c);
        bool match(const std::string& str);
        std::string parseString();
        std::vector<std::string> parseStringArray();
        
        // Parsing methods for different types
        types::json::parsable* parseObject();
        types::hunk* parseHunk();
        types::commit* parseCommit();
        types::summary* parseSummary();
        
        // Helper method to skip unknown values
        void skipValue();
        
    public:
        FastJsonParser(const std::string& jsonData);  // Original constructor for backward compatibility
        FastJsonParser(const std::string& jsonData, types::json::type type);
        
        // Main parsing method
        std::vector<types::json::parsable*> parse();
        
        // Static convenience method to parse from file
        static std::vector<types::json::parsable*> parseFromFile(const std::string& filepath, types::json::type type);
    };
    
    // Utility functions
    namespace utils {
        std::string unescapeString(const std::string& str);
        bool isValidJson(const std::string& data);
        
        // Additional utility functions for summaries
        void writeCommitsToJson(const std::vector<types::summary>& commits, const std::string& filepath);
        std::vector<types::summary> filterCommitsByKeyword(const std::vector<types::summary>& commits, const std::string& keyword);
        void printCommitStatistics(const std::vector<types::summary>& commits);
        types::summary findCommitById(const std::vector<types::summary>& commits, const std::string& id);
        
        // Generic utility functions for parsable objects
        void writeParsablesToJson(const std::vector<types::json::parsable*>& objects, const std::string& filepath);
        std::vector<types::json::parsable*> filterParsablesByKeyword(const std::vector<types::json::parsable*>& objects, const std::string& keyword);
        void printParsableStatistics(const std::vector<types::json::parsable*>& objects);
    }

}

#endif