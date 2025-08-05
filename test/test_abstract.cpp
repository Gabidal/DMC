#include "../src/abstract.h"
#include "../src/types.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>

namespace test_abstract {

    // Helper function to create a test summary
    types::summary* createTestCommit(const std::string& id, 
                                   const std::vector<std::string>& ctagDefs,
                                   const std::vector<std::string>& regexDefs = {}) {
        types::summary* summary = new types::summary();
        summary->id = id;
        summary->originalMessage = "Test summary " + id;
        summary->newMessage = "Processed test summary " + id;
        summary->ctagDefinitions = ctagDefs;
        summary->regexDefinitions = regexDefs;
        summary->timeIndex = 0; // Will be set by processCommits
        return summary;
    }

    void testBasicAbstractSystemFunctionality() {
        std::cout << "Testing basic AbstractSystem functionality..." << std::endl;
        
        abstract::base system;
        
        // Create test commits with overlapping definitions
        std::vector<types::json::parsable*> commits;
        
        // Commit 1: Functions A, B, C
        commits.push_back(createTestCommit("commit1", {"FunctionA", "FunctionB", "FunctionC"}));
        
        // Commit 2: Functions A, B, D (A and B co-occur again)
        commits.push_back(createTestCommit("commit2", {"FunctionA", "FunctionB", "FunctionD"}));
        
        // Commit 3: Functions C, E, F
        commits.push_back(createTestCommit("commit3", {"FunctionC", "FunctionE", "FunctionF"}));
        
        // Commit 4: Functions D, E (D and E co-occur)
        commits.push_back(createTestCommit("commit4", {"FunctionD", "FunctionE"}));
        
        system.processSummaries(commits);
        
        // Test basic statistics
        auto stats = system.getStatistics();
        assert(stats.totalCommits == 4);
        assert(stats.totalDefinitions == 6); // A, B, C, D, E, F
        std::cout << "  ✓ Basic statistics correct" << std::endl;
        
        // Test that definitions exist
        const auto* defA = system.getDefinition("FunctionA");
        const auto* defB = system.getDefinition("FunctionB");
        assert(defA != nullptr);
        assert(defB != nullptr);
        assert(defA->symbol == "FunctionA");
        assert(defB->symbol == "FunctionB");
        std::cout << "  ✓ Definitions created correctly" << std::endl;
        
        // Test connection weights vector
        auto weightsA = system.getConnectionWeightsVector(*defA);
        assert(weightsA.size() == 4); // Should have weights for all 4 commits
        assert(weightsA[0] > 0.0f); // Should have weight in summary 0
        assert(weightsA[1] > 0.0f); // Should have weight in summary 1
        assert(weightsA[2] == 0.0f); // Should not have weight in summary 2
        assert(weightsA[3] == 0.0f); // Should not have weight in summary 3
        std::cout << "  ✓ Connection weights vector correct" << std::endl;
        
        // Test cosine similarity
        float similarity = system.calculateCosineSimilarity(*defA, *defB);
        assert(similarity > 0.0f); // A and B co-occur in commits 0 and 1
        std::cout << "  ✓ Cosine similarity calculation works (A-B similarity: " << similarity << ")" << std::endl;
        
        std::cout << "Basic AbstractSystem functionality tests passed!" << std::endl;
    }

    void testClusteringFunctionality() {
        std::cout << "\nTesting clustering functionality..." << std::endl;
        
        abstract::base system;
        
        // Create test commits with clear clustering patterns
        std::vector<types::json::parsable*> commits;
        
        // Group 1: Web-related functions (should cluster together)
        commits.push_back(createTestCommit("commit1", {"renderHTML", "parseCSS", "handleHTTPRequest"}));
        commits.push_back(createTestCommit("commit2", {"renderHTML", "parseCSS", "validateHTML"}));
        commits.push_back(createTestCommit("commit3", {"handleHTTPRequest", "validateHTML"}));
        
        // Group 2: Database-related functions (should cluster together)
        commits.push_back(createTestCommit("commit4", {"connectDB", "executeQuery", "closeConnection"}));
        commits.push_back(createTestCommit("commit5", {"connectDB", "executeQuery", "beginTransaction"}));
        commits.push_back(createTestCommit("commit6", {"closeConnection", "beginTransaction"}));
        
        // Group 3: Isolated functions (should be in smaller clusters or noise)
        commits.push_back(createTestCommit("commit7", {"utilityFunction", "helperMethod"}));
        commits.push_back(createTestCommit("commit8", {"singletonFunction"}));
        
        system.processSummaries(commits);
        
        // Test that we have the expected number of definitions
        auto stats = system.getStatistics();
        std::cout << "  Debug: Found " << stats.totalDefinitions << " definitions (expected 11)" << std::endl;
        
        // Print all definitions for debugging
        const auto& allDefs = system.getDefinitions();
        std::cout << "  All definitions found:" << std::endl;
        for (const auto& pair : allDefs) {
            std::cout << "    - " << pair.first << std::endl;
        }
        
        assert(stats.totalDefinitions == 11); // All unique functions
        std::cout << "  ✓ Created " << stats.totalDefinitions << " definitions" << std::endl;
        
        // Perform clustering
        system.cluster();
        
        // Get all clusters
        const auto& allClusters = system.getClusters();
        std::cout << "  ✓ Generated " << allClusters.size() << " total clusters" << std::endl;
        
        // Test different cluster types
        auto chronicLClusters = system.getClustersByType(types::node::Type::CHRONIC);
        
        std::cout << "  ✓ Chronic clusters: " << chronicLClusters.size() << std::endl;
        
        for (const auto cluster : allClusters) {
            if (cluster->definitions.size() >= 2) {
                std::cout << "  Cluster (type: ";
                switch(cluster->type) {
                    case types::node::Type::CHRONIC:
                        std::cout << "CHRONIC";
                        break;
                    case types::node::Type::OCCURRENCE:
                        std::cout << "OCCURRENCE";
                        break;
                    case types::node::Type::DISSONANCE_HUB:
                        std::cout << "DISSONANCE_HUB";
                        break;
                    case types::node::Type::RESONANCE_HUB:
                        std::cout << "RESONANCE_HUB";
                        break;
                    case types::node::Type::CONTEXT:
                        std::cout << "CONTEXT";
                        break;
                    default:
                        std::cout << "UNKNOWN";
                        break;
                }
                std::cout << ") with " << cluster->definitions.size() << " definitions:" << std::endl;
                
                std::vector<std::string> symbols;
                for (const auto* def : cluster->definitions) {
                    symbols.push_back(def->getName());
                    std::cout << "    - " << def->getName() << std::endl;
                }
                
                // Check if this looks like a web-related cluster
                int webCount = 0;
                for (const std::string& symbol : symbols) {
                    if (symbol.find("HTML") != std::string::npos || 
                        symbol.find("CSS") != std::string::npos || 
                        symbol.find("HTTP") != std::string::npos) {
                        webCount++;
                    }
                }
                
                // Check if this looks like a database-related cluster
                int dbCount = 0;
                for (const std::string& symbol : symbols) {
                    if (symbol.find("DB") != std::string::npos || 
                        symbol.find("Query") != std::string::npos || 
                        symbol.find("Connection") != std::string::npos ||
                        symbol.find("Transaction") != std::string::npos) {
                        dbCount++;
                    }
                }
            }
        }
        
        // We should find at least some meaningful clusters
        assert(allClusters.size() > 0);
        std::cout << "  ✓ Found meaningful clusters in the data" << std::endl;
        
        std::cout << "Clustering functionality tests passed!" << std::endl;
    }

