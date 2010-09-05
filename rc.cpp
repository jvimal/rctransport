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
    vector< Ptr<BridgeNetDevice> > br;     // will be null for end hosts
    Ptr<Node> node;
    NetDeviceContainer intfs;    // the interfaces on the host
    Ipv4InterfaceContainer inet; // the inet stacks on all the intfs
  };

  vector<NS3Node> hosts;
  RC::Graph g;

  NS3Graph(){}
  NS3Graph(const RC::Graph &);
};

Mac48Address NextMacAddress() {
  static unsigned long long count = 0;
  static unsigned char *addr = (unsigned char *)(&count);
  static char buff[128];
  count++;
  sprintf(buff, "%x:%x:%x:%x:%x:%x", 
          addr[5], addr[4], addr[3],
          addr[2], addr[1], addr[0]);
  return Mac48Address(buff);
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

  EACH(e, g.edges) {
    int from = (*e).first.id, to = (*e).second.id;

    LET(dev_from, Create<PointToPointNetDevice>());
    LET(dev_to, Create<PointToPointNetDevice>());

    LET(q_from, Create<ProxyQueue>());
    LET(q_to, Create<ProxyQueue>());

    dev_from->SetDataRate(DataRate("10Gbps"));
    dev_from->SetAddress(Mac48Address::Allocate());
    hosts[from].node->AddDevice(dev_from);
    dev_from->SetQueue(q_from);
    
    dev_to->SetDataRate(DataRate("10Gbps"));
    dev_to->SetAddress(Mac48Address::Allocate());
    hosts[to].node->AddDevice(dev_to);
    dev_to->SetQueue(q_to);

    LET(chan, Create<PointToPointChannel>());
    chan->Attach(dev_from);
    chan->Attach(dev_to);

    
    hosts[from].intfs.Add(dev_from);
    hosts[to].intfs.Add(dev_to);
  }

  // Install the inet stack on all hosts
  InternetStackHelper internet;
  internet.Install(nodes);
  
  // Configure the IP addresses of all the nodes (hosts only)
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.0.0.0");
  
  REP(i, g.nodes.size()) {
    // Assign ip address to the net devices
    // only on the hosts
    hosts[i].inet = ipv4.Assign(hosts[i].intfs);
  }

  //GlobalRouteManager::BuildGlobalRoutingDatabase();

  // An example of how nodes can be queried for information
  REP(i, g.nodes.size()) {
    LET(n, nodes.Get(i));
    ostringstream out;
    out << "Node " << i << " has " << n->GetNDevices() << " devices ";
    //if(not hosts[i].is_switch)
    out << hosts[i].inet.GetAddress(0) << " ";
    out << "Link up? " << hosts[i].intfs.Get(0)->IsLinkUp() << " ";
    out << "Address: " << hosts[i].intfs.Get(0)->GetAddress() << " ";
    out << "MTU: " << hosts[i].intfs.Get(0)->GetMtu() << " ";
    puts(out.str().c_str());
  }

  
  // Run a UDP client test
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get(1));
  
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  
  UdpEchoClientHelper echoClient (hosts[1].inet.GetAddress(0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  
  ApplicationContainer clientApps = echoClient.Install (nodes.Get(0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  
}
  
int main(int argc, char *argv[]) {
  RC::Graph g = RC::dumbell(5, 1);
  NS3Graph ns3g(g);
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
