#ifndef NS2_NODE_UTILITY_HPP
#define NS2_NODE_UTILITY_HPP

#include <fstream>
#include <sstream>
#include <map>
#include <set>      // Added for counting unique nodes
#include <string>
#include <algorithm> // Added for std::max
#include "ns3/core-module.h"

using namespace ns3;

class Ns2NodeUtility {
public:
    Ns2NodeUtility() : m_maxSimTime(0.0) {}

    void Parse(std::string traceFile) {
        std::ifstream file(traceFile);
        if (!file.is_open()) {
            NS_FATAL_ERROR("Could not open trace file: " << traceFile);
        }

        std::string line;
        while (std::getline(file, line)) {
            
            int nodeId = -1;
            size_t nodePos = line.find("$node_(");
            if (nodePos != std::string::npos) {
                size_t endParen = line.find(")", nodePos);
                if (endParen != std::string::npos) {
                    std::string idStr = line.substr(nodePos + 7, endParen - (nodePos + 7));
                    try {
                        nodeId = std::stoi(idStr);
                        m_uniqueNodes.insert(nodeId); // Store unique ID
                    } catch (...) { continue; }
                }
            }
            if (line.find("$ns_ at") != std::string::npos) {
                
                size_t atPos = line.find("at ");
                size_t quotePos = line.find("\"", atPos);
                
                if (atPos != std::string::npos && quotePos != std::string::npos) {
                    std::string timeStr = line.substr(atPos + 3, quotePos - (atPos + 3));
                    double time = std::stod(timeStr);

                    if (time > m_maxSimTime) {
                        m_maxSimTime = time;
                    }

             
                    if (nodeId != -1) {
                        // Update Entry Time (Min found)
                        if (m_entryTimes.find(nodeId) == m_entryTimes.end()) {
                            m_entryTimes[nodeId] = time;
                        } else {
                            if (time < m_entryTimes[nodeId]) m_entryTimes[nodeId] = time;
                        }

                        // Update Exit Time (Max found)
                        if (m_exitTimes.find(nodeId) == m_exitTimes.end()) {
                            m_exitTimes[nodeId] = time;
                        } else {
                            if (time > m_exitTimes[nodeId]) m_exitTimes[nodeId] = time;
                        }
                    }
                }
            } 
        }
        file.close();
    }

    double GetTotalSimulationTime() const {
        return m_maxSimTime;
    }


    uint32_t GetTotalNodeCount() const {
        return m_uniqueNodes.size();
    }

    double GetEntryTimeForNode(uint32_t nodeId) {
        if (m_entryTimes.find(nodeId) != m_entryTimes.end()) {
            return m_entryTimes[nodeId];
        }
        return 0.0; 
    }

    double GetExitTimeForNode(uint32_t nodeId) {
        if (m_exitTimes.find(nodeId) != m_exitTimes.end()) {
            return m_exitTimes[nodeId];
        }
        // Return Max Sim Time instead of hardcoded 500.0 if node not found
        return m_maxSimTime > 0 ? m_maxSimTime : 100.0; 
    }

private:
    std::map<int, double> m_entryTimes;
    std::map<int, double> m_exitTimes;
    std::set<int> m_uniqueNodes; 
    double m_maxSimTime;         
};

#endif 