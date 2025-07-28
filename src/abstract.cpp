#include "abstract.h"

#include <cmath>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <random>

namespace abstract {

    AbstractSystem::AbstractSystem() : totalCommits(0) {
    }

    AbstractSystem::~AbstractSystem() {
        clear();
    }

    void AbstractSystem::processCommits(const std::vector<types::commit>& inputCommits) {
        clear();
        
        commits = inputCommits;
        totalCommits = commits.size();
        
        // Set timeIndex for each commit and process them
        for (size_t i = 0; i < commits.size(); ++i) {
            commits[i].timeIndex = static_cast<unsigned int>(i);
            processCommit(commits[i], commits[i].timeIndex);
        }
        
        // Calculate statistical data
        calculateOccurrences();
        calculateChronicPoints();
    }

    void AbstractSystem::processCommit(const types::commit& commit, unsigned int timeIndex) {
        // Calculate connection weight for this commit
        float weight = calculateConnectionWeight(timeIndex, totalCommits);
        
        // Process ctag definitions
        for (const std::string& symbol : commit.ctagDefinitions) {
            if (!symbol.empty()) {
                addDefinition(symbol, timeIndex, weight);
            }
        }
        
        // Process regex definitions
        for (const std::string& symbol : commit.regexDefinitions) {
            if (!symbol.empty()) {
                addDefinition(symbol, timeIndex, weight);
            }
        }
    }

    void AbstractSystem::addDefinition(const std::string& symbol, unsigned int commitIndex, float weight) {
        // Find or create the definition
        auto it = definitions.find(symbol);
        if (it == definitions.end()) {
            // Create new definition
            types::definition newDef;
            newDef.symbol = symbol;
            newDef.occurrence = 0.0f;
            newDef.chronicPoint = 0.0f;
            definitions[symbol] = newDef;
            it = definitions.find(symbol);
        }
        
        // Add connection to this commit
        types::connection conn;
        conn.Index = commitIndex;
        conn.weight = weight;
        
        // Check if connection to this commit already exists
        bool connectionExists = false;
        for (auto& existingConn : it->second.connections) {
            if (existingConn.Index == commitIndex) {
                // Update existing connection weight (accumulate)
                existingConn.weight += weight;
                connectionExists = true;
                break;
            }
        }
        
        if (!connectionExists) {
            it->second.connections.push_back(conn);
        }
    }

    void AbstractSystem::calculateOccurrences() {
        if (totalCommits == 0) return;
        
        // Calculate the maximum possible weight (if a definition appeared in all commits)
        float maxPossibleWeight = 0.0f;
        for (unsigned int i = 0; i < totalCommits; ++i) {
            maxPossibleWeight += calculateConnectionWeight(i, totalCommits);
        }
        
        // Calculate normalized occurrence for each definition
        for (auto& pair : definitions) {
            float definitionWeight = 0.0f;
            for (const auto& conn : pair.second.connections) {
                definitionWeight += conn.weight;
            }
            
            // Normalize occurrence as a fraction of maximum possible weight
            pair.second.occurrence = maxPossibleWeight > 0.0f ? (definitionWeight / maxPossibleWeight) : 0.0f;
        }
    }

    void AbstractSystem::calculateChronicPoints() {
        if (totalCommits == 0) return;
        
        for (auto& pair : definitions) {
            if (pair.second.connections.empty()) {
                pair.second.chronicPoint = 0.0f;
                continue;
            }
            
            // Calculate weighted center of mass in time
            float weightedSum = 0.0f;
            float totalWeight = 0.0f;
            
            for (const auto& conn : pair.second.connections) {
                // Normalize time index to 0.0-1.0 range
                float normalizedTime = static_cast<float>(conn.Index) / static_cast<float>(totalCommits - 1);
                weightedSum += normalizedTime * conn.weight;
                totalWeight += conn.weight;
            }
            
            // Calculate chronicPoint as weighted average
            pair.second.chronicPoint = totalWeight > 0.0f ? (weightedSum / totalWeight) : 0.0f;
        }
    }

