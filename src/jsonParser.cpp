#include "jsonParser.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace jsonParser {

FastJsonParser::FastJsonParser(const std::string& jsonData) 
    : data(jsonData.c_str()), pos(0), length(jsonData.length()), targetType(types::json::type::SUMMARY) {}

FastJsonParser::FastJsonParser(const std::string& jsonData, types::json::type type) 
    : data(jsonData.c_str()), pos(0), length(jsonData.length()), targetType(type) {}

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
        if (data[pos] == '"') {
            // Check if this quote is escaped by counting preceding backslashes
            size_t backslashCount = 0;
            size_t checkPos = pos - 1;
            
            while (checkPos < length && checkPos >= start && data[checkPos] == '\\') {
                backslashCount++;
                if (checkPos == 0) break;
                checkPos--;
            }
            
            // If even number of backslashes (including 0), the quote is not escaped
            if (backslashCount % 2 == 0) {
                std::string result(data + start, pos - start);
                pos++; // Skip closing quote
                return utils::unescapeString(result);
            }
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

types::json::parsable* FastJsonParser::parseObject() {
    switch (targetType) {
        case types::json::type::SUMMARY:
            return parseSummary();
        case types::json::type::COMMIT:
            return parseCommit();
        case types::json::type::HUNK:
            return parseHunk();
        default:
            throw std::runtime_error("Unsupported parsable type");
    }
}

types::hunk* FastJsonParser::parseHunk() {
    auto hunk = new types::hunk();
    
    if (!match('{')) {
        throw std::runtime_error("Expected '{' for hunk object");
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
        
        if (key == "file") {
            hunk->file = parseString();
        } else if (key == "file_status" || key == "changeType") {
            hunk->changeType = parseString();
        } else if (key == "old_start") {
            // Parse number
            skipWhitespace();
            size_t start = pos;
            while (pos < length && (std::isdigit(data[pos]) || data[pos] == '-')) pos++;
            if (pos > start) {
                hunk->oldStart = std::stoul(std::string(data + start, pos - start));
            }
        } else if (key == "old_lines") {
            skipWhitespace();
            size_t start = pos;
            while (pos < length && (std::isdigit(data[pos]) || data[pos] == '-')) pos++;
            if (pos > start) {
                hunk->oldLines = std::stoul(std::string(data + start, pos - start));
            }
        } else if (key == "new_start") {
            skipWhitespace();
            size_t start = pos;
            while (pos < length && (std::isdigit(data[pos]) || data[pos] == '-')) pos++;
            if (pos > start) {
                hunk->newStart = std::stoul(std::string(data + start, pos - start));
            }
        } else if (key == "new_lines") {
            skipWhitespace();
            size_t start = pos;
            while (pos < length && (std::isdigit(data[pos]) || data[pos] == '-')) pos++;
            if (pos > start) {
                hunk->newLines = std::stoul(std::string(data + start, pos - start));
            }
        } else if (key == "old_text") {
            hunk->oldText = parseString();
        } else if (key == "new_text") {
            hunk->newText = parseString();
        } else {
            // Skip unknown fields
            skipValue();
        }
    }
    
    return hunk;
}

void FastJsonParser::skipValue() {
    skipWhitespace();
    if (pos >= length) return;
    
    if (data[pos] == '"') {
        parseString(); // Skip string value
    } else if (data[pos] == '[') {
        // Skip array - more robust handling
        int bracketCount = 0;
        do {
            if (data[pos] == '[') bracketCount++;
            else if (data[pos] == ']') bracketCount--;
            pos++;
        } while (pos < length && bracketCount > 0);
    } else if (data[pos] == '{') {
        // Skip object value - more robust handling
        int braceCount = 0;
        do {
            if (data[pos] == '{') braceCount++;
            else if (data[pos] == '}') braceCount--;
            pos++;
        } while (pos < length && braceCount > 0);
    } else if (std::isdigit(data[pos]) || data[pos] == '-') {
        // Skip number value
        while (pos < length && (std::isdigit(data[pos]) || data[pos] == '.' || data[pos] == '-' || data[pos] == 'e' || data[pos] == 'E' || data[pos] == '+')) {
            pos++;
        }
    } else if (data[pos] == 't' || data[pos] == 'f') {
        // Skip boolean value (true/false)
        while (pos < length && std::isalpha(data[pos])) {
            pos++;
        }
    } else if (data[pos] == 'n') {
        // Skip null value
        while (pos < length && std::isalpha(data[pos])) {
            pos++;
        }
    } else {
        // Skip until next comma or closing brace/bracket
        while (pos < length && data[pos] != ',' && data[pos] != '}' && data[pos] != ']') {
            pos++;
        }
    }
}

types::commit* FastJsonParser::parseCommit() {
    auto commit = new types::commit();
    
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
            commit->id = parseString();
        } else if (key == "message") {
            commit->message = parseString();
        } else if (key == "hunks") {
            // Parse hunks array
            skipWhitespace();
            if (!match('[')) {
                throw std::runtime_error("Expected '[' for hunks array");
            }
            
            skipWhitespace();
            if (!match(']')) {
                do {
                    auto oldTargetType = targetType;
                    targetType = types::json::type::HUNK;
                    auto hunk = parseHunk();
                    targetType = oldTargetType;
                    commit->hunks.push_back(std::move(*hunk));
                    skipWhitespace();
                } while (match(','));
                
                if (!match(']')) {
                    throw std::runtime_error("Expected ']' to close hunks array");
                }
            }
        } else {
            // Skip unknown fields
            skipValue();
        }
    }
    
    return commit;
}

