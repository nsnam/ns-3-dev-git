#include "ns3/node-list.h"
#include "ns3/ipv4.h"
#include "ns3/mobility-model.h"
#include "ns3/vector.h"
#include "gpsr.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GpsrRoutingProtocol");

namespace gpsr {

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

const uint32_t RoutingProtocol::GPSR_PORT = 666;

RoutingProtocol::RoutingProtocol ()
  : HelloInterval (Seconds (1)),
    HelloIntervalTimer (Timer::CANCEL_ON_DESTROY),
    PerimeterMode (false)
{
}

TypeId RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::gpsr::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::HelloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("PerimeterMode", "Indicates if PerimeterMode is enabled",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RoutingProtocol::PerimeterMode),
                   MakeBooleanChecker ());
  return tid;
}

RoutingProtocol::~RoutingProtocol () {}

void RoutingProtocol::DoDispose ()
{
  m_ipv4 = nullptr;
  Ipv4RoutingProtocol::DoDispose ();
}

bool RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                  const UnicastForwardCallback &ucb, const MulticastForwardCallback &mcb,
                                  const LocalDeliverCallback &lcb, const ErrorCallback &ecb)
{
  NS_ASSERT (m_ipv4 != nullptr);
  NS_ASSERT (p != nullptr);
  
  int32_t iif = m_ipv4->GetInterfaceForDevice (idev);
  Ipv4Address dst = header.GetDestination ();

  if (m_ipv4->IsDestinationAddress (dst, iif))
    {
      lcb (p->Copy (), header, iif);
      return true;
    }
  return Forwarding (p, header, ucb, ecb);
}

Ptr<Ipv4Route> RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  if (!p)
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      return Ptr<Ipv4Route> ();
    }

  Ipv4Address dst = header.GetDestination ();
  Ptr<MobilityModel> myMobility = m_ipv4->GetObject<MobilityModel> ();
  if (!myMobility) return Ptr<Ipv4Route> ();
  
  Vector myPos = myMobility->GetPosition ();
  Vector dstPos;
  bool dstFound = false;

  // 1. Oraculo: Acha o destino
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i) {
    Ptr<Ipv4> ipv4 = (*i)->GetObject<Ipv4> ();
    if (ipv4 && ipv4->GetAddress (1, 0).GetLocal () == dst) {
      dstPos = (*i)->GetObject<MobilityModel> ()->GetPosition ();
      dstFound = true;
      break;
    }
  }

  if (!dstFound) {
    sockerr = Socket::ERROR_NOROUTETOHOST;
    return Ptr<Ipv4Route> ();
  }

  // 2. Acha o melhor vizinho para o primeiro salto
  double bestDistance = CalculateDistance (myPos, dstPos);
  Ipv4Address nextHop = Ipv4Address::GetAny ();

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i) {
    Ptr<Ipv4> ipv4 = (*i)->GetObject<Ipv4> ();
    if (ipv4 && ipv4 != m_ipv4) {
      Vector nPos = (*i)->GetObject<MobilityModel> ()->GetPosition ();
      if (CalculateDistance (myPos, nPos) <= 250.0) {
        double nDist = CalculateDistance (nPos, dstPos);
        if (nDist < bestDistance) {
          bestDistance = nDist;
          nextHop = ipv4->GetAddress (1, 0).GetLocal ();
        }
      }
    }
  }

  if (nextHop == Ipv4Address::GetAny ()) {
    sockerr = Socket::ERROR_NOROUTETOHOST;
    return Ptr<Ipv4Route> ();
  }

  // 3. Monta a Rota Completa e valida a saida do pacote
  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  route->SetDestination (dst);
  route->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
  route->SetGateway (nextHop);
  route->SetOutputDevice (m_ipv4->GetNetDevice (1));
  
  NS_LOG_INFO ("ORIGEM: Pacote gerado, enviando para o primeiro salto: " << nextHop);
  return route;
}

void RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != nullptr);
  m_ipv4 = ipv4;
  HelloIntervalTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  HelloIntervalTimer.Schedule (Seconds (1.0));
}

void RoutingProtocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback) { m_downTarget = callback; }
IpL4Protocol::DownTargetCallback RoutingProtocol::GetDownTarget (void) const { return m_downTarget; }
void RoutingProtocol::AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route) {
  if (!m_downTarget.IsNull ()) {
    m_downTarget (p, source, destination, protocol, route);
  }
}

void RoutingProtocol::HelloTimerExpire () {
  // 1. Dispara o Beacon
  SendHello ();
  
  // 2. Reseta o cronômetro para o próximo segundo
  HelloIntervalTimer.Cancel ();
  HelloIntervalTimer.Schedule (HelloInterval);
}

