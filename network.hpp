#ifndef NETWORK_HPP
#define NETWORK_HPP
#include <string>
#include <vector>
#include <map>
#include <utility>
class NetworkGraph {
    std::map<std::string, std::vector<std::pair<std::string,int>>> adj;
public:
    void addLink(const std::string&, const std::string&, int);
    void disableLink(const std::string&, const std::string&);
    std::vector<std::string> shortestPath(const std::string&, const std::string&) const;
    int pathCost(const std::vector<std::string>&) const;
    void displayLinks() const;
};
#endif
