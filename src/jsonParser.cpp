#include "jsonParser.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace jsonParser {

FastJsonParser::FastJsonParser(const std::string& jsonData) 
    : data(jsonData.c_str()), pos(0), length(jsonData.length()) {}

void FastJsonParser::skipWhitespace() {
    while (pos < length && std::isspace(data[pos])) {
        pos++;
    }
}

bool FastJsonParser::match(char c) {
    skipWhitespace();
    if (pos < length && data[pos] == c) {
        pos++;
        return true;
    }
    return false;
}

bool FastJsonParser::match(const std::string& str) {
    skipWhitespace();
    if (pos + str.length() <= length) {
        if (std::string(data + pos, str.length()) == str) {
            pos += str.length();
            return true;
        }
    }
    return false;
}

std::string FastJsonParser::parseString() {
    skipWhitespace();
    
    if (pos >= length || data[pos] != '"') {
        throw std::runtime_error("Expected '\"' at position " + std::to_string(pos));
    }
    
    pos++; // Skip opening quote
    size_t start = pos;
    
    while (pos < length) {
        if (data[pos] == '"' && (pos == start || data[pos-1] != '\\')) {
            std::string result(data + start, pos - start);
            pos++; // Skip closing quote
            return utils::unescapeString(result);
        }
        pos++;
    }
    
    throw std::runtime_error("Unterminated string");
}

std::vector<std::string> FastJsonParser::parseStringArray() {
    std::vector<std::string> result;
    
    skipWhitespace();
    if (!match('[')) {
        throw std::runtime_error("Expected '[' for array");
    }
    
    skipWhitespace();
    if (match(']')) {
        return result; // Empty array
    }
    
    do {
        result.push_back(parseString());
        skipWhitespace();
    } while (match(','));
    
    if (!match(']')) {
        throw std::runtime_error("Expected ']' to close array");
    }
    
    return result;
}

types::commit FastJsonParser::parseCommit() {
    types::commit commit;
    
    if (!match('{')) {
        throw std::runtime_error("Expected '{' for commit object");
    }
    
    bool first = true;
    while (true) {
        skipWhitespace();
        if (match('}')) {
            break;
        }
        
        if (!first && !match(',')) {
            throw std::runtime_error("Expected ',' between object members");
        }
        first = false;
        
        std::string key = parseString();
        
        if (!match(':')) {
            throw std::runtime_error("Expected ':' after key");
        }
        
        if (key == "id") {
            commit.id = parseString();
        } else if (key == "message") {
            commit.originalMessage = parseString();
        } else if (key == "summaries") {
            commit.hunkSummaries = parseStringArray();
        } else if (key == "commit_summary") {
            commit.newMessage = parseString();
        } else if (key == "definitions") {
            commit.ctagDefinitions = parseStringArray();
        } else if (key == "key_points") {
            commit.regexDefinitions = parseStringArray();
        } else {
            // Skip unknown fields
            skipWhitespace();
            if (data[pos] == '"') {
                parseString(); // Skip string value
            } else if (data[pos] == '[') {
                parseStringArray(); // Skip array value
            } else if (data[pos] == '{') {
                // Skip object value - simple implementation
                int braceCount = 0;
                do {
                    if (data[pos] == '{') braceCount++;
                    else if (data[pos] == '}') braceCount--;
                    pos++;
                } while (pos < length && braceCount > 0);
            } else {
                // Skip other values (numbers, booleans, null)
                while (pos < length && data[pos] != ',' && data[pos] != '}') {
                    pos++;
                }
            }
        }
    }
    
    return commit;
}

std::vector<types::commit> FastJsonParser::parseCommitSummaries() {
    std::vector<types::commit> commits;
    
    skipWhitespace();
    if (!match('[')) {
        throw std::runtime_error("Expected '[' for top-level array");
    }
    
    skipWhitespace();
    if (match(']')) {
        return commits; // Empty array
    }
    
    do {
        commits.push_back(parseCommit());
        skipWhitespace();
    } while (match(','));
    
    if (!match(']')) {
        throw std::runtime_error("Expected ']' to close top-level array");
    }
    
    return commits;
}

std::vector<types::commit> FastJsonParser::parseFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    // Read entire file into string for fast parsing
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    if (content.empty()) {
        throw std::runtime_error("File is empty: " + filepath);
    }
    
    FastJsonParser parser(content);
    return parser.parseCommitSummaries();
}

namespace utils {

std::string unescapeString(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':  result += '"'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '/':  result += '/'; i++; break;
                case 'b':  result += '\b'; i++; break;
                case 'f':  result += '\f'; i++; break;
                case 'n':  result += '\n'; i++; break;
                case 'r':  result += '\r'; i++; break;
                case 't':  result += '\t'; i++; break;
                case 'u':  
                    // Unicode escape - simplified handling
                    if (i + 5 < str.length()) {
                        // For now, just copy the escape sequence as-is
                        result += str.substr(i, 6);
                        i += 5;
                    } else {
                        result += str[i];
                    }
                    break;
                default:
                    result += str[i];
                    break;
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

bool isValidJson(const std::string& data) {
    try {
        FastJsonParser parser(data);
        parser.parseCommitSummaries();
        return true;
    } catch (...) {
        return false;
    }
}

void writeCommitsToJson(const std::vector<types::commit>& commits, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filepath);
    }
    
