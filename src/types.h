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

    namespace node {
        enum class Type {
            UNKNOWN,
            DEFINITION,
            CHRONIC,
            OCCURRENCE,
            DISSONANCE_HUB,     // Hub clusters, consisting of similar clusters with same radiuses representing the field radius, where larger radius represents more different definitions.
            RESONANCE_HUB       // Hub clusters, consisting of similar definition vectors.
        };

        class base {
        public:
            Type type = node::Type::UNKNOWN;

            base(Type t) : type(t) {}

            virtual ~base() {}

            virtual std::string getName() const = 0;
            virtual std::string getStats(unsigned int indent = 0) = 0;

            virtual std::vector<float> getVector() = 0;

            std::string getVectorAsString() {
                std::vector<float> vec = getVector();
                std::string result = "[";
                for (size_t i = 0; i < vec.size(); ++i) {
                    result += std::to_string(vec[i]);
                    if (i < vec.size() - 1) {
                        result += ", ";
                    }
                }
                return result + "]";
            }
        };
    }

    class definition : public node::base {
    public:
        std::string symbol;     // The name this definition goes by.

        std::vector<connection> connections;    // Single definition can be present in multiple different commits, where the newer commits have more impact on said definition and its use.

        float CommitFrequency;      // from 0.0f to 1.0f, representing the normalized weight amount of this definition occurred in all of the commits.
        float ClusterFrequency;     // from 0.0f to 1.0f, representing the normalized weight amount of this definition occurred in all of the clusters.

        float chronicPoint; // from 0.0f to 1.0f, representing as a 1D vector, where this definition is most common in the order of commits in time axis.

        definition() : base(node::Type::DEFINITION) {}

        std::string getName() const override {
            return symbol;
        }

        std::string getStats(unsigned int indent = 0) override {
            std::string spaces(indent * 2, ' ');
            return
                spaces + "  {\n" +
                spaces + "    \"symbol\": \"" + symbol + "\",\n" +
                spaces + "    \"vector\": " + getVectorAsString() + ",\n" +
                spaces + "    \"connections\": " + std::to_string(connections.size()) + "\n" +
                spaces + "  }";
        }

        std::vector<float> getVector() override {
            return {
                CommitFrequency,
                ClusterFrequency,
                chronicPoint
            };
        }
    };

    class cluster : public node::base {
        public:
        static constexpr float upscaleRadius = 1000;

        float radius;       // Represents the required radius of the definitions and their indifference to be encompassed under this cluster type.
        std::vector<base*> definitions;    // Contains pointers to the definitions under this cluster.

        std::vector<float> cachedVector;

        cluster(node::Type t) : base(t), radius(std::numeric_limits<float>::min()) { }

        ~cluster() override {
            for (auto* def : definitions) {
                delete def; // Clean up definitions if they were dynamically allocated
            }
        }

        std::string getName() const override {
            switch (type) {
                case node::Type::CHRONIC:
                    return "CHRONIC";
                case node::Type::OCCURRENCE:
                    return "OCCURRENCE";
                case node::Type::DISSONANCE_HUB:
                    return "DISSONANCE_HUB";
                case node::Type::RESONANCE_HUB:
                    return "RESONANCE_HUB";
                default:
                    return "UNKNOWN";
            }
        }
    
        std::string getStats(unsigned int indent = 0) override {
            std::string spaces(indent * 2, ' ');

            std::string definitionsJson = "[\n";

            for (size_t i = 0; i < definitions.size(); ++i) {
                if (i > 0) definitionsJson += ",\n";
                definitionsJson += definitions[i]->getStats(indent + 2);
            }
            definitionsJson += "\n" + spaces + "    " + "]";

            return
                spaces + "  {\n" +
                spaces + "    \"type\": \"" + getName() + "\",\n" +
                spaces + "    \"radius\": " + std::to_string(radius * upscaleRadius) + ",\n" +
                spaces + "    \"vector\": " + getVectorAsString() + ",\n" +
                spaces + "    \"definitions\": " + definitionsJson + "\n" +
                spaces + "  }";
        }
    
        std::vector<float> getVector() override;
    };
}

#endif