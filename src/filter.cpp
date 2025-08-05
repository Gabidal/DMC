#include "filter.h"
#include <sstream>
#include <regex>

namespace filter {

    // Common English stop words that should be filtered out
    const std::unordered_set<std::string> DefinitionFilter::stopWords = {
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
        "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
        "to", "was", "will", "with", "or", "but", "not", "this", "they",
        "have", "had", "what", "when", "where", "who", "which", "why", "how"
    };

    // Common programming keywords that aren't definition names
    const std::unordered_set<std::string> DefinitionFilter::programmingKeywords = {
        "auto", "break", "case", "catch", "class", "const", "continue", "default",
        "delete", "do", "else", "enum", "explicit", "extern", "false", "finally",
        "for", "friend", "goto", "if", "inline", "int", "long", "namespace",
        "new", "null", "nullptr", "operator", "private", "protected", "public",
        "return", "short", "signed", "sizeof", "static", "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "while",
        "bool", "char", "double", "float", "string", "vector", "map", "set",
        "list", "array", "function", "method", "variable", "object", "type",
        "include", "define", "ifdef", "ifndef", "endif", "pragma"
    };

    // Common short words and noise patterns
    const std::unordered_set<std::string> DefinitionFilter::noiseWords = {
        "i", "x", "y", "z", "n", "m", "t", "s", "p", "q", "r", "c", "d", "e",
        "f", "g", "h", "j", "k", "l", "o", "u", "v", "w", "b", "tmp", "temp",
        "val", "var", "ptr", "ref", "obj", "cnt", "num", "idx", "len", "str",
        "msg", "err", "ret", "res", "arg", "param", "data", "info", "item",
        "node", "elem", "key", "value", "size", "count", "index", "length",
        "width", "height", "min", "max", "sum", "avg", "std", "dev", "test",
        "debug", "log", "print", "output", "input", "file", "path", "name",
        "id", "uid", "pid", "tid", "time", "date", "year", "month", "day",
        "hour", "minute", "second", "ms", "sec", "us", "ns"
    };

    std::string DefinitionFilter::normalizeWord(const std::string& word) {
        std::string normalized = word;
        
        // Convert to lowercase
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Remove leading/trailing whitespace
        normalized.erase(normalized.begin(), 
                        std::find_if(normalized.begin(), normalized.end(), 
                                   [](unsigned char ch) { return !std::isspace(ch); }));
        normalized.erase(std::find_if(normalized.rbegin(), normalized.rend(), 
                                    [](unsigned char ch) { return !std::isspace(ch); }).base(), 
                        normalized.end());
        
        return normalized;
    }

    bool DefinitionFilter::isTooShort(const std::string& word) {
        // Filter out very short words (1-2 characters) that are likely noise
        return word.length() <= 2;
    }

    bool DefinitionFilter::isValidIdentifier(const std::string& word) {
        if (word.empty()) return false;
        
        // Special case: lambda expressions
        if (word.find("lambda[") == 0) {
            return true;
        }
        
        // Special case: C++ operators
        if (word.find("operator") == 0) {
            return true;
        }
        
        // Special case: destructors (Class::~Destructor)
        if (word.find("::~") != std::string::npos) {
            return true;
        }
        
        // Check if it starts with a letter or underscore
        if (!std::isalpha(word[0]) && word[0] != '_') {
            return false;
        }
        
        // Check if it contains only valid identifier characters (including :: for scoping)
        for (char c : word) {
            if (!std::isalnum(c) && c != '_' && c != ':') {
                return false;
            }
        }
        
        // Must contain at least one letter (not just numbers and underscores)
        bool hasLetter = false;
        for (char c : word) {
            if (std::isalpha(c)) {
                hasLetter = true;
                break;
            }
        }
        
        return hasLetter;
    }

    bool DefinitionFilter::shouldFilter(const std::string& word) {
        std::string normalized = normalizeWord(word);
        
        // Filter empty or very short words
        if (normalized.empty() || isTooShort(normalized)) {
            return true;
        }
        
        // Special case: if word contains "::" it's likely a scoped identifier, don't filter based on individual parts
        if (word.find("::") != std::string::npos) {
            return !isValidIdentifier(word);
        }
        
        // Filter stop words
        if (stopWords.find(normalized) != stopWords.end()) {
            return true;
        }
        
        // Filter programming keywords
        if (programmingKeywords.find(normalized) != programmingKeywords.end()) {
            return true;
        }
        
        // Filter noise words
        if (noiseWords.find(normalized) != noiseWords.end()) {
            return true;
        }
        
        // Filter if it doesn't look like a valid identifier
        if (!isValidIdentifier(word)) {
            return true;
        }
        
        // Filter purely numeric words
        bool allDigits = true;
        for (char c : normalized) {
            if (!std::isdigit(c)) {
                allDigits = false;
                break;
            }
        }
        if (allDigits) {
            return true;
        }
        
        return false;
    }

    std::string DefinitionFilter::filterDefinition(const std::string& definition) {
        if (shouldFilter(definition)) {
            return "";  // Return empty string for filtered definitions
        }
        return definition;
    }

    std::vector<std::string> DefinitionFilter::filterDefinitions(const std::vector<std::string>& definitions) {
        std::vector<std::string> filtered;
        filtered.reserve(definitions.size());
        
        for (const std::string& def : definitions) {
            std::string filteredDef = filterDefinition(def);
            if (!filteredDef.empty()) {
                filtered.push_back(filteredDef);
            }
        }
        
        return filtered;
    }

    void DefinitionFilter::filterCommitDefinitions(types::summary& summary) {
        // Filter ctag definitions
        summary.ctagDefinitions = filterDefinitions(summary.ctagDefinitions);
        
        // Filter regex definitions (key_points)
        summary.regexDefinitions = filterDefinitions(summary.regexDefinitions);
    }

    void DefinitionFilter::filterCommitDefinitions(std::vector<types::summary>& commits) {
        for (types::summary& summary : commits) {
            filterCommitDefinitions(summary);
        }
    }

    DefinitionFilter::FilterStats DefinitionFilter::getFilterStats(
        const std::vector<std::string>& original, 
        const std::vector<std::string>& filtered) {
        
        FilterStats stats;
        stats.totalWords = original.size();
        stats.remainingWords = filtered.size();
        stats.filteredWords = stats.totalWords - stats.remainingWords;
        stats.filterRatio = stats.totalWords > 0 ? 
                           static_cast<double>(stats.filteredWords) / stats.totalWords : 0.0;
        
        return stats;
    }

}
