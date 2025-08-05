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
        
        std::cout << "Parsing summary summaries from: " << filepath << "\n";
        
        // Measure parsing time
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<types::json::parsable*> summaries = jsonParser::FastJsonParser::parseFromFile(filepath, types::json::type::SUMMARY);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Parsed " << summaries.size() << " commits in " << duration.count() << " ms\n\n";
        
        // Display statistics
        size_t totalHunks = 0;
        size_t totalDefinitions = 0;
        size_t totalKeyPoints = 0;
        
        for (const auto summary : summaries) {
            const auto Summary = (types::summary*)summary;

            totalHunks += Summary->hunkSummaries.size();
            totalDefinitions += Summary->ctagDefinitions.size();
            totalKeyPoints += Summary->regexDefinitions.size();
        }
        
        std::cout << "Statistics:\n";
        std::cout << "- Total commits: " << summaries.size() << "\n";
        std::cout << "- Total hunk summaries: " << totalHunks << "\n";
        std::cout << "- Total ctag definitions: " << totalDefinitions << "\n";
        std::cout << "- Total regex definitions: " << totalKeyPoints << "\n\n";
        
        // Now lets filter the definitions in each summary
        for (auto& summary : summaries) {
            const auto Summary = (types::summary*)summary;

            Summary->ctagDefinitions = filter::DefinitionFilter::filterDefinitions(Summary->ctagDefinitions);
            Summary->regexDefinitions = filter::DefinitionFilter::filterDefinitions(Summary->regexDefinitions);
        }

        // Now we'll feed the combed commits into the abstract
        abstract::base abstractSystem;
        abstractSystem.processSummaries(summaries);

        abstract::base::abstractStats stats = abstractSystem.getStatistics();

        std::cout << "Abstract System Statistics:\n";
        std::cout << "- Total definitions: " << stats.totalDefinitions << "\n";
        std::cout << "- Total connections: " << stats.totalConnections << "\n";
        std::cout << "- Total commits processed: " << stats.totalCommits << "\n";
        std::cout << "- Average occurrence: " << std::fixed << std::setprecision(4) << stats.averageOccurrence << "\n";
        std::cout << "- Average chronic point: " << std::fixed << std::setprecision(4) << stats.averageChronicPoint << "\n";
        std::cout << "- Average connections per definition: " << std::fixed << std::setprecision(2) << stats.averageConnectionsPerDefinition << "\n";

        abstractSystem.cluster();

        std::cout << "- Gained anti-entropy: " << abstractSystem.getEntropy() << "\n";
        std::cout << "- Variance: " << abstractSystem.getVariance() << "\n";
        std::cout << "- Average cluster size: " << abstractSystem.getAverageClusterSize() << "\n";
        std::cout << "- Silhouette score: " << std::fixed << std::setprecision(4) << abstractSystem.getSilhouetteScore() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}