void RoutingProtocol::SendHello () { 
  // 1. Acessa o GPS interno do veículo (MobilityModel) para descobrir onde ele está
  Ptr<MobilityModel> mobility = m_ipv4->GetObject<MobilityModel> ();
  if (!mobility) return;
  
  Vector myPosition = mobility->GetPosition ();

  // 2. Varre as antenas do veículo para enviar o broadcast
  for (uint32_t i = 1; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address myAddress = m_ipv4->GetAddress (i, 0).GetLocal ();
      NS_LOG_INFO ("BEACON: Veículo " << myAddress << " informando vizinhos. Minha posição -> X:" << myPosition.x << " Y:" << myPosition.y);
    }
}

bool RoutingProtocol::IsMyOwnAddress (Ipv4Address src) { return false; }
Ptr<Socket> RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const { return nullptr; }
void RoutingProtocol::RecvGPSR (Ptr<Socket> socket) { }
void RoutingProtocol::UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver, Vector Pos) { }

bool RoutingProtocol::Forwarding (Ptr<const Packet> p, const Ipv4Header & header, const UnicastForwardCallback &ucb, const ErrorCallback &ecb)
{
  Ptr<Packet> packet = p->Copy ();
  Ipv4Address dst = header.GetDestination ();
  
  Ptr<MobilityModel> myMobility = m_ipv4->GetObject<MobilityModel> ();
  if (!myMobility) return false;
  Vector myPos = myMobility->GetPosition ();

  // 1. Oraculo: Descobrir a posicao final do destino
  Vector dstPos;
  bool dstFound = false;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i) {
    Ptr<Ipv4> ipv4 = (*i)->GetObject<Ipv4> ();
    if (ipv4 && ipv4->GetAddress (1, 0).GetLocal () == dst) {
      dstPos = (*i)->GetObject<MobilityModel> ()->GetPosition ();
      dstFound = true;
      break;
    }
  }
  if (!dstFound) return false;

  // 2. Calcula a sua propria distancia para o destino
  double bestDistance = CalculateDistance (myPos, dstPos);
  Ipv4Address nextHop = Ipv4Address::GetAny ();
  Ptr<NetDevice> outputDevice = m_ipv4->GetNetDevice (1);

  // 3. Varredura de Vizinhos: Encontra quem esta matematicamente mais perto
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i) {
    Ptr<Ipv4> ipv4 = (*i)->GetObject<Ipv4> ();
    if (ipv4 && ipv4 != m_ipv4) {
      Vector neighborPos = (*i)->GetObject<MobilityModel> ()->GetPosition ();
      double distToNeighbor = CalculateDistance (myPos, neighborPos);
      
      // Se o vizinho estiver no alcance do radio Wi-Fi 802.11p (aprox. 250 metros)
      if (distToNeighbor <= 250.0) {
        double neighborDistToDst = CalculateDistance (neighborPos, dstPos);
        
        if (neighborDistToDst < bestDistance) {
          bestDistance = neighborDistToDst;
          nextHop = ipv4->GetAddress (1, 0).GetLocal ();
        }
      }
    }
  }

  // 4. Repasse de Pacote pela interface de rede MAC
  if (nextHop != Ipv4Address::GetAny ()) {
    NS_LOG_INFO ("GREEDY FORWARDING: Saltando de " << m_ipv4->GetAddress(1,0).GetLocal() << " para vizinho " << nextHop);
    Ptr<Ipv4Route> route = Create<Ipv4Route> ();
    route->SetDestination (dst);
    route->SetGateway (nextHop);
    route->SetOutputDevice (outputDevice);
    ucb (route, packet, header);
    return true;
  }

  NS_LOG_INFO ("FALHA (Local Maximum): Nenhum vizinho disponivel para avancar.");
  return false;
}

void RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, const UnicastForwardCallback &ucb, const ErrorCallback &ecb)
{
  Socket::SocketErrno sockerr;
  // Re-calcula a rota agora que a fila do ARP foi libertada
  Ptr<Ipv4Route> route = RouteOutput (p->Copy (), header, m_ipv4->GetNetDevice (1), sockerr);
  
  if (route)
    {
      NS_LOG_INFO ("ARP resolvido! Retomando o envio do pacote da origem.");
      ucb (route, p, header); // Injeta o pacote na camada MAC 802.11p
    }
  else
    {
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
    }
}

Ptr<Ipv4Route> RoutingProtocol::LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const { return nullptr; }
void RoutingProtocol::RecoveryMode(Ipv4Address dst, Ptr<Packet> p, const UnicastForwardCallback &ucb, Ipv4Header header) { }

void RoutingProtocol::NotifyInterfaceUp (uint32_t interface) { }
void RoutingProtocol::NotifyInterfaceDown (uint32_t interface) { }
void RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) { }
void RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) { }
void RoutingProtocol::Start () { }

} 
}
