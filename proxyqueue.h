
#ifndef PROXYQUEUE_H
#define PROXYQUEUE_H

#include <queue>
#include "ns3/packet.h"
#include "ns3/queue.h"


#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("ProxyQueue");

namespace ns3 {

class TraceContainer;
  
 class ProxyQueue : public Queue {
 public:
  static TypeId GetTypeId (void);
  ProxyQueue ();
  
  virtual ~ProxyQueue();
  
  /**
   * Enumeration of the modes supported in the class.
   *
   */
  enum Mode {
    ILLEGAL,     /**< Mode not set */
    PACKETS,     /**< Use number of packets for maximum queue size */
    BYTES,       /**< Use number of bytes for maximum queue size */  
  };
  
  /**
   * Set the operating mode of this device.
   *
   * \param mode The operating mode of this device.
   *
   */
  void SetMode (ProxyQueue::Mode mode);

  /**
   * Get the encapsulation mode of this device.
   *
   * \returns The encapsulation mode of this device.
   */
  ProxyQueue::Mode  GetMode (void);

private:
  virtual bool DoEnqueue (Ptr<Packet> p);
  virtual Ptr<Packet> DoDequeue (void);
  virtual Ptr<const Packet> DoPeek (void) const;

  std::queue<Ptr<Packet> > m_packets;
  uint32_t m_maxPackets;
  uint32_t m_maxBytes;
  uint32_t m_bytesInQueue;
  Mode     m_mode;
};

NS_OBJECT_ENSURE_REGISTERED (ProxyQueue);


TypeId ProxyQueue::GetTypeId (void) 
{
  static TypeId tid = TypeId ("ns3::ProxyQueue")
    .SetParent<Queue> ()
    .AddConstructor<ProxyQueue> ()
    .AddAttribute ("Mode", 
                   "Whether to use Bytes (see MaxBytes) or Packets (see MaxPackets) as the maximum queue size metric.",
                   EnumValue (PACKETS),
                   MakeEnumAccessor (&ProxyQueue::SetMode),
                   MakeEnumChecker (BYTES, "Bytes",
                                    PACKETS, "Packets"))
    .AddAttribute ("MaxPackets", 
                   "The maximum number of packets accepted by this ProxyQueue.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ProxyQueue::m_maxPackets),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxBytes", 
                   "The maximum number of bytes accepted by this ProxyQueue.",
                   UintegerValue (100 * 65535),
                   MakeUintegerAccessor (&ProxyQueue::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    ;
  
  return tid;
}

ProxyQueue::ProxyQueue () :
  Queue (),
  m_packets ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

ProxyQueue::~ProxyQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

  void 
ProxyQueue::SetMode (enum Mode mode)
{
  NS_LOG_FUNCTION (mode);
  m_mode = mode;
}

  ProxyQueue::Mode
ProxyQueue::GetMode (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mode;
}
  
bool 
ProxyQueue::DoEnqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  if (m_mode == PACKETS && (m_packets.size () >= m_maxPackets))
    {
      NS_LOG_LOGIC ("Queue full (at max packets) -- droppping pkt");
      Drop (p);
      return false;
    }

  if (m_mode == BYTES && (m_bytesInQueue + p->GetSize () >= m_maxBytes))
    {
      NS_LOG_LOGIC ("Queue full (packet would exceed max bytes) -- droppping pkt");
      Drop (p);
      return false;
    }

  m_bytesInQueue += p->GetSize ();
  m_packets.push(p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return true;
}

Ptr<Packet>
ProxyQueue::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty()) 
    {
      NS_LOG_LOGIC ("Queue empty");
      return false;
    }

  Ptr<Packet> p = m_packets.front ();
  m_packets.pop ();
  m_bytesInQueue -= p->GetSize ();

  NS_LOG_LOGIC ("Popped " << p);

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

Ptr<const Packet>
ProxyQueue::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  if (m_packets.empty()) 
    {
      NS_LOG_LOGIC ("Queue empty");
      return false;
    }

  Ptr<Packet> p = m_packets.front ();

  NS_LOG_LOGIC ("Number packets " << m_packets.size ());
  NS_LOG_LOGIC ("Number bytes " << m_bytesInQueue);

  return p;
}

}; // end namespace RC

#endif
