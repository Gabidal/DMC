#ifndef _TYPES_H_
#define _TYPES_H_

#include <string>
#include <vector>

namespace types {

    struct commit {
        std::string id;                                 // The commit hash
        std::string originalMessage;                    // The user written commit message
        std::vector<std::string> hunkSummaries;         // Summaries of each of the hunk belonging to this commit
        std::string newMessage;                         // The new FixCom generated commit message
        std::vector<std::string> ctagDefinitions;       // ctags exported symbols from the definitions
        std::vector<std::string> regexDefinitions;      // regex extracted symbols from the key_points
    };

}

#endif