#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/bridge-net-device.h"
#include "ns3/ipv4-global-routing.h"
#include "ns3/global-route-manager.h"
#include "ns3/mac48-address.h"
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"

#include "proxyqueue.h"

using namespace ns3;

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
using namespace std;

#define REP(i,n) for(typeof(n) i = 0; i < (n); ++i)
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
  
  struct NS3Node {
    bool is_switch;
    BridgeNetDevice br;          // will be null for end hosts
    Ptr<Node> node;
    NetDeviceContainer intfs;    // the interfaces on the host
    vector< Ptr<ProxyQueue> > queues; // the transmit queues on the host
    Ipv4InterfaceContainer inet; // the inet stacks on all the intfs
  };

  vector<NS3Node> hosts;
  RC::Graph g;

  NS3Graph(){}
  NS3Graph(const RC::Graph &);
};

void cb(uint32_t old, uint32_t neew) {
  cout << Simulator::Now().GetSeconds() << "\t" << neew << endl;
}

NS3Graph::NS3Graph(const RC::Graph &_g) {
  g = _g;
  // Create the nodes
  nodes.Create(g.nodes.size());
  hosts.resize(g.nodes.size());

  // For each node, create a single net device
  REP(i, g.nodes.size()) {
    hosts[i].node = nodes.Get(i);
    hosts[i].is_switch = g.nodes[i].is_switch;
  }

  // Every edge represents a pair of net-devices and 
  // a channel connecting them.  will create new devices
  CsmaHelper ptp;
  ptp.SetChannelAttribute ("DataRate" , StringValue ("1Gbps"));
  //ptp.SetChannelAttribute ("Delay", StringValue ("300ns"));
  //ptp.SetQueue("ns3::ProxyQueue"); // doesn't work, i dunno why
   
  EACH(e, g.edges) {
    int from = (*e).first.id, to = (*e).second.id;
    LET(container, ptp.Install(NodeContainer(nodes.Get(from), nodes.Get(to))));
    hosts[from].intfs.Add(container.Get(0));
    hosts[to].intfs.Add(container.Get(1));
    int which[] = {from, to};

    REP(i, 2) {
      LET(a, PeekPointer(container.Get(i)));
      LET(q, Create<ProxyQueue>());
      static_cast<CsmaNetDevice*>(a)->SetQueue(q);
      hosts[ which[i] ].queues.push_back(q);
    }
  }
  
  /* Create a trace for host 6 (switch) and queue 5 */
  hosts[6].queues[5]
    ->SetQueueTrace(MakeCallback(&cb));

  // Install the inet stack on all hosts
  InternetStackHelper internet;
  // Enable this for routed network
  //Ipv4GlobalRoutingHelper glob;
  //internet.SetRoutingHelper(glob);
  internet.Install(nodes);
  
  // Configure the IP addresses of all the nodes (hosts only)
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.255.255.0");
  
  REP(i, g.nodes.size()) {
    // Assign ip address to the net devices
    // only on the hosts
    hosts[i].inet = ipv4.Assign(hosts[i].intfs);

    // disable this for routed network
    if(hosts[i].is_switch) {
      BridgeHelper bridge;
      bridge.Install(hosts[i].node, hosts[i].intfs);
    }
  }
  
  // Enable this only if we want a routed network instead
  // of a switched network
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  UdpClientHelper udp_client(Ipv4Address("10.0.0.6"), 10);
  udp_client.SetAttribute("MaxPackets", StringValue("200"));
  udp_client.SetAttribute("Interval", StringValue("0.1us"));
  udp_client.SetAttribute("StartTime", StringValue("0"));
  udp_client.SetAttribute("StopTime", StringValue("5s"));

  udp_client.Install(NodeContainer(nodes.Get(0), nodes.Get(1)));
}
  
int main(int argc, char *argv[]) {
  RC::Graph g = RC::dumbell(5, 1);
  NS3Graph ns3g(g);
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
