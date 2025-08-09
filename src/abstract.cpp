#include "abstract.h"

#include <cmath>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <random>

namespace types {
    std::vector<float> cluster::getVector() {
        if (!cachedVector.empty())
            return cachedVector;

        // Handle empty clusters
        if (definitions.empty()) {
            cachedVector = {0, 0, 0, 0};
            return cachedVector;
        }

        std::vector<float> result = {0, 0, 0, 0};

        for (auto* def : definitions) {
            if (def == nullptr) continue; // Skip null pointers
            
            std::vector<float> defVector = def->getVector();

            if (defVector.size() != 4) {
                std::cerr << "Error: Definition vector size is not 4!" << std::endl;
                continue;
            }
            
            for (size_t i = 0; i < defVector.size(); ++i) {
                result[i] += defVector[i];
            }
        }

        // Normalize the result vector
        float norm = std::sqrt(std::inner_product(result.begin(), result.end(), result.begin(), 0.0f));
        if (norm > 0) {
            for (auto& val : result) {
                val /= norm;
            }
        }

        cachedVector = result;

        return result;
    }

    float cluster::getVariance() {
        // Var(V)= 1/N Sum(0->i)[||v i-q||^2]
        // q is a centroid.
        // Calculate variance within this cluster

        if (definitions.empty()) {
            return 0.0f;
        }

        // Pre-cache all definition vectors and determine vector size
        std::vector<std::vector<float>> definitionVectors;
        definitionVectors.reserve(definitions.size());
        size_t vectorSize = 0;
        
        for (auto* def : definitions) {
            if (def == nullptr) continue; // Skip null pointers
            
            std::vector<float> vec = def->getVector();
            if (!vec.empty()) {
                if (vectorSize == 0) {
                    vectorSize = vec.size();
                } else if (vec.size() != vectorSize) {
                    continue; // Skip inconsistent vector sizes
                }
                definitionVectors.push_back(std::move(vec));
            }
        }
        
        if (definitionVectors.empty() || vectorSize == 0) {
            return 0.0f;
        }

        // Calculate cluster centroid
        std::vector<float> clusterCentroid(vectorSize, 0.0f);
        
        // Calculate centroid
        for (const auto& vec : definitionVectors) {
            for (size_t i = 0; i < vectorSize; ++i) {
                clusterCentroid[i] += vec[i];
            }
        }
        
        float invValidDefs = 1.0f / static_cast<float>(definitionVectors.size());
        for (size_t i = 0; i < vectorSize; ++i) {
            clusterCentroid[i] *= invValidDefs;
        }
        
        // Calculate variance within this cluster
        float variance = 0.0f;
        for (const auto& vec : definitionVectors) {
            float squaredDistance = 0.0f;
            for (size_t i = 0; i < vectorSize; ++i) {
                float diff = vec[i] - clusterCentroid[i];
                squaredDistance += diff * diff;
            }
            variance += squaredDistance;
        }
        variance *= invValidDefs;

        return variance;
    }
}

namespace abstract {

    base::base() : totalSummaries(0) {
    }

    base::~base() {
        clear();
    }

    void base::processSummaries(const std::vector<types::json::parsable*>& inputSummaries) {
        // Convert parsable pointers to summary pointers with proper casting
        summaries.clear();
        summaries.reserve(inputSummaries.size());
        for (auto* obj : inputSummaries) {
            types::summary* summary = dynamic_cast<types::summary*>(obj);
            if (summary) {
                summaries.push_back(summary);
            }
        }
        totalSummaries = summaries.size();
        
        // Set timeIndex for each summary and process them
        for (size_t i = 0; i < summaries.size(); ++i) {
            summaries[i]->timeIndex = static_cast<unsigned int>(i);
            processSummary(summaries[i], summaries[i]->timeIndex);
        }
        
        // Calculate statistical data
        calculateOccurrences();
        calculateChronicPoints();
    }

    void base::processCommits(const std::vector<types::json::parsable*>& inputCommits) {
        commits.clear();
        commits.reserve(inputCommits.size());

        for (auto* obj : inputCommits) {
            types::commit* commit = dynamic_cast<types::commit*>(obj);
            if (commit) {
                commits.push_back(commit);
            }
        }
        totalCommits = commits.size();

        calculateSummaryIndiciesForCommits();
        calculateFileNodes();
    }

    void base::processSummary(const types::summary* summary, unsigned int timeIndex) {
        // Calculate connection weight for this summary
        float weight = calculateConnectionWeight(timeIndex, totalSummaries);
        
        // Process ctag definitions
        for (const std::string& symbol : summary->ctagDefinitions) {
            if (!symbol.empty()) {
                addDefinition(symbol, timeIndex, weight);
            }
        }
        
        // Process regex definitions
        for (const std::string& symbol : summary->regexDefinitions) {
            if (!symbol.empty()) {
                addDefinition(symbol, timeIndex, weight);
            }
        }
    }

