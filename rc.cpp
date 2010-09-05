#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"

#include "proxyqueue.h"
using namespace ns3;

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
using namespace std;

#define REP(i,n) for(int i = 0; i < (n); ++i)
#define GI ({int _; scanf("%d", &_); _;})
#define dbg(e) (cout << #e << " = " << e << " ")
#define dbge(e) dbg(e) << endl
#define LET(it,a) __typeof(a) it(a)
#define EACH(it,cont) for(LET(it,cont.begin()); it != cont.end(); ++it)
#define FOR(i,s,e) for(int i = s; i < e; ++i)
#define mp make_pair

typedef unsigned long long u64;

namespace RC {
  
struct Node {
  int id;
  bool is_switch;
  
  Node():id(-1), is_switch(false){}
  Node(int a, bool b):id(a), is_switch(b){}
};

typedef pair<Node,Node> Edge;

struct Graph {
  vector<Node> nodes;
  vector<Edge> edges;
};

Graph dumbell(int left, int right) {
  Graph ret;
  REP(i, left) ret.nodes.push_back(Node(i, false));
  REP(i, right) ret.nodes.push_back(Node(i + left, false));
  ret.nodes.push_back(Node(left+right, true));

  REP(i, left) {
    Edge e = make_pair(ret.nodes[i], ret.nodes[left+right]);
    ret.edges.push_back(e);
  }

  REP(i, right) {
    Edge e = make_pair(ret.nodes[left+i], ret.nodes[left+right]);
    ret.edges.push_back(e);
  }

  return ret;
}

    
} // END Namespace RC



struct NS3Graph {
  NodeContainer nodes;
  Ipv4InterfaceContainer ipaddr;

  vector<NodeContainer> edges;
  vector<NetDeviceContainer> edges_devices;

  NetDeviceContainer all_devices;
  
  RC::Graph g;

  NS3Graph(){}
  NS3Graph(const RC::Graph &);
};

NS3Graph::NS3Graph(const RC::Graph &_g) {
  g = _g;
  // Create the nodes
  nodes.Create(g.nodes.size());

  // Create the channels
  PointToPointHelper ptp;
  ptp.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
  //ptp.SetDeviceAttribute("Delay", StringValue("300ns"));
  ptp.SetQueue("ns3::ProxyQueue");
  EACH(e, g.edges) {
    int from = (*e).first.id, to = (*e).second.id;
    edges.push_back( NodeContainer(nodes.Get(from), nodes.Get(to)) );
    edges_devices.push_back( ptp.Install(edges.back()) );
    all_devices.Add(edges_devices.back());
  }

  // Install the inet stack on all hosts
  InternetStackHelper internet;
  internet.Install(nodes);

  // Configure the IP addresses of all the nodes (hosts and routers)
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.0.0.0");
  ipv4.Assign(all_devices);

  // Configure routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
}

int main(int argc, char *argv[]) {
  RC::Graph g = RC::dumbell(5, 1);
  NS3Graph ns3g(g);
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
