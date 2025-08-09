#include "../src/jsonParser.h"
#include "../src/filter.h"
#include "../src/abstract.h"
#include "../src/types.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

/**
 * Web Visualizer for DMC
 * This executable generates JSON data for the web-based interactive visualizer
 * and optionally starts a simple HTTP server to serve the visualization
 */
class WebVisualizer {
private:
    abstract::base abstractSystem;
    std::vector<types::json::parsable*> summaries;
    std::vector<types::json::parsable*> commits;
    
    std::string generateVisualizationData() {
        std::ostringstream json;

        json << "[\n";
        bool first = true;
        for (const auto cluster : abstractSystem.getClusters()) {
            if (cluster->type != types::node::Type::DISSONANCE_HUB)
                continue;

            if (!first) {
                json << ",\n";
            }
            json << "  " << cluster->getStats();
            first = false;
        }
        json << "\n]\n";
        
        return json.str();
    }
    
public:
    void processData(const std::string& summaryPath, const std::string& commitPath) {
        std::cout << "Processing commit data from: " << summaryPath << "\n";
        
        // Parse summaries
        summaries = jsonParser::FastJsonParser::parseFromFile(summaryPath, types::json::type::SUMMARY);
        std::cout << "Parsed " << summaries.size() << " summaries\n";
        
        // Parse commits
        commits = jsonParser::FastJsonParser::parseFromFile(commitPath, types::json::type::COMMIT);
        std::cout << "Parsed " << commits.size() << " commits\n";

        // Filter definitions
        for (auto& obj : summaries) {
            types::summary* commit = dynamic_cast<types::summary*>(obj);
            if (commit) {
                commit->ctagDefinitions = filter::DefinitionFilter::filterDefinitions(commit->ctagDefinitions);
                commit->regexDefinitions = filter::DefinitionFilter::filterDefinitions(commit->regexDefinitions);
            }
        }
        
        // Process through abstract system
        abstractSystem.processSummaries(summaries);
        abstractSystem.processCommits(commits);
        abstractSystem.cluster();
        
        std::cout << "Generated " << abstractSystem.getClusters().size() << " clusters\n";
    }
    
    void generateVisualizationData(const std::string& outputPath) {
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open output file: " + outputPath);
        }
        
        std::cout << "Generating visualization data...\n";
        
        // Use the simplified JSON generation method
        file << generateVisualizationData();
        
        file.close();
        std::cout << "Visualization data saved to: " << outputPath << "\n";
    }
    
    void generateStatistics() const {
        auto stats = abstractSystem.getStatistics();
        
        std::cout << "\n=== DMC Statistics ===\n";
        std::cout << "Total definitions: " << stats.totalDefinitions << "\n";
        std::cout << "Total connections: " << stats.totalConnections << "\n";
        std::cout << "Total commits: " << stats.totalCommits << "\n";
        std::cout << "Average occurrence: " << std::fixed << std::setprecision(4) << stats.averageOccurrence << "\n";
        std::cout << "Average chronic point: " << std::fixed << std::setprecision(4) << stats.averageChronicPoint << "\n";
        std::cout << "Average connections per definition: " << std::fixed << std::setprecision(2) << stats.averageConnectionsPerDefinition << "\n";
    }
};

int main(int argc, char* argv[]) {
    try {
        std::cout << "DMC Web Visualizer\n";
        std::cout << "==================\n\n";
        
        std::string inputSummariesPath = "test/data/commit_summaries.json";
        std::string inputCommitPath = "test/data/commit_data.json";
        std::string outputPath = "test/visualizer/data.json";
        
        // Parse command line arguments
        if (argc > 1) {
            inputSummariesPath = argv[1];
        }
        if (argc > 2) {
            outputPath = argv[2];
        }
        
        WebVisualizer visualizer;
        
        // Process the data
        auto start = std::chrono::high_resolution_clock::now();
        visualizer.processData(inputSummariesPath, inputCommitPath);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Processing completed in " << duration.count() << " ms\n";
        
        // Generate statistics
        visualizer.generateStatistics();
        
        // Generate visualization data
        visualizer.generateVisualizationData(outputPath);
        
        std::cout << "\n=== Next Steps ===\n";
        std::cout << "1. Open test/visualizer/index.html in your browser\n";
        std::cout << "2. Or run a local server: python3 -m http.server 8080 from test/visualizer/\n";
        std::cout << "3. Navigate to http://localhost:8080 in your browser\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