    void base::addDefinition(const std::string& symbol, unsigned int commitIndex, float weight) {
        // Find or create the definition
        auto it = definitions.find(symbol);
        if (it == definitions.end()) {
            // Create new definition
            types::definition newDef;
            newDef.symbol = symbol;
            newDef.CommitFrequency = 0.0f;
            newDef.chronicPoint = 0.0f;
            definitions[symbol] = newDef;
            it = definitions.find(symbol);
        }
        
        // Add connection to this summary
        types::connection conn;
        conn.Index = commitIndex;
        conn.weight = weight;
        
        // Check if connection to this summary already exists
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

    void base::calculateOccurrences() {
        if (totalSummaries == 0) return;
        
        // Calculate the maximum possible weight (if a definition appeared in all commits)
        float maxPossibleWeight = 0.0f;
        for (unsigned int i = 0; i < totalSummaries; ++i) {
            maxPossibleWeight += calculateConnectionWeight(i, totalSummaries);
        }
        
        // Calculate normalized occurrence for each definition
        for (auto& pair : definitions) {
            float definitionWeight = 0.0f;
            for (const auto& conn : pair.second.connections) {
                definitionWeight += conn.weight;
            }
            
            // Normalize occurrence as a fraction of maximum possible weight
            pair.second.CommitFrequency = maxPossibleWeight > 0.0f ? (definitionWeight / maxPossibleWeight) : 0.0f;
        }
    }

    void base::calculateChronicPoints() {
        if (totalSummaries == 0) return;
        
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
                float normalizedTime = static_cast<float>(conn.Index) / static_cast<float>(totalSummaries - 1);
                weightedSum += normalizedTime * conn.weight;
                totalWeight += conn.weight;
            }
            
            // Calculate chronicPoint as weighted average
            pair.second.chronicPoint = totalWeight > 0.0f ? (weightedSum / totalWeight) : 0.0f;
        }
    }

    void base::calculateSummaryIndiciesForCommits() {
        // First we'll go through the summaries, creating an quick hashmap with summary::id and then finding commits with the identical hash id
        std::unordered_map<std::string, unsigned int> summaryIndexMap;

        for (unsigned int i = 0; i < summaries.size(); ++i) {
            summaryIndexMap[summaries[i]->id] = i;
        }

        // Now we can assign the summary index to each commit
        for (auto& commit : commits) {
            auto it = summaryIndexMap.find(commit->id);
            if (it != summaryIndexMap.end()) {
                commit->summaryIndex = it->second;
            }
        }
    }

    void base::calculateFileNodes() {
        std::unordered_map<std::string, std::vector<types::commit*>> fileToCommits;
        std::vector<std::string> markForRemoval;
        std::unordered_map<std::string, std::vector<std::string>> renameHistory;

        // First combine same file located commits.
        for (auto c : commits) {
            for (auto h : c->hunks) {
                fileToCommits[h.file].push_back(c);
            }
        }

        // Now we'll combine files where they have been renamed, this can be found when in a commit one hunk is on file_status "deleted" and other one is "added", and the old_text of the removes is the same as the new_text of the added one
        for (auto& commit : commits) {
            std::vector<types::hunk*> removed;
            std::vector<types::hunk*> added;

            for (auto& hunk : commit->hunks) {
                if (hunk.changeType == "deleted") {
                    removed.push_back(&hunk);
                } else if (hunk.changeType == "added") {
                    added.push_back(&hunk);
                }
            }

            for (size_t r = 0; r < removed.size(); r++) {
                for (size_t a = 0; a < added.size(); a++) {
                    // If we found an removed and an added hunk in the same commit, we should check if they are basically the same file, just renamed.
                    if (removed[r]->oldStart == added[a]->newStart && removed[r]->oldLines == added[a]->newLines) { // We cant really compare the actual file contents, since if the file has been renamed, it's include file names have also probably been renamed.
                        // Now we can add the commits of the oldFile to the newFile commits and then remove the oldFile from the map
                        fileToCommits[added[a]->file].insert(fileToCommits[added[a]->file].end(), fileToCommits[removed[r]->file].begin(), fileToCommits[removed[r]->file].end());
                        
                        markForRemoval.push_back(removed[r]->file); // Mark the old file for removal
        
                        renameHistory[added[a]->file].push_back(removed[r]->file); // Add the rename history for the new file

                        // now we remove the added indicies
                        added.erase(added.begin() + a);
                        // We do not need to remove the removed list indicies, since we wont go through them again.
                        
                        break;
                    }
                }
            }
        }

        // Now remove the marked files from the map
        for (const auto& file : markForRemoval) {
            fileToCommits.erase(file);
        }

        for (auto f : fileToCommits) {
            types::definition fileDefinition;  // Create on stack instead of heap
            fileDefinition.symbol = f.first;

            // Add the history renames
            fileDefinition.history = renameHistory[f.first];

            for (auto commit : f.second) {
                // Create commit connections via the summaryIndex in commits
                types::connection conn;

                conn.Index = commit->summaryIndex;
                conn.weight = calculateConnectionWeight(commit->summaryIndex, totalSummaries);

                fileDefinition.connections.push_back(conn);
            }

            files.push_back(fileDefinition);
        }

        // Recalculate statistics for all definitions including the new file definitions
        calculateOccurrences();
        calculateChronicPoints();
    }

    float base::calculateConnectionWeight(unsigned int timeIndex, size_t numCommits) const {
        if (numCommits <= 1) return 1.0f;
        
        // Weight calculation: (timeIndex + 1) / numCommits
        // This gives a range from 1/numCommits (first summary) to 1.0 (last summary)
        // This ensures all commits get non-zero weights
        return static_cast<float>(timeIndex + 1) / static_cast<float>(numCommits);
    }

    size_t base::getTotalCommits() const {
        return totalSummaries;
    }

    float base::getEntropy() {
        // Based on: E[||v_i - v_j||^2]
        // Runs the entropy calculation on definition listing only and then again for the clusters, returning the 1D vector of the gained anti-entropy gained via the clusters.
        // result == 0.0f -> identical entropy between definitions and clusters
        // result < 0.0f ->  clustering brings more entropy than anti-entropy.
        // result > 0.0f -> clustering brings less entropy than anti-entropy.

        if (definitions.empty()) {
            return 0.0f;
        }

        // Pre-cache all definition vectors for performance
        std::vector<std::vector<float>> definitionVectors;
        definitionVectors.reserve(definitions.size());
        size_t vectorSize = 0;
        
        for (auto& pair : definitions) {
            std::vector<float> vec = pair.second.getVector();
            if (!vec.empty()) {
                if (vectorSize == 0) {
                    vectorSize = vec.size();
                } else if (vec.size() != vectorSize) {
                    continue; // Skip inconsistent vector sizes
                }
                definitionVectors.push_back(std::move(vec));
            }
        }
        
        if (definitionVectors.empty()) {
            return 0.0f;
        }

        // Calculate entropy for definitions - optimized with cached vectors
        float definitionEntropy = 0.0f;
        size_t defCount = 0;
        
        for (size_t i = 0; i < definitionVectors.size(); ++i) {
            for (size_t j = i + 1; j < definitionVectors.size(); ++j) {
                const auto& vec1 = definitionVectors[i];
                const auto& vec2 = definitionVectors[j];
                
                // Calculate squared Euclidean distance ||v_i - v_j||^2 - inline for performance
                float squaredDistance = 0.0f;
                for (size_t k = 0; k < vectorSize; ++k) {
                    float diff = vec1[k] - vec2[k];
                    squaredDistance += diff * diff;
                }
                
                definitionEntropy += squaredDistance;
                defCount++;
            }
        }
        
        // Normalize by number of pairs
        if (defCount > 0) {
            definitionEntropy /= static_cast<float>(defCount);
        }

        // Calculate entropy for clusters - optimized with cached vectors
        float clusterEntropy = 0.0f;
        size_t clusterCount = 0;
        
        if (clusters.size() >= 2) {
            // Pre-cache cluster vectors
            std::vector<std::vector<float>> clusterVectors;
            clusterVectors.reserve(clusters.size());
            
            for (auto* cluster : clusters) {
                if (cluster != nullptr && !cluster->definitions.empty()) {
                    std::vector<float> vec = cluster->getVector();
                    if (vec.size() == vectorSize) {
                        clusterVectors.push_back(std::move(vec));
                    }
                }
            }
            
            // Calculate pairwise distances between clusters
            for (size_t i = 0; i < clusterVectors.size(); ++i) {
                for (size_t j = i + 1; j < clusterVectors.size(); ++j) {
                    const auto& vec1 = clusterVectors[i];
                    const auto& vec2 = clusterVectors[j];
                    
                    // Calculate squared Euclidean distance ||v_i - v_j||^2 - inline for performance
                    float squaredDistance = 0.0f;
                    for (size_t k = 0; k < vectorSize; ++k) {
                        float diff = vec1[k] - vec2[k];
                        squaredDistance += diff * diff;
                    }
                    
                    clusterEntropy += squaredDistance;
                    clusterCount++;
                }
            }
            
            // Normalize by number of pairs
            if (clusterCount > 0) {
                clusterEntropy /= static_cast<float>(clusterCount);
            }
        }

        // Return the difference: positive means clustering reduces entropy (anti-entropy)
        return definitionEntropy - clusterEntropy;
    }

    float base::getVariance() {
        // Var(V)= 1/N Sum(0->i)[||v_i-q||^2]
        // q is a centroid.
        // Variance Gain = 1 - (clusterVariance / definitionVariance);

        if (definitions.empty()) {
            return 0.0f;
        }

        // Pre-cache all definition vectors and determine vector size
        std::vector<std::vector<float>> definitionVectors;
        definitionVectors.reserve(definitions.size());
        size_t vectorSize = 0;
        
        for (auto& pair : definitions) {
            std::vector<float> vec = pair.second.getVector();
            if (!vec.empty()) {
                if (vectorSize == 0) {
                    vectorSize = vec.size();
                } else if (vec.size() != vectorSize) {
                    continue; // Skip inconsistent vector sizes
                }
                definitionVectors.push_back(std::move(vec));
            }
        }
        
        if (definitionVectors.empty() || vectorSize == 0) {
            return 0.0f;
        }

        // Calculate definition centroid and variance in single pass
        std::vector<float> definitionCentroid(vectorSize, 0.0f);
        
        // Calculate centroid
        for (const auto& vec : definitionVectors) {
            for (size_t i = 0; i < vectorSize; ++i) {
                definitionCentroid[i] += vec[i];
            }
        }
        
        float invValidDefs = 1.0f / static_cast<float>(definitionVectors.size());
        for (size_t i = 0; i < vectorSize; ++i) {
            definitionCentroid[i] *= invValidDefs;
        }
        
        // Calculate definition variance
        float definitionVariance = 0.0f;
        for (const auto& vec : definitionVectors) {
            float squaredDistance = 0.0f;
            for (size_t i = 0; i < vectorSize; ++i) {
                float diff = vec[i] - definitionCentroid[i];
                squaredDistance += diff * diff;
            }
            definitionVariance += squaredDistance;
        }
        definitionVariance *= invValidDefs;

        // Calculate intra-cluster variance with cached cluster centroids
        float intraVariance = 0.0f;
        size_t totalPoints = 0;

        for (auto* cluster : clusters) {
            if (cluster == nullptr || cluster->definitions.empty()) {
                continue;
            }
            
            // Cache cluster centroid
            std::vector<float> clusterCentroid = cluster->getVector();
            if (clusterCentroid.size() != vectorSize) {
                continue;
            }
            
            // Process all definitions in this cluster
            for (auto* def : cluster->definitions) {
                std::vector<float> vec = def->getVector();
                if (vec.size() == vectorSize) {
                    float sqDist = 0.0f;
                    for (size_t i = 0; i < vectorSize; ++i) {
                        float diff = vec[i] - clusterCentroid[i];
                        sqDist += diff * diff;
                    }
                    intraVariance += sqDist;
                    totalPoints++;
                }
            }
        }

        if (totalPoints == 0) {
            return 0.0f;
        }
        
        intraVariance /= static_cast<float>(totalPoints);
        
        // Variance gain: 1 - (intra-cluster variance / overall variance)
        if (definitionVariance == 0.0f) {
            return intraVariance == 0.0f ? 0.0f : 1.0f;
        }
        
        return 1.0f - (intraVariance / definitionVariance);
    }

    float base::getAverageClusterSize() {
        if (clusters.empty()) {
            return 0.0f;
        }

        // Single pass calculation
        size_t totalDefinitions = 0;
        size_t validClusters = 0;

        for (auto* cluster : clusters) {
            if (cluster != nullptr) {
                totalDefinitions += cluster->definitions.size();
                if (cluster->definitions.size() > 0) {  // Only count clusters with definitions
                    validClusters++;
                }
            }
        }

        return validClusters > 0 ? static_cast<float>(totalDefinitions) / static_cast<float>(validClusters) : 0.0f;
    }

    float base::getSilhouetteScore() {
        // s = (b - a) / max(a, b)
        // where:
        // a = mean intra-cluster distance
        // b = mean nearest-cluster distance
        
        if (clusters.empty() || clusters.size() < 2) {
            return 0.0f;  // Need at least 2 clusters for meaningful silhouette score
        }

        // Get the vector size from the first valid definition (all vectors should be same size)
        size_t vectorSize = 0;
        for (auto* cluster : clusters) {
            if (cluster != nullptr && !cluster->definitions.empty()) {
                for (auto* def : cluster->definitions) {
                    std::vector<float> vec = def->getVector();
                    if (!vec.empty()) {
                        vectorSize = vec.size();
                        break;
                    }
                }
                if (vectorSize > 0) break;
            }
        }
        
        if (vectorSize == 0) {
            return 0.0f;
        }

        // Pre-filter valid clusters and cache their vectors
        std::vector<std::pair<types::cluster*, std::vector<std::vector<float>>>> validClusters;
        validClusters.reserve(clusters.size());
        
        for (auto* cluster : clusters) {
            if (cluster != nullptr && !cluster->definitions.empty()) {
                std::vector<std::vector<float>> clusterVectors;
                clusterVectors.reserve(cluster->definitions.size());
                
                for (auto* def : cluster->definitions) {
                    std::vector<float> vec = def->getVector();
                    if (vec.size() == vectorSize) {
                        clusterVectors.push_back(std::move(vec));
                    }
                }
                
                if (!clusterVectors.empty()) {
                    validClusters.emplace_back(cluster, std::move(clusterVectors));
                }
            }
        }
        
        if (validClusters.size() < 2) {
            return 0.0f;
        }

        float totalSilhouetteScore = 0.0f;
        size_t totalPoints = 0;

        // For each valid cluster
        for (size_t clusterIdx = 0; clusterIdx < validClusters.size(); ++clusterIdx) {
            const auto& currentClusterVectors = validClusters[clusterIdx].second;
            
            // For each point in the cluster
            for (size_t pointIdx = 0; pointIdx < currentClusterVectors.size(); ++pointIdx) {
                const auto& defVector = currentClusterVectors[pointIdx];

                // Calculate mean intra-cluster distance (a) - optimized
                float intraClusterDistance = 0.0f;
                size_t intraCount = 0;
                
                for (size_t otherIdx = 0; otherIdx < currentClusterVectors.size(); ++otherIdx) {
                    if (otherIdx != pointIdx) {
                        const auto& otherVector = currentClusterVectors[otherIdx];
                        
                        // Inline distance calculation for better performance - dynamic size
                        float distanceSquared = 0.0f;
                        for (size_t i = 0; i < vectorSize; ++i) {
                            float diff = defVector[i] - otherVector[i];
                            distanceSquared += diff * diff;
                        }
                        intraClusterDistance += std::sqrt(distanceSquared);
                        intraCount++;
                    }
                }
                
                float a = (intraCount > 0) ? (intraClusterDistance / static_cast<float>(intraCount)) : 0.0f;

                // Calculate mean nearest-cluster distance (b) - optimized
                float minInterClusterDistance = std::numeric_limits<float>::max();
                
                for (size_t otherClusterIdx = 0; otherClusterIdx < validClusters.size(); ++otherClusterIdx) {
                    if (otherClusterIdx == clusterIdx) continue;
                    
                    const auto& otherClusterVectors = validClusters[otherClusterIdx].second;
                    
                    float interClusterDistance = 0.0f;
                    size_t interCount = otherClusterVectors.size();
                    
                    for (const auto& otherVector : otherClusterVectors) {
                        // Inline distance calculation for better performance - dynamic size
                        float distanceSquared = 0.0f;
                        for (size_t i = 0; i < vectorSize; ++i) {
                            float diff = defVector[i] - otherVector[i];
                            distanceSquared += diff * diff;
                        }
                        interClusterDistance += std::sqrt(distanceSquared);
                    }
                    
                    float avgInterDistance = interClusterDistance / static_cast<float>(interCount);
                    minInterClusterDistance = std::min(minInterClusterDistance, avgInterDistance);
                }
                
                float b = (minInterClusterDistance != std::numeric_limits<float>::max()) ? minInterClusterDistance : 0.0f;

                // Calculate silhouette score for this point: s = (b - a) / max(a, b)
                float maxAB = std::max(a, b);
                if (maxAB > 0.0f) {
                    float silhouetteScore = (b - a) / maxAB;
                    totalSilhouetteScore += silhouetteScore;
                    totalPoints++;
                }
            }
        }

        // Return average silhouette score
        return (totalPoints > 0) ? (totalSilhouetteScore / static_cast<float>(totalPoints)) : 0.0f;
    }

    base::abstractStats base::getStatistics() const {
        abstractStats stats;
        stats.totalDefinitions = definitions.size();
        stats.totalCommits = totalSummaries;
        stats.totalConnections = 0;
        
        float sumOccurrence = 0.0f;
        float sumChronicPoint = 0.0f;
        
        for (const auto& pair : definitions) {
            stats.totalConnections += pair.second.connections.size();
            sumOccurrence += pair.second.CommitFrequency;
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

    void base::clear() {
        definitions.clear();
        summaries.clear();
        clusters.clear();
        totalSummaries = 0;
    }

    std::vector<float> base::getConnectionWeightsVector(const types::definition& definition) const {
        std::vector<float> weights(totalSummaries, 0.0f);
        
        for (const auto& connection : definition.connections) {
            if (connection.Index < totalSummaries) {
                weights[connection.Index] = connection.weight;
            }
        }
        
        return weights;
    }

    float base::calculateCosineSimilarity(const types::definition& def1, const types::definition& def2) const {
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

    std::vector<std::pair<std::string, types::definition>> base::getDefinitionsVector() {
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

    void base::cluster() {
        // Clear existing clusters
        clusters.clear();
        
        if (definitions.size() < 2) {
            return; // Not enough definitions to cluster
        }
        
        // Apply all clustering methods

        namespaceClustering();

        lokiClustering();

        chronicClustering();

        occurrenceClustering();

        resonanceHubClustering();
        
        dissonanceHubClustering();

        fileClustering();

        gradientDecent();   // Whole program optimization to find lower cost for each cluster and their threshold variance.
    }

    std::vector<std::string> slice(std::string input, std::vector<std::string> delimiters) {
        std::vector<std::string> parts;
        size_t start = 0;

        while (start < input.size()) {
            size_t minPos = std::string::npos;
            size_t delimLen = 0;
            // Find the earliest occurrence of any delimiter
            for (const auto& delim : delimiters) {
                if (delim.empty()) continue;
                size_t pos = input.find(delim, start);
                if (pos != std::string::npos && (minPos == std::string::npos || pos < minPos)) {
                    minPos = pos;
                    delimLen = delim.length();
                }
            }
            if (minPos == std::string::npos) {
                parts.push_back(input.substr(start));
                break;
            } else {
                parts.push_back(input.substr(start, minPos - start));
                start = minPos + delimLen;
            }
        }

        return parts;
    }

    void base::namespaceClustering() {
        for (auto& def : definitions) {

            // Slice symbol
            std::vector<std::string> parts = slice(def.first, {"::", "/"});
            if (parts.size() < 2) {
                continue;
            }

            types::context* root = findContext(types::getNormalSymbol(parts.front()));
            if (!root) {
                root = new types::context(parts.front());
                clusters.push_back(root);
            }

            for (size_t j = 1; j < parts.size() - 1; ++j) {
                std::string currentName = parts[j];

                if (currentName == root->symbol)
                    continue;

                types::context* next = root->find(currentName);

                if (!next) {
                    next = new types::context(currentName);
                    root->definitions.push_back(next);
                }

                root = next;
            }

            root->definitions.push_back(&def.second);

            // Now modify the symbol and reinsert
            def.second.symbol = parts.back();
            definitions[parts.back()] = def.second;
        }
    }

    void base::lokiClustering() {
        // Contains in chronological order of all definitions which contain similar name but in different naming conventions.
        std::unordered_map<std::string, std::vector<types::definition*>> related;

        // Now we'll go through the definitions and put them into the realted map via the normalSymbol function
        for (auto& pair : definitions) {
            related[types::getNormalSymbol(pair.first)].push_back(&pair.second);
        }

        for (auto rel : related) {
            if (rel.second.size() < 2)
                continue;

            types::definition* inheritor = rel.second.back();   // The last added definition should be the newest occurrence of the definition.

            for (size_t i = 0; i < rel.second.size() -1; i++) {
                inheritor->history.push_back(rel.second[i]->symbol); // Add the prior definition to the history of the inheritor

                // Combine connections between the prior definitions
                for (const auto& conn : rel.second[i]->connections) {
                    // Add connection to the inheritor definition
                    inheritor->connections.push_back(conn);
                }
            }
        }

        // Now we'll go through and remove from the original definitions listing all related on each related map but the last definition
        for (auto& rel : related) {
            if (rel.second.size() < 2)
                continue;

            // Keep the last definition, remove all others
            for (size_t i = 0; i < rel.second.size() - 1; ++i) {
                definitions.erase(rel.second[i]->symbol);
            }
        }
    }

    // Sorts definitions into a temporary list of indicies, where the list is sorted via the chronicPoint weight.
    void base::chronicClustering() {
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
        types::cluster* currentCluster = new types::cluster(types::node::Type::CHRONIC);

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
                currentCluster = new types::cluster(types::node::Type::CHRONIC);
            }
            else {
                // Add the definition to the current cluster
                itA->second.ClusterFrequency++;
                currentCluster->definitions.push_back(&itA->second);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }
    }

    // Basically same as the chronic clustering but for occurrences of definitions
    void base::occurrenceClustering() {
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
                return itA->second.CommitFrequency < itB->second.CommitFrequency;
            }
        );

        // This is the threshold, indicies and their distances over this threshold, will not be counted as in same cluster.
        float averageOccurrenceDistance = getAverageOccurrenceRadius(sortedIndicies);

        // Now lets create the cluster.
        types::cluster* currentCluster = new types::cluster(types::node::Type::OCCURRENCE);

        for (size_t i = 0; i < sortedIndicies.size() - 1; i++) {
            auto itA = definitions.begin();
            std::advance(itA, sortedIndicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, sortedIndicies[i + 1]);

            float distance = std::abs(itA->second.CommitFrequency - itB->second.CommitFrequency);

            if (distance > averageOccurrenceDistance) {
                // If the distance is greater than the threshold, finalize the current cluster
                if (!currentCluster->definitions.empty()) {
                    clusters.push_back(currentCluster);
                }

                // Start a new cluster
                currentCluster = new types::cluster(types::node::Type::OCCURRENCE);
            }
            else {
                // Add the definition to the current cluster
                itA->second.ClusterFrequency++;
                currentCluster->definitions.push_back(&itA->second);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }
    }
    
    // Basically the same as the chronic and occurrence clustering but for the clusters themselves
    void base::dissonanceHubClustering() {
        if (clusters.empty())
            return;

        std::vector<size_t> sortedIndicies = std::vector<size_t>(clusters.size());

        // First lets populate the normal indicies into the zeroed list.
        std::iota(sortedIndicies.begin(), sortedIndicies.end(), 0);

        // Now lets sort the indicies based on the radius of clusters
        std::sort(
            sortedIndicies.begin(), sortedIndicies.end(), 
            [this](size_t a, size_t b) {
                auto itA = clusters.begin();
                std::advance(itA, a);
                auto itB = clusters.begin();
                std::advance(itB, b);
                return (*itA)->radius < (*itB)->radius;
            }
        );

        float averageClusterRadius = getAverageClusterRadius(sortedIndicies);

        // Now lets create the cluster.
        types::cluster* currentCluster = new types::cluster(types::node::Type::DISSONANCE_HUB);

        for (size_t i = 0; i < sortedIndicies.size() - 1; i++) {
            auto itA = clusters.begin();
            std::advance(itA, sortedIndicies[i]);
            auto itB = clusters.begin();
            std::advance(itB, sortedIndicies[i + 1]);

            float distance = std::abs((*itA)->radius - (*itB)->radius);

            if (distance > averageClusterRadius) {
                // If the distance is greater than the threshold, finalize the current cluster
                if (!currentCluster->definitions.empty()) {
                    clusters.push_back(currentCluster);
                }

                // Start a new cluster
                currentCluster = new types::cluster(types::node::Type::DISSONANCE_HUB);
            }
            else {
                // Add the cluster to the current hub cluster
                currentCluster->definitions.push_back(*itA);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }
    }

    void base::resonanceHubClustering() {
        if (clusters.empty())
            return;

        std::vector<size_t> sortedIndicies = std::vector<size_t>(clusters.size());

        // First lets populate the normal indicies into the zeroed list.
        std::iota(sortedIndicies.begin(), sortedIndicies.end(), 0);

        // Calculate average dot product similarity for each cluster based on its definitions
        std::vector<float> averageClusterSimilarity(clusters.size(), 0.0f);
        
        for (size_t i = 0; i < clusters.size(); ++i) {
            if (clusters[i]->definitions.empty()) {
                averageClusterSimilarity[i] = 0.0f;
                continue;
            }

            float totalSimilarity = 0.0f;
            int validComparisons = 0;

            // Calculate average dot product similarity within the cluster
            for (size_t defA = 0; defA < clusters[i]->definitions.size(); ++defA) {

                for (size_t defB = defA + 1; defB < clusters[i]->definitions.size(); ++defB) {

                    std::vector<float> dotProductResult;
                    float similarity = dotProduct(clusters[i]->definitions[defA], clusters[i]->definitions[defB], dotProductResult);
                    totalSimilarity += similarity;
                    validComparisons++;
                }
            }

            averageClusterSimilarity[i] = validComparisons > 0 ? (totalSimilarity / validComparisons) : 0.0f;
        }

        // Sort indices based on average cluster similarity
        std::sort(
            sortedIndicies.begin(), sortedIndicies.end(), 
            [&averageClusterSimilarity](size_t a, size_t b) {
                return averageClusterSimilarity[a] < averageClusterSimilarity[b];
            }
        );

        // Calculate average similarity threshold between consecutive clusters
        float averageSimilarityThreshold = getAverageClusterSimilarityRadius(sortedIndicies, averageClusterSimilarity);

        // Now lets create the hub cluster.
        types::cluster* currentCluster = new types::cluster(types::node::Type::RESONANCE_HUB);

        for (size_t i = 0; i < sortedIndicies.size() - 1; i++) {
            size_t idxA = sortedIndicies[i];
            size_t idxB = sortedIndicies[i + 1];

            // Calculate distance between consecutive clusters based on their average similarities
            float distance = std::abs(averageClusterSimilarity[idxA] - averageClusterSimilarity[idxB]);

            if (distance > averageSimilarityThreshold) {
                // If the distance is greater than the threshold, finalize the current cluster
                if (!currentCluster->definitions.empty()) {
                    clusters.push_back(currentCluster);
                }

                // Start a new cluster
                currentCluster = new types::cluster(types::node::Type::RESONANCE_HUB);
            }
            else {
                // Add the cluster to the current hub cluster
                currentCluster->definitions.push_back(clusters[idxA]);

                if (currentCluster->radius < distance) {
                    currentCluster->radius = distance; // Update the radius to the maximum distance found
                }
            }
        }

        // Don't forget to add the last cluster if it has members
        if (!currentCluster->definitions.empty()) {
            clusters.push_back(currentCluster);
        } else {
            delete currentCluster; // Clean up if empty
        }
    }

    /**
     * For us to get file vectors, we first need to do prior steps:
     *  - Create clusters for each file containing all definitions which are from the commits related to the file clusters
     *  - Order the files list to represent the closeness to each file, where two files which have similar definitions would appear next to each other in the files listing.
     *  - Then go through the definitions and calculate their file vector to point to the most occurring files and any deviation would still hold to closeby related files.
     */
    void base::fileClustering() {
        if (files.empty() || definitions.empty()) {
            return;
        }

        // Step 1: Create file clusters and gather definitions from related commits
        std::unordered_map<std::string, std::vector<types::node::base*>> fileToDefinitions;
        
        // For each file, find definitions that are related through commits
        for (auto& file : files) {
            for (auto& connection : file.connections) {
                // Find all definitions that appear in the same commit as this file
                for (auto& defPair : definitions) {
                    for (auto& defConnection : defPair.second.connections) {
                        if (defConnection.Index == connection.Index) {
                            fileToDefinitions[file.symbol].push_back(&defPair.second);
                            break;
                        }
                    }
                }
            }
        }

        // Step 2: Calculate similarity between files based on shared definitions
        std::vector<float> averageFileSimilarity(files.size(), 0.0f);
        
        for (size_t i = 0; i < files.size(); ++i) {
            const auto& fileA = files[i];
            float totalSimilarity = 0.0f;
            int validComparisons = 0;
            
            for (size_t j = 0; j < files.size(); ++j) {
                if (i == j) continue;
                
                const auto& fileB = files[j];
                
                // Calculate file similarity based on cosine similarity of their connection patterns
                float similarity = calculateCosineSimilarity(fileA, fileB);
                totalSimilarity += similarity;
                validComparisons++;
            }
            
            averageFileSimilarity[i] = validComparisons > 0 ? (totalSimilarity / validComparisons) : 0.0f;
        }

        // Step 3: Sort files by their average similarity to create ordered clusters
        std::vector<size_t> fileIndices(files.size());
        std::iota(fileIndices.begin(), fileIndices.end(), 0);
        
        std::sort(
            fileIndices.begin(), fileIndices.end(),
            [&averageFileSimilarity](size_t a, size_t b) {
                return averageFileSimilarity[a] < averageFileSimilarity[b];
            }
        );
        
        // Reorder files based on sorted indices
        std::vector<types::definition> sortedFiles;
        sortedFiles.reserve(files.size());
        for (size_t idx : fileIndices) {
            sortedFiles.push_back(files[idx]);
        }
        files = std::move(sortedFiles);

        // Step 4: Create file clusters, and add definitions to them
        // Use a map to track which file indices have been added to each definition
        std::unordered_map<types::definition*, std::unordered_set<size_t>> definitionFileIndices;
        
        for (auto defs : fileToDefinitions) {
            types::context* currentCluster = new types::context(defs.first);

            currentCluster->definitions = std::move(defs.second);

            // Get this file's own index
            size_t fileIndex = 0;

            for (size_t i = 0; i < files.size(); ++i) {
                if (files[i].symbol == defs.first) {
                    fileIndex = i;
                    break;
                }
            }

            // cache where the definition occurred (only if not already added)
            for (auto* def : currentCluster->definitions) {
                types::definition* typedDef = (types::definition*)def;
                if (definitionFileIndices[typedDef].find(fileIndex) == definitionFileIndices[typedDef].end()) {
                    typedDef->referenced.push_back(fileIndex);
                    definitionFileIndices[typedDef].insert(fileIndex);
                }
            }
        }

        // Now we can go through the definitions and calculate their normalized file index
        for (auto& def : definitions) {
            types::definition* tmp = &def.second;

            for (auto& ref : tmp->referenced) {
                tmp->fileVector += (float)ref;
            }

            if (!tmp->referenced.empty()) {
                // Calculate average file index, then normalize to [0.0, 1.0] range
                tmp->fileVector /= static_cast<float>(tmp->referenced.size()); // Average file index
                if (files.size() > 1) {
                    tmp->fileVector /= static_cast<float>(files.size() - 1); // Normalize to [0.0, 1.0]
                }
            } else {
                tmp->fileVector = 0.0f; // No references, set to zero
            }
        }

    }

    // Whole program optimization to find lower cost for each cluster and their threshold variance.
    void base::gradientDecent() {
        
    }

    // Compute dot product between a and b, with the floating point members use as vector values.
    float base::dotProduct(types::node::base* a, types::node::base* b, std::vector<float>& result) {
        // Clear the result vector and prepare it to store the component products
        result.clear();
        
        std::vector<float> vectorA = a->getVector();
        std::vector<float> vectorB = b->getVector();
        
        // Compute dot product: sum of component-wise products
        float dotProduct = 0.0f;
        result.reserve(vectorA.size());
        
        for (size_t i = 0; i < vectorA.size(); ++i) {
            float componentProduct = vectorA[i] * vectorB[i];
            result.push_back(componentProduct);
            dotProduct += componentProduct;
        }
        
        return dotProduct;
    }

    const std::vector<types::cluster*>& base::getClusters() {
        return clusters;
    }

    types::context* base::findContext(const std::string name) {
        for (auto& cluster : clusters) {
            if (cluster->type == types::node::Type::CONTEXT && types::getNormalSymbol(((types::context*)cluster)->symbol) == name) {
                return static_cast<types::context*>(cluster);
            }
        }

        return nullptr;
    }
    
    std::vector<types::cluster*> base::getClustersByType(types::node::Type type) {
        std::vector<types::cluster*> result;
        for (const auto cluster : clusters) {
            if (cluster->type == type) {
                result.push_back(cluster);
            }
        }
        return result;
    }
    
    std::vector<std::vector<float>> base::buildSimilarityMatrix() {
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

    float base::getAverageChronicRadius(const std::vector<size_t>& indicies) {
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

    float base::getAverageOccurrenceRadius(const std::vector<size_t>& indicies) {
        // Checks for each pair of i and i+1, and calculates their occurrence distance, returning the average of these distances.
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        // NOTE: Because, we already sort the indicies via occurrence, thus O(n^2) is not necessary.
        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            auto itA = definitions.begin();
            std::advance(itA, indicies[i]);
            auto itB = definitions.begin();
            std::advance(itB, indicies[i + 1]);
            totalDistance += std::abs(itA->second.CommitFrequency - itB->second.CommitFrequency);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    float base::getAverageClusterRadius(const std::vector<size_t>& indicies) {
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        // NOTE: Because, we already sort the indicies via occurrence, thus O(n^2) is not necessary.
        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            auto itA = clusters.begin();
            std::advance(itA, indicies[i]);
            auto itB = clusters.begin();
            std::advance(itB, indicies[i + 1]);
            totalDistance += std::abs((*itA)->radius - (*itB)->radius);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    float base::getAverageDotProductRadius(const std::vector<size_t>& indicies, std::vector<std::pair<std::string, types::definition>>& defsVector) {
        // Calculates the average dot product distance between consecutive definitions in the sorted indices
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            size_t idxA = indicies[i];
            size_t idxB = indicies[i + 1];

            // Calculate dot product between the two definitions
            std::vector<float> dotProductResult;
            float dotProdA = 0.0f, dotProdB = 0.0f;
            
            // Calculate average dot product for each definition with all others
            int validComparisons = 0;
            for (size_t j = 0; j < defsVector.size(); ++j) {
                if (j != idxA) {
                    dotProdA += dotProduct(&defsVector[idxA].second, &defsVector[j].second, dotProductResult);
                    validComparisons++;
                }
            }
            if (validComparisons > 0) dotProdA /= validComparisons;

            validComparisons = 0;
            for (size_t j = 0; j < defsVector.size(); ++j) {
                if (j != idxB) {
                    dotProdB += dotProduct(&defsVector[idxB].second, &defsVector[j].second, dotProductResult);
                    validComparisons++;
                }
            }
            if (validComparisons > 0) dotProdB /= validComparisons;

            totalDistance += std::abs(dotProdA - dotProdB);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    float base::getAverageClusterSimilarityRadius(const std::vector<size_t>& indicies, const std::vector<float>& averageClusterSimilarity) {
        // Calculates the average similarity distance between consecutive clusters in the sorted indices
        if (indicies.size() < 2) return 0.0f;

        float totalDistance = 0.0f;

        for (size_t i = 0; i < indicies.size() - 1; ++i) {
            size_t idxA = indicies[i];
            size_t idxB = indicies[i + 1];

            totalDistance += std::abs(averageClusterSimilarity[idxA] - averageClusterSimilarity[idxB]);
        }

        return totalDistance / static_cast<float>(indicies.size() - 1);
    }

    // Legacy methods for backward compatibility with tests
    const std::unordered_map<std::string, types::definition>& base::getDefinitions() const {
        return definitions;
    }
    
    const types::definition* base::getDefinition(const std::string& symbol) const {
        auto it = definitions.find(symbol);
        return (it != definitions.end()) ? &it->second : nullptr;
    }
    
    std::vector<types::definition> base::getDefinitionsByOccurrence() {
        std::vector<std::pair<std::string, types::definition>> defsVec = getDefinitionsVector();
        std::vector<types::definition> result;
        
        // Sort by occurrence (descending)
        std::sort(defsVec.begin(), defsVec.end(), 
                  [](const std::pair<std::string, types::definition>& a, 
                     const std::pair<std::string, types::definition>& b) {
                      return a.second.CommitFrequency > b.second.CommitFrequency;
                  });
        
        for (const auto& pair : defsVec) {
            result.push_back(pair.second);
        }
        
        return result;
    }
    
    std::vector<types::definition> base::findTemporallyRelatedDefinitions(const std::string& symbol, float threshold) const {
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
    
    std::vector<types::definition> base::findCoOccurringDefinitions(const std::string& symbol, float threshold) const {
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
