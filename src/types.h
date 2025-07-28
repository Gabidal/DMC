#ifndef _TYPES_H_
#define _TYPES_H_

#include <string>
#include <vector>
#include <limits>

namespace types {

    // This is an representative of the data we get per entry from the FixCom output json
    struct commit {
        std::string id;                                 // The commit hash
        std::string originalMessage;                    // The user written commit message
        std::vector<std::string> hunkSummaries;         // Summaries of each of the hunk belonging to this commit
        std::string newMessage;                         // The new FixCom generated commit message
        std::vector<std::string> ctagDefinitions;       // ctags exported symbols from the definitions
        std::vector<std::string> regexDefinitions;      // regex extracted symbols from the key_points


        // ---- DMC specifics ----
        unsigned int timeIndex;                         // Commits are added in time order, this is used to weight older commits higher.
    };

    struct connection {
        unsigned int Index;         // Points to the index where the commit resides.
        float weight;               // This is a normalized weight calculated via the number of commits and the timeIndex of the hosting commit. 

        void connect(commit& c) {
            // Connect this connection to the provided commit
            // The commit's timeIndex is used as the Index
            Index = c.timeIndex;
        }
    };

    struct definition {
        std::string symbol;     // The name this definition goes by.

        std::vector<connection> connections;    // Single definition can be present in multiple different commits, where the newer commits have more impact on said definition and its use.

        float occurrence;   // from 0.0f to 1.0f, representing the normalized weight amount of this definition occurred in all of the commits.

        float chronicPoint; // from 0.0f to 1.0f, representing as a 1D vector, where this definition is most common in the order of commits in time axis.
    };

    namespace cluster {
        enum class Type {
            UNKNOWN,
            CHRONIC,
            OCCURRENCE
        };
    
        class base {
        public:
            Type type = Type::UNKNOWN;
    
            std::vector<const definition*> definitions;    // Contains pointers to the definitions under this cluster.

            base(Type t) {  
                type = t;
            }
        };
    
        class chronic : public base {
        public:
            float radius;       // Represents the required radius of the definitions and their indifference to be encompassed under this cluster type.

            chronic() : base(Type::CHRONIC), radius(std::numeric_limits<float>::min()) {}
        };

        class occurrence : public base {
        public:
            float radius;       // Represents the required radius of the definitions and their indifference to be encompassed under this cluster type.

            occurrence() : base(Type::OCCURRENCE), radius(std::numeric_limits<float>::min()) {}
        };
    }


}

#endif