types::summary* FastJsonParser::parseSummary() {
    auto summary = new types::summary();
    
    if (!match('{')) {
        throw std::runtime_error("Expected '{' for summary object");
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
            summary->id = parseString();
        } else if (key == "message") {
            summary->originalMessage = parseString();
        } else if (key == "summaries") {
            summary->hunkSummaries = parseStringArray();
        } else if (key == "commit_summary") {
            summary->newMessage = parseString();
        } else if (key == "definitions") {
            summary->ctagDefinitions = parseStringArray();
        } else if (key == "key_points") {
            summary->regexDefinitions = parseStringArray();
        } else {
            // Skip unknown fields
            skipValue();
        }
    }
    
    return summary;
}

std::vector<types::json::parsable*> FastJsonParser::parse() {
    std::vector<types::json::parsable*> objects;
    
    skipWhitespace();
    if (!match('[')) {
        throw std::runtime_error("Expected '[' for top-level array");
    }
    
    skipWhitespace();
    if (match(']')) {
        return objects; // Empty array
    }
    
    do {
        objects.push_back(parseObject());
        
        // Set timeIndex for summaries
        if (targetType == types::json::type::SUMMARY) {
            auto* summary = dynamic_cast<types::summary*>(objects.back());
            if (summary) {
                summary->timeIndex = static_cast<unsigned int>(objects.size() - 1);
            }
        }
        
        skipWhitespace();
    } while (match(','));
    
    if (!match(']')) {
        throw std::runtime_error("Expected ']' to close top-level array");
    }
    
    return objects;
}

std::vector<types::json::parsable*> FastJsonParser::parseFromFile(const std::string& filepath, types::json::type type) {
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
    
    FastJsonParser parser(content, type);
    return parser.parse();
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
    
    // Filter Windows-style line endings (\r\n) and replace with Unix-style (\n)
    std::string filtered;
    filtered.reserve(result.length());
    
    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] == '\r' && i + 1 < result.length() && result[i + 1] == '\n') {
            // Skip the \r and let the \n be added in the next iteration
            continue;
        } else {
            filtered += result[i];
        }
    }
    
    return filtered;
}

bool isValidJson(const std::string& data, types::json::type t) {
    try {
        FastJsonParser parser(data, t);
        parser.parse();
        return true;
    } catch (...) {
        return false;
    }
}

bool isValidJson(const std::string& data) {
    // Default to SUMMARY type for backward compatibility
    return isValidJson(data, types::json::type::SUMMARY);
}

