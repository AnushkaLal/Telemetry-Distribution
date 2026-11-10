#include "network.hpp"
#include <queue>
#include <limits>
#include <iostream>
#include <algorithm>
void NetworkGraph::addLink(const std::string& a,const std::string& b,int c){ adj[a].push_back({b,c}); adj[b].push_back({a,c}); }
void NetworkGraph::disableLink(const std::string& a,const std::string& b){
    auto erase=[](auto& v,const std::string& x){ v.erase(std::remove_if(v.begin(),v.end(),[&](const auto& e){return e.first==x;}),v.end()); };
    erase(adj[a],b); erase(adj[b],a);
}
std::vector<std::string> NetworkGraph::shortestPath(const std::string& src,const std::string& dst) const {
    using P=std::pair<int,std::string>; std::priority_queue<P,std::vector<P>,std::greater<P>> pq;
    std::map<std::string,int> d; std::map<std::string,std::string> prev;
    for(const auto& kv:adj)d[kv.first]=std::numeric_limits<int>::max();
    if(!d.count(src)||!d.count(dst)) return {};
    d[src]=0; pq.push({0,src});
    while(!pq.empty()) { auto [cost,u]=pq.top();pq.pop(); if(cost!=d[u])continue; if(u==dst)break;
        for(const auto& [v,w]:adj.at(u)) if(d[v]>cost+w){d[v]=cost+w;prev[v]=u;pq.push({d[v],v});}
    }
    if(d[dst]==std::numeric_limits<int>::max())return {};
    std::vector<std::string> path; for(std::string at=dst;;at=prev[at]){path.push_back(at);if(at==src)break;if(!prev.count(at))return {};}
    std::reverse(path.begin(),path.end()); return path;
}
int NetworkGraph::pathCost(const std::vector<std::string>& p) const { int sum=0; for(std::size_t i=1;i<p.size();++i){bool found=false;for(auto [v,w]:adj.at(p[i-1]))if(v==p[i]){sum+=w;found=true;break;}if(!found)return -1;}return sum; }
void NetworkGraph::displayLinks() const { for(const auto& [u,edges]:adj) for(const auto& [v,w]:edges) if(u<v) std::cout<<"  "<<u<<" -- "<<v<<"  cost "<<w<<"\n"; }
