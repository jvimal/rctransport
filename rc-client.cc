
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

#include "rc-client.h"

namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("RCClient");
  NS_OBJECT_ENSURE_REGISTERED (RCClient);

  TypeId
  RCClient::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::RCClient")
      .SetParent<Application> ()
      .AddConstructor<RCClient> ()
      .AddAttribute ("RequestSize",
                     "The size of request",
                     RandomVariableValue(ParetoVariable(100.0)),
                     MakeRandomVariableAccessor (&RCClient::request_size),
                     MakeRandomVariableChecker()) /* No checked */
      .AddAttribute ("ServerAddress", 
                     "The destination Ipv4Address of the outbound packets",
                     Ipv4AddressValue (),
                     MakeIpv4AddressAccessor (&RCClient::server_addr),
                     MakeIpv4AddressChecker ())
      .AddAttribute ("ServerPort", 
                     "The destination port of the outbound packets",
                     UintegerValue (1000),
                     MakeUintegerAccessor (&RCClient::server_port),
                     MakeUintegerChecker<uint16_t> ())
      .AddTraceSource("ResponseTime", "Tracing request/response time", 
                      MakeTraceSourceAccessor(&RCClient::response_trace))
      ;
    return tid;
  }
  
  RCClient::RCClient() {
    NS_LOG_FUNCTION_NOARGS();
    socket = 0;
    send_event = EventId();
  }

  RCClient::~RCClient() {
    NS_LOG_FUNCTION_NOARGS();
    socket = 0;
  }
  
  void RCClient::DoDispose() {
    NS_LOG_FUNCTION_NOARGS();
    Application::DoDispose();
  }

  void RCClient::SetResponseCallback(const CallbackBase &cb) {
    response_trace.ConnectWithoutContext(cb);
  }
  
  void RCClient::StartApplication() {
    NS_LOG_FUNCTION_NOARGS();
    if(socket == 0) {
      TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
      socket = Socket::CreateSocket(GetNode(), tid);
      socket -> Bind();
      socket -> Connect(InetSocketAddress(server_addr, server_port));
    }

    socket->SetRecvCallback(MakeCallback(&RCClient::Receive, this));
    ScheduleTransmit(Seconds(0));
  }
  
  void RCClient::StopApplication() {
    NS_LOG_FUNCTION_NOARGS();
    if(socket != 0) {
      socket->Close();
      socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> > ());
      socket = 0;
    }
    Simulator::Cancel(send_event);
  }

  void RCClient::ScheduleTransmit(Time dt) {
    NS_LOG_FUNCTION_NOARGS();
    send_event = Simulator::Schedule(dt, &RCClient::Send, this);
  }

  void RCClient::Send() {
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(send_event.IsExpired());
    int sz = (int)request_size.GetValue();
    Ptr<Packet> p = Create<Packet>(sz);
    NS_LOG_INFO("####### sending request of size " << sz);
    start_time = Simulator::Now();
    socket->Send(p);
    //ScheduleTransmit(Time("0.1s"));
    socket->SetRecvCallback(MakeCallback(&RCClient::Receive, this));
  }

  void RCClient::Receive(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address from;
    while(packet = socket->RecvFrom(from)) {
      if(packet->GetSize() == 0) {
        end_time = Simulator::Now();
        break;
      }
    }
    
    response_trace(start_time, end_time);
    ScheduleTransmit(Time("0"));
  }

}; // end namespace