    float AbstractSystem::calculateConnectionWeight(unsigned int timeIndex, size_t numCommits) const {
        if (numCommits <= 1) return 1.0f;
        
        // Weight calculation: (timeIndex + 1) / numCommits
        // This gives a range from 1/numCommits (first commit) to 1.0 (last commit)
        // This ensures all commits get non-zero weights
        return static_cast<float>(timeIndex + 1) / static_cast<float>(numCommits);
    }

    size_t AbstractSystem::getTotalCommits() const {
        return totalCommits;
    }

    AbstractSystem::AbstractStats AbstractSystem::getStatistics() const {
        AbstractStats stats;
        stats.totalDefinitions = definitions.size();
        stats.totalCommits = totalCommits;
        stats.totalConnections = 0;
        
        float sumOccurrence = 0.0f;
        float sumChronicPoint = 0.0f;
        
        for (const auto& pair : definitions) {
            stats.totalConnections += pair.second.connections.size();
            sumOccurrence += pair.second.occurrence;
            sumChronicPoint += pair.second.chronicPoint;
        }
        
        stats.averageOccurrence = stats.totalDefinitions > 0 ? 
            (sumOccurrence / static_cast<float>(stats.totalDefinitions)) : 0.0f;
        stats.averageChronicPoint = stats.totalDefinitions > 0 ? 
            (sumChronicPoint / static_cast<float>(stats.totalDefinitions)) : 0.0f;
        stats.averageConnectionsPerDefinition = stats.totalDefinitions > 0 ? 
            (static_cast<float>(stats.totalConnections) / static_cast<float>(stats.totalDefinitions)) : 0.0f;
        
        return stats;
    }

    void AbstractSystem::clear() {
        definitions.clear();
        commits.clear();
        clusters.clear();
        totalCommits = 0;
    }

    std::vector<float> AbstractSystem::getConnectionWeightsVector(const types::definition& definition) const {
        std::vector<float> weights(totalCommits, 0.0f);
        
        for (const auto& connection : definition.connections) {
            if (connection.Index < totalCommits) {
                weights[connection.Index] = connection.weight;
            }
        }
        
        return weights;
    }

    float AbstractSystem::calculateCosineSimilarity(const types::definition& def1, const types::definition& def2) const {
        std::vector<float> weights1 = getConnectionWeightsVector(def1);
        std::vector<float> weights2 = getConnectionWeightsVector(def2);
        
        if (weights1.size() != weights2.size()) {
            return 0.0f;
        }
        
        // Calculate dot product
        float dotProduct = 0.0f;
        for (size_t i = 0; i < weights1.size(); ++i) {
            dotProduct += weights1[i] * weights2[i];
        }
        
        // Calculate magnitudes
        float magnitude1 = 0.0f;
        float magnitude2 = 0.0f;
        for (size_t i = 0; i < weights1.size(); ++i) {
            magnitude1 += weights1[i] * weights1[i];
            magnitude2 += weights2[i] * weights2[i];
        }
        magnitude1 = std::sqrt(magnitude1);
        magnitude2 = std::sqrt(magnitude2);
        
        // Avoid division by zero
        if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
            return 0.0f;
        }
        
