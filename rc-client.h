#ifndef __RC_CLIENT_H__
#define __RC_CLIENT_H__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable.h"

namespace ns3 {
class Socket;
class Packet;

class RCClient : public Application {
public:
  static TypeId GetTypeId(void);
  RCClient();
  virtual ~RCClient();

  void SetRemote(Ipv4Address ip, uint16_t port);
  
protected:
  virtual void DoDispose (void);

private:
  RandomVariable request_size;
  Ptr<Socket> socket;

  Ipv4Address server_addr;
  uint16_t server_port;

  Time start_time;
  Time end_time;

  EventId send_event;
  TracedCallback<Time,Time> response_trace;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTransmit (Time dt);
  void Send (void);

  void Receive (Ptr<Socket> socket);
};

  
}; // end namespace

#endif
