
#ifndef __rc_server_h__
#define __rc_server_h__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/random-variable.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

class RCServer : public Application 
{
public:
  static TypeId GetTypeId (void);
  RCServer ();

  virtual ~RCServer ();

  /**
   * \return the total bytes received in this sink app
   */
  uint32_t GetTotalRx () const;
  
protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  void HandleRead (Ptr<Socket>);
  void HandleAccept (Ptr<Socket>, const Address& from);
  void HandlePeerClose(Ptr<Socket>);
  void HandlePeerError(Ptr<Socket>);
  void SendResponse(Ptr<Socket>);
  // In the case of TCP, each socket accept returns a new socket, so the 
  // listening socket is stored seperately from the accepted sockets
  Ptr<Socket>     m_socket;       // Listening socket
  std::list<Ptr<Socket> > m_socketList; //the accepted sockets

  Address         m_local;        // Local address to bind to
  uint32_t        m_totalRx;      // Total bytes received
  TypeId          m_tid;          // Protocol TypeId

  RandomVariable response_size;
};
 
};
#endif
