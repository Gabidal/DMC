#ifndef _TYPES_H_
#define _TYPES_H_

#include <string>
#include <vector>
#include <limits>

#include <algorithm>

namespace types {
    namespace json {
        enum class type {
            UNKNOWN,
            SUMMARY,
            COMMIT,
            HUNK
        };

        class parsable {
        public:
            type Type = type::UNKNOWN;

            parsable() = default;
            parsable(type t) : Type(t) {}

            virtual ~parsable() {}
        };
    }

    class hunk : public json::parsable {
    public:
        std::string file;           // The file name
        std::string changeType;     // The info of where it was addition, rename or modification
        unsigned int oldStart;      // The line number of the old hunk
        unsigned int oldLines;      // The number of lines the old hunk represents
        unsigned int newStart;      // The line number of the new hunk
        unsigned int newLines;      // The number of lines the new hunk represents
        std::string oldText;        // The text of the old hunk
        std::string newText;        // The text of the new hunk

        hunk() : json::parsable(json::type::HUNK), oldStart(0), oldLines(0), newStart(0), newLines(0) {}
    };

    class commit : public json::parsable {
    public:
        std::string id;             // The commit hash
        std::string message;        // The commit message

        std::vector<hunk> hunks;    // The related hunks
        
        // ----- DMC specifics -----
        unsigned int summaryIndex;      // Index pointing to the summary index in abstract::base
        
        commit() : json::parsable(json::type::COMMIT), summaryIndex(0) {}
    };

    // This is an representative of the data we get per entry from the FixCom output json
    class summary : public json::parsable {
    public:
        std::string id;                                 // The summary hash
        std::string originalMessage;                    // The user written summary message
        std::vector<std::string> hunkSummaries;         // Summaries of each of the hunk belonging to this summary
        std::string newMessage;                         // The new FixCom generated summary message
        std::vector<std::string> ctagDefinitions;       // ctags exported symbols from the definitions
        std::vector<std::string> regexDefinitions;      // regex extracted symbols from the key_points


        // ---- DMC specifics ----
        unsigned int timeIndex;                         // summaries are added in time order, this is used to weight older summaries higher.

        summary() : json::parsable(json::type::SUMMARY), timeIndex(0) {}
    };

    struct connection {
        unsigned int Index;         // Points to the index where the summary resides.
        float weight;               // This is a normalized weight calculated via the number of commits and the timeIndex of the hosting summary. 

        void connect(summary& c) {
            // Connect this connection to the provided summary
            // The summary's timeIndex is used as the Index
            Index = c.timeIndex;
        }
    };

    inline std::string getNormalSymbol(std::string raw) {
        // First we'll remove all underscore characters
        raw.erase(std::remove(raw.begin(), raw.end(), '_'), raw.end());

        // Now we'll make all uppercase letters lowercase
        std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

        return raw;
    }

    namespace node {
        enum class Type {
            UNKNOWN,
            DEFINITION,
            CHRONIC,
            OCCURRENCE,
            DISSONANCE_HUB,     // Hub clusters, consisting of similar clusters with same radiuses representing the field radius, where larger radius represents more different definitions.
            RESONANCE_HUB,       // Hub clusters, consisting of similar definition vectors.
            CONTEXT,    // context nodes which contain namespace like member fetching of smaller features.
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

        std::vector<size_t> referenced;     // Contains file indicies where this definition has been referenced in.

        std::vector<std::string> history;   // Contains in chronological order of the different aliases this definition has had through the commit history.

        float CommitFrequency;      // from 0.0f to 1.0f, representing the normalized weight amount of this definition occurred in all of the commits.
        float ClusterFrequency;     // from 0.0f to 1.0f, representing the normalized weight amount of this definition occurred in all of the clusters.
        float chronicPoint;         // from 0.0f to 1.0f, representing as a 1D vector, where this definition is most common in the order of commits in time axis.
        float fileVector;           // from 0.0f to 1.0f, representing as a 1D vector, pointing to the file index and its surrounding files.

        definition() : base(node::Type::DEFINITION), CommitFrequency(0.0f), ClusterFrequency(0.0f), chronicPoint(0.0f), fileVector(0.0f) {}

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
                chronicPoint,
                fileVector
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
                case node::Type::CONTEXT:
                    return "CONTEXT";
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

        // Used as an cost function
        float getVariance();
    };

    class context : public cluster {
    public:
        std::string symbol;

        context(const std::string& sym) : cluster(node::Type::CONTEXT), symbol(sym) {}

        context* find(const std::string name) {
            if (getNormalSymbol(symbol) == getNormalSymbol(name)) {
                return this;
            }
            for (auto* def : definitions) {
                if (def->type == node::Type::CONTEXT) {
                    context* ctx = static_cast<context*>(def);
                    context* found = ctx->find(name);
                    if (found) {
                        return found;
                    }
                }
            }
            return nullptr; // Not found
        }

        std::string getName() const override {
            return symbol;
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
                spaces + "    \"symbol\": \"" + symbol + "\",\n" +
                spaces + "    \"type\": \"CONTEXT\",\n" +
                spaces + "    \"radius\": " + std::to_string(radius * upscaleRadius) + ",\n" +
                spaces + "    \"vector\": " + getVectorAsString() + ",\n" +
                spaces + "    \"definitions\": " + definitionsJson + "\n" +
                spaces + "  }";
        }

        std::vector<float> getVector() override {
            return cluster::getVector();
        }
    };
}

#endif