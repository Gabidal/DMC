#ifndef _FILTER_H_
#define _FILTER_H_

#include "types.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>

namespace filter {

    /**
     * Filter utility class to remove unnecessary words from commit definitions.
     * Removes common English stop words, programming keywords, and other noise
     * to focus on actual definition names like function names, class names, etc.
     */
    class DefinitionFilter {
    private:
        // Common English stop words that should be filtered out
        static const std::unordered_set<std::string> stopWords;
        
        // Common programming keywords that aren't definition names
        static const std::unordered_set<std::string> programmingKeywords;
        
        // Common short words and noise patterns
        static const std::unordered_set<std::string> noiseWords;
        
        /**
         * Check if a word should be filtered out based on various criteria
         */
        static bool shouldFilter(const std::string& word);
        
        /**
         * Normalize a word (lowercase, trim, etc.) for comparison
         */
        static std::string normalizeWord(const std::string& word);
        
        /**
         * Check if a word looks like a valid identifier/definition name
         */
        static bool isValidIdentifier(const std::string& word);
        
        /**
         * Check if a word is too short or contains only common patterns
         */
        static bool isTooShort(const std::string& word);
        
    public:
        /**
         * Filter a single definition string, removing noise words
         */
        static std::string filterDefinition(const std::string& definition);
        
        /**
         * Filter a vector of definition strings
         */
        static std::vector<std::string> filterDefinitions(const std::vector<std::string>& definitions);
        
        /**
         * Filter definitions from a commit structure
         */
        static void filterCommitDefinitions(types::commit& commit);
        
        /**
         * Filter definitions from multiple commits
         */
        static void filterCommitDefinitions(std::vector<types::commit>& commits);
        
        /**
         * Get statistics about filtering (how many words were removed, etc.)
         */
        struct FilterStats {
            size_t totalWords;
            size_t filteredWords;
            size_t remainingWords;
            double filterRatio;
        };
        
        static FilterStats getFilterStats(const std::vector<std::string>& original, 
                                        const std::vector<std::string>& filtered);
    };

}

#endif