        return dotProduct / (magnitude1 * magnitude2);
    }

    std::vector<std::pair<std::string, types::definition>> AbstractSystem::getDefinitionsVector() const {
        std::vector<std::pair<std::string, types::definition>> result;
        result.reserve(definitions.size());
        
        for (const auto& pair : definitions) {
            result.emplace_back(pair.first, pair.second);
        }
        
        // Sort by symbol name for consistent indexing
        std::sort(result.begin(), result.end(),
                  [](const auto& a, const auto& b) {
                      return a.first < b.first;
                  });
        
        return result;
    }

    void AbstractSystem::cluster() {
        // Clear existing clusters
        clusters.clear();
        
        if (definitions.size() < 2) {
            return; // Not enough definitions to cluster
        }
        
        // Apply all clustering methods

        chronicClustering();

        occurrenceClustering();
    }

    // Sorts definitions into a temporary list of indicies, where the list is sorted via the chronicPoint weight.
    void AbstractSystem::chronicClustering() {
        std::vector<size_t> sortedIndicies = std::vector<size_t>(definitions.size());

        // First lets populate the normal indicies into the zeroed list.
        std::iota(sortedIndicies.begin(), sortedIndicies.end(), 0);

        // Now lets sort the indicies based on the chronicPoint of each definition.
        std::sort(
            sortedIndicies.begin(), sortedIndicies.end(), 
            [this](size_t a, size_t b) {
                auto itA = definitions.begin();
                std::advance(itA, a);
                auto itB = definitions.begin();
                std::advance(itB, b);
                return itA->second.chronicPoint < itB->second.chronicPoint;
            }
        );

        // This is the threshold, indicies and their distances over this threshold, will not be counted as in same cluster.
        float averageChronicDistance = getAverageChronicRadius(sortedIndicies);

        // Now lets create the cluster.
        types::cluster::chronic* currentCluster = new types::cluster::chronic();

        for (size_t i = 0; i < sortedIndicies.size() - 1; i++) {
            auto itA = definitions.begin();
            std::advance(itA, sortedIndicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, sortedIndicies[i + 1]);

            float distance = std::abs(itA->second.chronicPoint - itB->second.chronicPoint);

            if (distance > averageChronicDistance) {
                // If the distance is greater than the threshold, finalize the current cluster
                if (!currentCluster->definitions.empty()) {
                    clusters.push_back(currentCluster);
                }

                // Start a new cluster
                currentCluster = new types::cluster::chronic();
            }
            else {
                // Add the definition to the current cluster
                currentCluster->definitions.push_back(&itA->second);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }


    }

    // Basically same as the chronic clustering but for occurrences of definitions
    void AbstractSystem::occurrenceClustering() {
        std::vector<size_t> sortedIndicies = std::vector<size_t>(definitions.size());

        // First lets populate the normal indicies into the zeroed list.
        std::iota(sortedIndicies.begin(), sortedIndicies.end(), 0);

        // Now lets sort the indicies based on the occurrence of each definition.
        std::sort(
            sortedIndicies.begin(), sortedIndicies.end(), 
            [this](size_t a, size_t b) {
                auto itA = definitions.begin();
                std::advance(itA, a);
                auto itB = definitions.begin();
                std::advance(itB, b);
                return itA->second.occurrence < itB->second.occurrence;
            }
        );

        // This is the threshold, indicies and their distances over this threshold, will not be counted as in same cluster.
        float averageOccurrenceDistance = getAverageOccurrenceRadius(sortedIndicies);

        // Now lets create the cluster.
        types::cluster::occurrence* currentCluster = new types::cluster::occurrence();

        for (size_t i = 0; i < sortedIndicies.size() - 1; i++) {
            auto itA = definitions.begin();
            std::advance(itA, sortedIndicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, sortedIndicies[i + 1]);

            float distance = std::abs(itA->second.occurrence - itB->second.occurrence);

            if (distance > averageOccurrenceDistance) {
                // If the distance is greater than the threshold, finalize the current cluster
                if (!currentCluster->definitions.empty()) {
                    clusters.push_back(currentCluster);
                }

                // Start a new cluster
                currentCluster = new types::cluster::occurrence();
            }
            else {
                // Add the definition to the current cluster
                currentCluster->definitions.push_back(&itA->second);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }
    }
    
    const std::vector<types::cluster::base*>& AbstractSystem::getClusters() const {
        return clusters;
    }
    
    std::vector<types::cluster::base*> AbstractSystem::getClustersByType(types::cluster::Type type) const {
        std::vector<types::cluster::base*> result;
        for (const auto cluster : clusters) {
            if (cluster->type == type) {
                result.push_back(cluster);
            }
        }
        return result;
    }
    
    std::vector<std::vector<float>> AbstractSystem::buildSimilarityMatrix() const {
        auto defsVector = getDefinitionsVector();
        size_t n = defsVector.size();
        std::vector<std::vector<float>> matrix(n, std::vector<float>(n, 0.0f));
        
        for (size_t i = 0; i < n; ++i) {
            matrix[i][i] = 1.0f; // Self-similarity is 1.0
            for (size_t j = i + 1; j < n; ++j) {
                float similarity = calculateCosineSimilarity(defsVector[i].second, defsVector[j].second);
                matrix[i][j] = similarity;
                matrix[j][i] = similarity; // Symmetric matrix
            }
        }
        
        return matrix;
    }

    float AbstractSystem::getAverageChronicRadius(const std::vector<size_t>& indicies) {
        // Checks for each pair of i and i+1, and calculates their chronicPoint distance, returning the average of these distances.
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            auto itA = definitions.begin();
            std::advance(itA, indicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, indicies[i + 1]);
            totalDistance += std::abs(itA->second.chronicPoint - itB->second.chronicPoint);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    float AbstractSystem::getAverageOccurrenceRadius(const std::vector<size_t>& indicies) {
        // Checks for each pair of i and i+1, and calculates their occurrence distance, returning the average of these distances.
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        // NOTE: Because, we already sort the indicies via occurrence, thus O(n^2) is not necessary.
        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            auto itA = definitions.begin();
            std::advance(itA, indicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, indicies[i + 1]);
            totalDistance += std::abs(itA->second.occurrence - itB->second.occurrence);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    // Legacy methods for backward compatibility with tests
    const std::unordered_map<std::string, types::definition>& AbstractSystem::getDefinitions() const {
        return definitions;
    }
    
    const types::definition* AbstractSystem::getDefinition(const std::string& symbol) const {
        auto it = definitions.find(symbol);
        return (it != definitions.end()) ? &it->second : nullptr;
    }
    
    std::vector<types::definition> AbstractSystem::getDefinitionsByOccurrence() const {
        std::vector<std::pair<std::string, types::definition>> defsVec = getDefinitionsVector();
        std::vector<types::definition> result;
        
        // Sort by occurrence (descending)
        std::sort(defsVec.begin(), defsVec.end(), 
                  [](const std::pair<std::string, types::definition>& a, 
                     const std::pair<std::string, types::definition>& b) {
                      return a.second.occurrence > b.second.occurrence;
                  });
        
        for (const auto& pair : defsVec) {
            result.push_back(pair.second);
        }
        
        return result;
    }
    
    std::vector<types::definition> AbstractSystem::findTemporallyRelatedDefinitions(const std::string& symbol, float threshold) const {
        std::vector<types::definition> result;
        auto it = definitions.find(symbol);
        if (it == definitions.end()) return result;
        
        const auto& targetDef = it->second;
        
        for (const auto& pair : definitions) {
            if (pair.first == symbol) continue;
            
            float chronicDiff = std::abs(targetDef.chronicPoint - pair.second.chronicPoint);
            if (chronicDiff <= threshold) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }
    
    std::vector<types::definition> AbstractSystem::findCoOccurringDefinitions(const std::string& symbol, float threshold) const {
        std::vector<types::definition> result;
        auto it = definitions.find(symbol);
        if (it == definitions.end()) return result;
        
        const auto& targetDef = it->second;
        
        for (const auto& pair : definitions) {
            if (pair.first == symbol) continue;
            
            float similarity = calculateCosineSimilarity(targetDef, pair.second);
            if (similarity >= threshold) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }

}