    file << "[\n";
    for (size_t i = 0; i < commits.size(); ++i) {
        const auto& commit = commits[i];
        
        file << "  {\n";
        file << "    \"id\": \"" << commit.id << "\",\n";
        file << "    \"message\": \"" << commit.originalMessage << "\",\n";
        file << "    \"summaries\": [";
        
        for (size_t j = 0; j < commit.hunkSummaries.size(); ++j) {
            file << "\"" << commit.hunkSummaries[j] << "\"";
            if (j < commit.hunkSummaries.size() - 1) file << ", ";
        }
        file << "],\n";
        
        file << "    \"commit_summary\": \"" << commit.newMessage << "\",\n";
        file << "    \"definitions\": [";
        
        for (size_t j = 0; j < commit.ctagDefinitions.size(); ++j) {
            file << "\"" << commit.ctagDefinitions[j] << "\"";
            if (j < commit.ctagDefinitions.size() - 1) file << ", ";
        }
        file << "],\n";
        
        file << "    \"key_points\": [";
        
        for (size_t j = 0; j < commit.regexDefinitions.size(); ++j) {
            file << "\"" << commit.regexDefinitions[j] << "\"";
            if (j < commit.regexDefinitions.size() - 1) file << ", ";
        }
        file << "]\n";
        
        file << "  }";
        if (i < commits.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
}

std::vector<types::commit> filterCommitsByKeyword(const std::vector<types::commit>& commits, const std::string& keyword) {
    std::vector<types::commit> filtered;
    
    for (const auto& commit : commits) {
        bool matches = false;
        
        // Check original message
        if (commit.originalMessage.find(keyword) != std::string::npos) {
            matches = true;
        }
        
        // Check new message
        if (commit.newMessage.find(keyword) != std::string::npos) {
            matches = true;
        }
        
        // Check definitions
        if (!matches) {
            for (const auto& def : commit.ctagDefinitions) {
                if (def.find(keyword) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        // Check key points
        if (!matches) {
            for (const auto& key : commit.regexDefinitions) {
                if (key.find(keyword) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            filtered.push_back(commit);
        }
    }
    
    return filtered;
}

void printCommitStatistics(const std::vector<types::commit>& commits) {
    if (commits.empty()) {
        std::cout << "No commits to analyze.\n";
        return;
    }
    
    size_t totalHunks = 0;
    size_t totalDefinitions = 0;
    size_t totalKeyPoints = 0;
    size_t commitsWithHunks = 0;
    size_t commitsWithDefinitions = 0;
    size_t commitsWithKeyPoints = 0;
    
    for (const auto& commit : commits) {
        totalHunks += commit.hunkSummaries.size();
        totalDefinitions += commit.ctagDefinitions.size();
        totalKeyPoints += commit.regexDefinitions.size();
        
        if (!commit.hunkSummaries.empty()) commitsWithHunks++;
        if (!commit.ctagDefinitions.empty()) commitsWithDefinitions++;
        if (!commit.regexDefinitions.empty()) commitsWithKeyPoints++;
    }
    
    std::cout << "=== Commit Statistics ===\n";
    std::cout << "Total commits: " << commits.size() << "\n";
    std::cout << "Total hunk summaries: " << totalHunks << "\n";
    std::cout << "Total ctag definitions: " << totalDefinitions << "\n";
    std::cout << "Total regex definitions: " << totalKeyPoints << "\n\n";
    
    std::cout << "Commits with hunk summaries: " << commitsWithHunks 
              << " (" << (commitsWithHunks * 100.0 / commits.size()) << "%)\n";
    std::cout << "Commits with definitions: " << commitsWithDefinitions 
              << " (" << (commitsWithDefinitions * 100.0 / commits.size()) << "%)\n";
    std::cout << "Commits with key points: " << commitsWithKeyPoints 
              << " (" << (commitsWithKeyPoints * 100.0 / commits.size()) << "%)\n\n";
    
    if (totalHunks > 0) {
        std::cout << "Average hunks per commit: " << (totalHunks / double(commits.size())) << "\n";
    }
    if (totalDefinitions > 0) {
        std::cout << "Average definitions per commit: " << (totalDefinitions / double(commits.size())) << "\n";
    }
    if (totalKeyPoints > 0) {
        std::cout << "Average key points per commit: " << (totalKeyPoints / double(commits.size())) << "\n";
    }
}

types::commit findCommitById(const std::vector<types::commit>& commits, const std::string& id) {
    auto it = std::find_if(commits.begin(), commits.end(),
        [&id](const types::commit& commit) {
            return commit.id == id;
        });
    
    if (it != commits.end()) {
        return *it;
    }
    
    throw std::runtime_error("Commit not found with ID: " + id);
}

} // namespace utils

} // namespace jsonParser