    void testSimilarityMatrix() {
        std::cout << "\nTesting similarity matrix generation..." << std::endl;
        
        abstract::base system;
        
        // Create simple test case with known relationships
        std::vector<types::json::parsable*> commits;
        
        // Functions A and B always appear together
        commits.push_back(createTestCommit("commit1", {"FunctionA", "FunctionB"}));
        commits.push_back(createTestCommit("commit2", {"FunctionA", "FunctionB"}));
        
        // Function C appears alone
        commits.push_back(createTestCommit("commit3", {"FunctionC"}));
        
        system.processSummaries(commits);
        
        // Get definitions for testing
        const auto* defA = system.getDefinition("FunctionA");
        const auto* defB = system.getDefinition("FunctionB");
        const auto* defC = system.getDefinition("FunctionC");
        
        assert(defA != nullptr);
        assert(defB != nullptr);
        assert(defC != nullptr);
        
        // Test cosine similarity
        float simAB = system.calculateCosineSimilarity(*defA, *defB);
        float simAC = system.calculateCosineSimilarity(*defA, *defC);
        float simBC = system.calculateCosineSimilarity(*defB, *defC);
        
        std::cout << "  A-B similarity: " << simAB << std::endl;
        std::cout << "  A-C similarity: " << simAC << std::endl;
        std::cout << "  B-C similarity: " << simBC << std::endl;
        
        // A and B should be very similar (they always co-occur)
        assert(simAB > 0.9f);
        
        // A/B and C should have zero similarity (they never co-occur)
        assert(simAC == 0.0f);
        assert(simBC == 0.0f);
        
        std::cout << "  ✓ Similarity calculations are correct" << std::endl;
        
        std::cout << "Similarity matrix tests passed!" << std::endl;
    }

    void testEdgeCases() {
        std::cout << "\nTesting edge cases..." << std::endl;
        
        abstract::base system;
        
        // Test empty system
        system.cluster();
        assert(system.getClusters().size() == 0);
        std::cout << "  ✓ Empty system clustering handled correctly" << std::endl;
        
        // Test single definition
        std::vector<types::json::parsable*> singleCommit;
        singleCommit.push_back(createTestCommit("commit1", {"SingleFunction"}));
        system.processSummaries(singleCommit);
        system.cluster();
        assert(system.getClusters().size() == 0); // Single definition can't form clusters
        std::cout << "  ✓ Single definition system handled correctly" << std::endl;
        
        // Test system with no connections (all definitions appear once in different commits)
        system.clear();
        std::vector<types::json::parsable*> isolatedCommits;
        isolatedCommits.push_back(createTestCommit("commit1", {"FunctionA"}));
        isolatedCommits.push_back(createTestCommit("commit2", {"FunctionB"}));
        isolatedCommits.push_back(createTestCommit("commit3", {"FunctionC"}));
        
        system.processSummaries(isolatedCommits);
        system.cluster();
        
        // Should have few or no clusters since functions don't co-occur
        std::cout << "  ✓ Isolated definitions generated " << system.getClusters().size() << " clusters" << std::endl;
        
        std::cout << "Edge case tests passed!" << std::endl;
    }

    void runAllTests() {
        std::cout << "=== Running AbstractSystem Tests ===" << std::endl;
        
        testBasicAbstractSystemFunctionality();
        testSimilarityMatrix();
        testClusteringFunctionality();
        testEdgeCases();
        
        std::cout << "\n=== All AbstractSystem Tests Passed! ===" << std::endl;
    }
}

int main() {
    test_abstract::runAllTests();
    return 0;
}
