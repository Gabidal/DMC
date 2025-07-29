#include "jsonParser.h"
#include "filter.h"
#include "abstract.h"
#include "types.h"


#include <iostream>
#include <chrono>
#include <iomanip>

int main() {
    try {
        std::cout << "DMC\n";
        std::cout << "=====================\n\n";
        
        const std::string filepath = "test/data/commit_summaries.json";
        
        std::cout << "Parsing commit summaries from: " << filepath << "\n";
        
        // Measure parsing time
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<types::commit> commits = jsonParser::FastJsonParser::parseFromFile(filepath);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Parsed " << commits.size() << " commits in " << duration.count() << " ms\n\n";
        
        // Display statistics
        size_t totalHunks = 0;
        size_t totalDefinitions = 0;
        size_t totalKeyPoints = 0;
        
        for (const auto& commit : commits) {
            totalHunks += commit.hunkSummaries.size();
            totalDefinitions += commit.ctagDefinitions.size();
            totalKeyPoints += commit.regexDefinitions.size();
        }
        
        std::cout << "Statistics:\n";
        std::cout << "- Total commits: " << commits.size() << "\n";
        std::cout << "- Total hunk summaries: " << totalHunks << "\n";
        std::cout << "- Total ctag definitions: " << totalDefinitions << "\n";
        std::cout << "- Total regex definitions: " << totalKeyPoints << "\n\n";
        
        // Now lets filter the definitions in each commit
        for (auto& commit : commits) {
            commit.ctagDefinitions = filter::DefinitionFilter::filterDefinitions(commit.ctagDefinitions);
            commit.regexDefinitions = filter::DefinitionFilter::filterDefinitions(commit.regexDefinitions);
        }

        // Now we'll feed the combed commits into the abstract
        abstract::base abstractSystem;
        abstractSystem.processCommits(commits);

        abstract::base::abstractStats stats = abstractSystem.getStatistics();

        std::cout << "Abstract System Statistics:\n";
        std::cout << "- Total definitions: " << stats.totalDefinitions << "\n";
        std::cout << "- Total connections: " << stats.totalConnections << "\n";
        std::cout << "- Total commits processed: " << stats.totalCommits << "\n";
        std::cout << "- Average occurrence: " << std::fixed << std::setprecision(4) << stats.averageOccurrence << "\n";
        std::cout << "- Average chronic point: " << std::fixed << std::setprecision(4) << stats.averageChronicPoint << "\n";
        std::cout << "- Average connections per definition: " << std::fixed << std::setprecision(2) << stats.averageConnectionsPerDefinition << "\n\n";
        
        // Now we'll call the clustering algos
        abstractSystem.cluster();

        const auto clusters = abstractSystem.getClusters();

        std::cout << "Generated Clusters (JSON format):\n";
        std::cout << "[\n";
        bool first = true;
        for (const auto cluster : clusters) {
            if (cluster->type != types::node::Type::DISSONANCE_HUB)
                continue;

            if (!first) {
                std::cout << ",\n";
            }
            std::cout << "  " << cluster->getStats();
            first = false;
        }
        std::cout << "\n]\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}