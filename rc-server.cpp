#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "rc-server.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RCServer");
NS_OBJECT_ENSURE_REGISTERED (RCServer);

TypeId 
RCServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RCServer")
    .SetParent<Application> ()
    .AddConstructor<RCServer> ()
    .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&RCServer::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (TcpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&RCServer::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("ResponseSize", "Reponse size Random Variable", 
                   RandomVariableValue(ParetoVariable(100.0)), 
                   MakeRandomVariableAccessor(&RCServer::response_size), 
                   MakeRandomVariableChecker())
    ;
  return tid;
}

RCServer::RCServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
  m_tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
}

RCServer::~RCServer()
{
  NS_LOG_FUNCTION (this);
}

uint32_t RCServer::GetTotalRx() const
{
  return m_totalRx;
}
  
void RCServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}

// Application Methods
void RCServer::StartApplication()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode(), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      m_socket->ShutdownSend ();
    }

  m_socket->SetRecvCallback (MakeCallback(&RCServer::HandleRead, this));
  m_socket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeCallback(&RCServer::HandleAccept, this));
  m_socket->SetCloseCallbacks (
            MakeCallback(&RCServer::HandlePeerClose, this),
            MakeCallback(&RCServer::HandlePeerError, this));
}

void RCServer::StopApplication()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty()) //these are accepted sockets, close them
  {
    Ptr<Socket> acceptedSocket = m_socketList.front();
    m_socketList.pop_front();
    acceptedSocket->Close();
  }
  if (m_socket) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void RCServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from))
    {
      if (packet->GetSize() == 0)
        { //EOF
          break;
        }
    }

  // schedule write response after sometime
  Simulator::Schedule(Time("1us"), &RCServer::SendResponse, this, socket);
}

  void RCServer::SendResponse(Ptr<Socket> socket) {
    NS_LOG_FUNCTION(this << socket);
    int resp_size = 1<<12;
    Ptr<Packet> resp = Create<Packet>(resp_size);
    NS_LOG_INFO("%%%%%%%% ********* sending resp size " << resp_size);
    int r = socket->Send(resp);
    NS_LOG_INFO("*** GOT send = " << r);
  }

void RCServer::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_INFO("PktSink, peerClose");
}
 
void RCServer::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_INFO("PktSink, peerError");
}
 
void RCServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback(&RCServer::HandleRead, this));
  m_socketList.push_back (s);
}

};
