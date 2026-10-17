#ifndef GPSR_H
#define GPSR_H

#include "gpsr-ptable.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/node-list.h"

namespace ns3 {
namespace gpsr {

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);
  static const uint32_t GPSR_PORT;
  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();

  virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                           const UnicastForwardCallback &ucb, const MulticastForwardCallback &mcb,
                           const LocalDeliverCallback &lcb, const ErrorCallback &ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const {}

  void SetDownTarget (IpL4Protocol::DownTargetCallback callback);
  IpL4Protocol::DownTargetCallback GetDownTarget (void) const;
  void AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

private:
  void Start ();
  void HelloTimerExpire ();
  void SendHello ();
  bool IsMyOwnAddress (Ipv4Address src);
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
  void RecvGPSR (Ptr<Socket> socket);
  void UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver, Vector Pos);
  void DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, const UnicastForwardCallback &ucb, const ErrorCallback &ecb);
  bool Forwarding (Ptr<const Packet> p, const Ipv4Header & header, const UnicastForwardCallback &ucb, const ErrorCallback &ecb);
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const;
  void RecoveryMode(Ipv4Address dst, Ptr<Packet> p, const UnicastForwardCallback &ucb, Ipv4Header header);

  Ptr<Ipv4> m_ipv4;
  std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
  Time HelloInterval;
  Timer HelloIntervalTimer;
  PositionTable m_neighbors;
  bool PerimeterMode;
  IpL4Protocol::DownTargetCallback m_downTarget;
};

} 
} 

#endif /* GPSR_H */