void writeCommitsToJson(const std::vector<types::summary>& commits, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filepath);
    }
    
    file << "[\n";
    for (size_t i = 0; i < commits.size(); ++i) {
        const auto& summary = commits[i];
        
        file << "  {\n";
        file << "    \"id\": \"" << summary.id << "\",\n";
        file << "    \"message\": \"" << summary.originalMessage << "\",\n";
        file << "    \"summaries\": [";
        
        for (size_t j = 0; j < summary.hunkSummaries.size(); ++j) {
            file << "\"" << summary.hunkSummaries[j] << "\"";
            if (j < summary.hunkSummaries.size() - 1) file << ", ";
        }
        file << "],\n";
        
        file << "    \"commit_summary\": \"" << summary.newMessage << "\",\n";
        file << "    \"definitions\": [";
        
        for (size_t j = 0; j < summary.ctagDefinitions.size(); ++j) {
            file << "\"" << summary.ctagDefinitions[j] << "\"";
            if (j < summary.ctagDefinitions.size() - 1) file << ", ";
        }
        file << "],\n";
        
        file << "    \"key_points\": [";
        
        for (size_t j = 0; j < summary.regexDefinitions.size(); ++j) {
            file << "\"" << summary.regexDefinitions[j] << "\"";
            if (j < summary.regexDefinitions.size() - 1) file << ", ";
        }
        file << "]\n";
        
        file << "  }";
        if (i < commits.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
}

std::vector<types::summary> filterCommitsByKeyword(const std::vector<types::summary>& commits, const std::string& keyword) {
    std::vector<types::summary> filtered;
    
    for (const auto& summary : commits) {
        bool matches = false;
        
        // Check original message
        if (summary.originalMessage.find(keyword) != std::string::npos) {
            matches = true;
        }
        
        // Check new message
        if (summary.newMessage.find(keyword) != std::string::npos) {
            matches = true;
        }
        
        // Check definitions
        if (!matches) {
            for (const auto& def : summary.ctagDefinitions) {
                if (def.find(keyword) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        // Check key points
        if (!matches) {
            for (const auto& key : summary.regexDefinitions) {
                if (key.find(keyword) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            filtered.push_back(summary);
        }
    }
    
    return filtered;
}

void printCommitStatistics(const std::vector<types::summary>& commits) {
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
    
    for (const auto& summary : commits) {
        totalHunks += summary.hunkSummaries.size();
        totalDefinitions += summary.ctagDefinitions.size();
        totalKeyPoints += summary.regexDefinitions.size();
        
        if (!summary.hunkSummaries.empty()) commitsWithHunks++;
        if (!summary.ctagDefinitions.empty()) commitsWithDefinitions++;
        if (!summary.regexDefinitions.empty()) commitsWithKeyPoints++;
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
        std::cout << "Average hunks per summary: " << (totalHunks / double(commits.size())) << "\n";
    }
    if (totalDefinitions > 0) {
        std::cout << "Average definitions per summary: " << (totalDefinitions / double(commits.size())) << "\n";
    }
    if (totalKeyPoints > 0) {
        std::cout << "Average key points per summary: " << (totalKeyPoints / double(commits.size())) << "\n";
    }
}

types::summary findCommitById(const std::vector<types::summary>& commits, const std::string& id) {
    auto it = std::find_if(commits.begin(), commits.end(),
        [&id](const types::summary& summary) {
            return summary.id == id;
        });
    
    if (it != commits.end()) {
        return *it;
    }
    
    throw std::runtime_error("Commit not found with ID: " + id);
}

// New generic utility functions
void writeParsablesToJson(const std::vector<types::json::parsable*>& objects, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filepath);
    }
    
    file << "[\n";
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& obj = objects[i];
        
        if (obj->Type == types::json::type::SUMMARY) {
            auto* summary = dynamic_cast<const types::summary*>(obj);
            if (summary) {
                file << "  {\n";
                file << "    \"id\": \"" << summary->id << "\",\n";
                file << "    \"message\": \"" << summary->originalMessage << "\",\n";
                file << "    \"summaries\": [";
                
                for (size_t j = 0; j < summary->hunkSummaries.size(); ++j) {
                    file << "\"" << summary->hunkSummaries[j] << "\"";
                    if (j < summary->hunkSummaries.size() - 1) file << ", ";
                }
                file << "],\n";
                
                file << "    \"commit_summary\": \"" << summary->newMessage << "\",\n";
                file << "    \"definitions\": [";
                
                for (size_t j = 0; j < summary->ctagDefinitions.size(); ++j) {
                    file << "\"" << summary->ctagDefinitions[j] << "\"";
                    if (j < summary->ctagDefinitions.size() - 1) file << ", ";
                }
                file << "],\n";
                
                file << "    \"key_points\": [";
                
                for (size_t j = 0; j < summary->regexDefinitions.size(); ++j) {
                    file << "\"" << summary->regexDefinitions[j] << "\"";
                    if (j < summary->regexDefinitions.size() - 1) file << ", ";
                }
                file << "]\n";
                file << "  }";
            }
        } else if (obj->Type == types::json::type::COMMIT) {
            auto* commit = dynamic_cast<const types::commit*>(obj);
            if (commit) {
                file << "  {\n";
                file << "    \"id\": \"" << commit->id << "\",\n";
                file << "    \"message\": \"" << commit->message << "\",\n";
                file << "    \"hunks\": [\n";
                
                for (size_t j = 0; j < commit->hunks.size(); ++j) {
                    const auto& hunk = commit->hunks[j];
                    file << "      {\n";
                    file << "        \"file\": \"" << hunk.file << "\",\n";
                    file << "        \"file_status\": \"" << hunk.changeType << "\",\n";
                    file << "        \"old_start\": " << hunk.oldStart << ",\n";
                    file << "        \"old_lines\": " << hunk.oldLines << ",\n";
                    file << "        \"new_start\": " << hunk.newStart << ",\n";
                    file << "        \"new_lines\": " << hunk.newLines << ",\n";
                    file << "        \"old_text\": \"" << hunk.oldText << "\",\n";
                    file << "        \"new_text\": \"" << hunk.newText << "\"\n";
                    file << "      }";
                    if (j < commit->hunks.size() - 1) file << ",";
                    file << "\n";
                }
                
                file << "    ]\n";
                file << "  }";
            }
        }
        
        if (i < objects.size() - 1) file << ",";
        file << "\n";
    }
    file << "]\n";
}

std::vector<types::json::parsable*> filterParsablesByKeyword(const std::vector<types::json::parsable*>& objects, const std::string& keyword) {
    std::vector<types::json::parsable*> filtered;
    
    for (const auto& obj : objects) {
        bool matches = false;
        
        if (obj->Type == types::json::type::SUMMARY) {
            auto* summary = dynamic_cast<const types::summary*>(obj);
            if (summary) {
                if (summary->originalMessage.find(keyword) != std::string::npos ||
                    summary->newMessage.find(keyword) != std::string::npos) {
                    matches = true;
                }
                
                if (!matches) {
                    for (const auto& def : summary->ctagDefinitions) {
                        if (def.find(keyword) != std::string::npos) {
                            matches = true;
                            break;
                        }
                    }
                }
            }
        } else if (obj->Type == types::json::type::COMMIT) {
            auto* commit = dynamic_cast<const types::commit*>(obj);
            if (commit) {
                if (commit->id.find(keyword) != std::string::npos ||
                    commit->message.find(keyword) != std::string::npos) {
                    matches = true;
                }
            }
        }
        
        if (matches) {
            // Create a copy of the object
            if (obj->Type == types::json::type::SUMMARY) {
                auto* summary = dynamic_cast<const types::summary*>(obj);
                if (summary) {
                    filtered.push_back(new types::summary(*summary));
                }
            } else if (obj->Type == types::json::type::COMMIT) {
                auto* commit = dynamic_cast<const types::commit*>(obj);
                if (commit) {
                    filtered.push_back(new types::commit(*commit));
                }
            }
        }
    }
    
    return filtered;
}

void printParsableStatistics(const std::vector<types::json::parsable*>& objects) {
    if (objects.empty()) {
        std::cout << "No objects to analyze.\n";
        return;
    }
    
    size_t summaryCount = 0;
    size_t commitCount = 0;
    size_t hunkCount = 0;
    
    for (const auto& obj : objects) {
        switch (obj->Type) {
            case types::json::type::SUMMARY:
                summaryCount++;
                break;
            case types::json::type::COMMIT:
                commitCount++;
                break;
            case types::json::type::HUNK:
                hunkCount++;
                break;
            default:
                break;
        }
    }
    
    std::cout << "=== Parsable Object Statistics ===\n";
    std::cout << "Total objects: " << objects.size() << "\n";
    std::cout << "Summaries: " << summaryCount << "\n";
    std::cout << "Commits: " << commitCount << "\n";
    std::cout << "Hunks: " << hunkCount << "\n";
}

} // namespace utils

} // namespace jsonParser
