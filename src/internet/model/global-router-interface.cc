/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Tom Henderson (tomhend@u.washington.edu)
 */

#include "global-router-interface.h"

#include "global-routing.h"
#include "ipv4.h"
#include "loopback-net-device.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/bridge-net-device.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/object-base.h"

#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GlobalRouter");

// ---------------------------------------------------------------------------
//
// GlobalRoutingLinkRecord Implementation
//
// ---------------------------------------------------------------------------

template <typename T>
GlobalRoutingLinkRecord<T>::GlobalRoutingLinkRecord()
    : m_linkId(IpAddress::GetZero()),
      m_linkData(IpAddress::GetZero()),
      m_linkLocData(IpAddress::GetZero()),
      m_linkType(Unknown),
      m_metric(0)
{
    NS_LOG_FUNCTION(this);
}

template <typename T>
GlobalRoutingLinkRecord<T>::GlobalRoutingLinkRecord(LinkType linkType,
                                                    IpAddress linkId,
                                                    IpAddress linkData,
                                                    uint16_t metric)
    : m_linkId(linkId),
      m_linkData(linkData),
      m_linkLocData(IpAddress::GetZero()),
      m_linkType(linkType),
      m_metric(metric)
{
    NS_LOG_FUNCTION(this << linkType << linkId << linkData << metric);
}

template <typename T>
GlobalRoutingLinkRecord<T>::GlobalRoutingLinkRecord(LinkType linkType,
                                                    IpAddress linkId,
                                                    IpAddress linkData,
                                                    IpAddress linkLocData,
                                                    uint16_t metric)
    : m_linkId(linkId),
      m_linkData(linkData),
      m_linkLocData(linkLocData),
      m_linkType(linkType),
      m_metric(metric)
{
    NS_LOG_FUNCTION(this << m_linkId << m_linkData << m_linkType << m_linkLocData << m_metric);
}

template <typename T>
GlobalRoutingLinkRecord<T>::~GlobalRoutingLinkRecord()
{
    NS_LOG_FUNCTION(this);
}

template <typename T>
GlobalRoutingLinkRecord<T>::IpAddress
GlobalRoutingLinkRecord<T>::GetLinkId() const
{
    NS_LOG_FUNCTION(this);
    return m_linkId;
}

template <typename T>
void
GlobalRoutingLinkRecord<T>::SetLinkId(IpAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_linkId = addr;
}

template <typename T>
GlobalRoutingLinkRecord<T>::IpAddress
GlobalRoutingLinkRecord<T>::GetLinkData() const
{
    NS_LOG_FUNCTION(this);
    return m_linkData;
}

template <typename T>
void
GlobalRoutingLinkRecord<T>::SetLinkData(GlobalRoutingLinkRecord<T>::IpAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_linkData = addr;
}

template <typename T>
GlobalRoutingLinkRecord<T>::LinkType
GlobalRoutingLinkRecord<T>::GetLinkType() const
{
    NS_LOG_FUNCTION(this);
    return m_linkType;
}

template <typename T>
void
GlobalRoutingLinkRecord<T>::SetLinkType(GlobalRoutingLinkRecord::LinkType linkType)
{
    NS_LOG_FUNCTION(this << linkType);
    m_linkType = linkType;
}

template <typename T>
uint16_t
GlobalRoutingLinkRecord<T>::GetMetric() const
{
    NS_LOG_FUNCTION(this);
    return m_metric;
}

template <typename T>
void
GlobalRoutingLinkRecord<T>::SetMetric(uint16_t metric)
{
    NS_LOG_FUNCTION(this << metric);
    m_metric = metric;
}

template <typename T>
void
GlobalRoutingLinkRecord<T>::SetLinkLocData(IpAddress linkLocData)
{
    NS_LOG_FUNCTION(this << linkLocData);
    m_linkLocData = linkLocData;
}

template <typename T>
GlobalRoutingLinkRecord<T>::IpAddress
GlobalRoutingLinkRecord<T>::GetLinkLocData()
{
    NS_LOG_FUNCTION(this);
    return m_linkLocData;
}

// ---------------------------------------------------------------------------
//
// GlobalRoutingLSA Implementation
//
// ---------------------------------------------------------------------------

template <typename T>
GlobalRoutingLSA<T>::GlobalRoutingLSA()
    : m_lsType(GlobalRoutingLSA::Unknown),
      m_linkStateId(IpAddress::GetZero()),
      m_advertisingRtr(IpAddress::GetZero()),
      m_linkRecords(),
      m_networkLSANetworkMask(IpMaskOrPrefix::GetZero()),
      m_attachedRouters(),
      m_status(GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED),
      m_node_id(0)
{
    NS_LOG_FUNCTION(this);
}

template <typename T>
GlobalRoutingLSA<T>::GlobalRoutingLSA(GlobalRoutingLSA::SPFStatus status,
                                      IpAddress linkStateId,
                                      IpAddress advertisingRtr)
    : m_lsType(GlobalRoutingLSA::Unknown),
      m_linkStateId(linkStateId),
      m_advertisingRtr(advertisingRtr),
      m_linkRecords(),
      m_networkLSANetworkMask(IpMaskOrPrefix::GetZero()),
      m_attachedRouters(),
      m_status(status),
      m_node_id(0)
{
    NS_LOG_FUNCTION(this << status << linkStateId << advertisingRtr);
}

template <typename T>
GlobalRoutingLSA<T>::GlobalRoutingLSA(GlobalRoutingLSA& lsa)
    : m_lsType(lsa.m_lsType),
      m_linkStateId(lsa.m_linkStateId),
      m_advertisingRtr(lsa.m_advertisingRtr),
      m_networkLSANetworkMask(lsa.m_networkLSANetworkMask),
      m_status(lsa.m_status),
      m_node_id(lsa.m_node_id)
{
    NS_LOG_FUNCTION(this << &lsa);
    NS_ASSERT_MSG(IsEmpty(), "GlobalRoutingLSA::GlobalRoutingLSA (): Non-empty LSA in constructor");
    CopyLinkRecords(lsa);
}

template <typename T>
GlobalRoutingLSA<T>&
GlobalRoutingLSA<T>::operator=(const GlobalRoutingLSA<T>& lsa)
{
    NS_LOG_FUNCTION(this << &lsa);
    m_lsType = lsa.m_lsType;
    m_linkStateId = lsa.m_linkStateId;
    m_advertisingRtr = lsa.m_advertisingRtr;
    m_networkLSANetworkMask = lsa.m_networkLSANetworkMask, m_status = lsa.m_status;
    m_node_id = lsa.m_node_id;

    ClearLinkRecords();
    CopyLinkRecords(lsa);
    return *this;
}

template <typename T>
void
GlobalRoutingLSA<T>::CopyLinkRecords(const GlobalRoutingLSA& lsa)
{
    NS_LOG_FUNCTION(this << &lsa);
    for (auto i = lsa.m_linkRecords.begin(); i != lsa.m_linkRecords.end(); i++)
    {
        GlobalRoutingLinkRecord<T>* pSrc = *i;
        auto pDst = new GlobalRoutingLinkRecord<T>;

        pDst->SetLinkType(pSrc->GetLinkType());
        pDst->SetLinkId(pSrc->GetLinkId());
        pDst->SetLinkData(pSrc->GetLinkData());
        pDst->SetLinkLocData(pSrc->GetLinkLocData());
        pDst->SetMetric(pSrc->GetMetric());

        m_linkRecords.push_back(pDst);
        pDst = nullptr;
    }

    m_attachedRouters = lsa.m_attachedRouters;
}

template <typename T>
GlobalRoutingLSA<T>::~GlobalRoutingLSA()
{
    NS_LOG_FUNCTION(this);
    ClearLinkRecords();
}

template <typename T>
void
GlobalRoutingLSA<T>::ClearLinkRecords()
{
    NS_LOG_FUNCTION(this);
    for (auto i = m_linkRecords.begin(); i != m_linkRecords.end(); i++)
    {
        NS_LOG_LOGIC("Free link record");

        GlobalRoutingLinkRecord<T>* p = *i;
        delete p;
        p = nullptr;

        *i = nullptr;
    }
    NS_LOG_LOGIC("Clear list");
    m_linkRecords.clear();
}

template <typename T>
uint32_t
GlobalRoutingLSA<T>::AddLinkRecord(GlobalRoutingLinkRecord<T>* lr)
{
    NS_LOG_FUNCTION(this << lr);
    m_linkRecords.push_back(lr);
    return m_linkRecords.size();
}

template <typename T>
uint32_t
GlobalRoutingLSA<T>::GetNLinkRecords() const
{
    NS_LOG_FUNCTION(this);
    return m_linkRecords.size();
}

template <typename T>
GlobalRoutingLinkRecord<T>*
GlobalRoutingLSA<T>::GetLinkRecord(uint32_t n) const
{
    NS_LOG_FUNCTION(this << n);
    uint32_t j = 0;
    for (auto i = m_linkRecords.begin(); i != m_linkRecords.end(); i++, j++)
    {
        if (j == n)
        {
            return *i;
        }
    }
    NS_ASSERT_MSG(false, "GlobalRoutingLSA::GetLinkRecord (): invalid index");
    return nullptr;
}

template <typename T>
bool
GlobalRoutingLSA<T>::IsEmpty() const
{
    NS_LOG_FUNCTION(this);
    return m_linkRecords.empty();
}

template <typename T>
GlobalRoutingLSA<T>::LSType
GlobalRoutingLSA<T>::GetLSType() const
{
    NS_LOG_FUNCTION(this);
    return m_lsType;
}

template <typename T>
void
GlobalRoutingLSA<T>::SetLSType(GlobalRoutingLSA::LSType typ)
{
    NS_LOG_FUNCTION(this << typ);
    m_lsType = typ;
}

template <typename T>
GlobalRoutingLSA<T>::IpAddress
GlobalRoutingLSA<T>::GetLinkStateId() const
{
    NS_LOG_FUNCTION(this);
    return m_linkStateId;
}

template <typename T>
void
GlobalRoutingLSA<T>::SetLinkStateId(IpAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_linkStateId = addr;
}

template <typename T>
GlobalRoutingLSA<T>::IpAddress
GlobalRoutingLSA<T>::GetAdvertisingRouter() const
{
    NS_LOG_FUNCTION(this);
    return m_advertisingRtr;
}

template <typename T>
void
GlobalRoutingLSA<T>::SetAdvertisingRouter(IpAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_advertisingRtr = addr;
}

template <typename T>
void
GlobalRoutingLSA<T>::SetNetworkLSANetworkMask(IpMaskOrPrefix mask)
{
    NS_LOG_FUNCTION(this << mask);
    m_networkLSANetworkMask = mask;
}

template <typename T>
GlobalRoutingLSA<T>::IpMaskOrPrefix
GlobalRoutingLSA<T>::GetNetworkLSANetworkMask() const
{
    NS_LOG_FUNCTION(this);
    return m_networkLSANetworkMask;
}

template <typename T>
GlobalRoutingLSA<T>::SPFStatus
GlobalRoutingLSA<T>::GetStatus() const
{
    NS_LOG_FUNCTION(this);
    return m_status;
}

template <typename T>
uint32_t
GlobalRoutingLSA<T>::AddAttachedRouter(IpAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_attachedRouters.push_back(addr);
    return m_attachedRouters.size();
}

template <typename T>
uint32_t
GlobalRoutingLSA<T>::GetNAttachedRouters() const
{
    NS_LOG_FUNCTION(this);
    return m_attachedRouters.size();
}

template <typename T>
GlobalRoutingLSA<T>::IpAddress
GlobalRoutingLSA<T>::GetAttachedRouter(uint32_t n) const
{
    NS_LOG_FUNCTION(this << n);
    uint32_t j = 0;
    for (auto i = m_attachedRouters.begin(); i != m_attachedRouters.end(); i++, j++)
    {
        if (j == n)
        {
            return *i;
        }
    }
    NS_ASSERT_MSG(false, "GlobalRoutingLSA::GetAttachedRouter (): invalid index");
    return IpAddress::GetZero();
}

template <typename T>
void
GlobalRoutingLSA<T>::SetStatus(GlobalRoutingLSA::SPFStatus status)
{
    NS_LOG_FUNCTION(this << status);
    m_status = status;
}

template <typename T>
Ptr<Node>
GlobalRoutingLSA<T>::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return NodeList::GetNode(m_node_id);
}

template <typename T>
void
GlobalRoutingLSA<T>::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node_id = node->GetId();
}

template <typename T>
void
GlobalRoutingLSA<T>::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << std::endl;
    os << "========== Global Routing LSA ==========" << std::endl;
    os << "m_lsType = " << m_lsType;
    if (m_lsType == GlobalRoutingLSA::RouterLSA)
    {
        os << " (GlobalRoutingLSA::RouterLSA)";
    }
    else if (m_lsType == GlobalRoutingLSA::NetworkLSA)
    {
        os << " (GlobalRoutingLSA::NetworkLSA)";
    }
    else if (m_lsType == GlobalRoutingLSA::ASExternalLSAs)
    {
        os << " (GlobalRoutingLSA::ASExternalLSA)";
    }
    else
    {
        os << "(Unknown LSType)";
    }
    os << std::endl;

    os << "m_linkStateId = " << m_linkStateId << " (Router ID)" << std::endl;
    os << "m_advertisingRtr = " << m_advertisingRtr << " (Router ID)" << std::endl;

    if (m_lsType == GlobalRoutingLSA::RouterLSA)
    {
        for (auto i = m_linkRecords.begin(); i != m_linkRecords.end(); i++)
        {
            GlobalRoutingLinkRecord<T>* p = *i;

            os << "---------- RouterLSA Link Record ----------" << std::endl;
            os << "m_linkType = " << p->m_linkType;
            if (p->m_linkType == GlobalRoutingLinkRecord<T>::PointToPoint)
            {
                os << " (GlobalRoutingLinkRecord::PointToPoint)" << std::endl;
                os << "m_linkId = " << p->m_linkId << std::endl;
                os << "m_linkData = " << p->m_linkData << std::endl;
                os << "m_metric = " << p->m_metric << std::endl;
            }
            else if (p->m_linkType == GlobalRoutingLinkRecord<T>::TransitNetwork)
            {
                os << " (GlobalRoutingLinkRecord::TransitNetwork)" << std::endl;
                os << "m_linkId = " << p->m_linkId << " (Designated router for network)"
                   << std::endl;
                os << "m_linkData = " << p->m_linkData << " (This router's IP address)"
                   << std::endl;
                os << "m_metric = " << p->m_metric << std::endl;
            }
            else if (p->m_linkType == GlobalRoutingLinkRecord<T>::StubNetwork)
            {
                os << " (GlobalRoutingLinkRecord::StubNetwork)" << std::endl;
                os << "m_linkId = " << p->m_linkId << " (Network number of attached network)"
                   << std::endl;
                os << "m_linkData = " << p->m_linkData << " (Network mask of attached network)"
                   << std::endl;
                os << "m_metric = " << p->m_metric << std::endl;
            }
            else
            {
                os << " (Unknown LinkType)" << std::endl;
                os << "m_linkId = " << p->m_linkId << std::endl;
                os << "m_linkData = " << p->m_linkData << std::endl;
                os << "m_metric = " << p->m_metric << std::endl;
            }
            os << "---------- End RouterLSA Link Record ----------" << std::endl;
        }
    }
    else if (m_lsType == GlobalRoutingLSA::NetworkLSA)
    {
        os << "---------- NetworkLSA Link Record ----------" << std::endl;
        os << "m_networkLSANetworkMask = " << m_networkLSANetworkMask << std::endl;
        for (auto i = m_attachedRouters.begin(); i != m_attachedRouters.end(); i++)
        {
            os << "attachedRouter = " << *i << std::endl;
        }
        os << "---------- End NetworkLSA Link Record ----------" << std::endl;
    }
    else if (m_lsType == GlobalRoutingLSA::ASExternalLSAs)
    {
        os << "---------- ASExternalLSA Link Record --------" << std::endl;
        os << "m_linkStateId = " << m_linkStateId << std::endl;
        os << "m_networkLSANetworkMask = " << m_networkLSANetworkMask << std::endl;
    }
    else
    {
        NS_ASSERT_MSG(0, "Illegal LSA LSType: " << m_lsType);
    }
    os << "========== End Global Routing LSA ==========" << std::endl;
}

template <typename T>
std::ostream&
operator<<(std::ostream& os, GlobalRoutingLSA<T>& lsa)
{
    lsa.Print(os);
    return os;
}

// ---------------------------------------------------------------------------
//
// GlobalRouter Implementation
//
// ---------------------------------------------------------------------------

template <typename T>
TypeId
GlobalRouter<T>::GetTypeId()
{
    std::string name;
    if constexpr (IsIpv4)
    {
        name = "Ipv4";
    }
    else
    {
        name = "Ipv6";
    }
    static TypeId tid = TypeId("ns3::" + name + "GlobalRouter")
                            .SetParent<Object>()
                            .SetGroupName("GlobalRouter")
                            .template AddConstructor<GlobalRouter<T>>();
    return tid;
}

template <typename T>
GlobalRouter<T>::GlobalRouter()
    : m_LSAs()
{
    NS_LOG_FUNCTION(this);
    if constexpr (IsIpv4)
    {
        m_routerId.Set(Ipv4GlobalRouteManager::AllocateRouterId());
    }
    else
    {
        // looks ugly but gets the job done .
        // Alternatively, convert uint32_t to a uint8_t buf[16] and then use  Ipv6Address(buf)
        m_routerId = Ipv6Address::MakeIpv4MappedAddress(
            Ipv4Address(GlobalRouteManager<Ipv6Manager>::AllocateRouterId()));
    }
}

template <typename T>
GlobalRouter<T>::~GlobalRouter()
{
    NS_LOG_FUNCTION(this);
    ClearLSAs();
}

template <typename T>
void
GlobalRouter<T>::SetRoutingProtocol(Ptr<GlobalRouting<IpRoutingProtocol>> routing)
{
    NS_LOG_FUNCTION(this << routing);
    m_routingProtocol = routing;
}

template <typename T>
Ptr<GlobalRouting<typename GlobalRouter<T>::IpRoutingProtocol>>
GlobalRouter<T>::GetRoutingProtocol()
{
    NS_LOG_FUNCTION(this);
    return m_routingProtocol;
}

template <typename T>
void
GlobalRouter<T>::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_routingProtocol = nullptr;
    for (auto k = m_injectedRoutes.begin(); k != m_injectedRoutes.end();
         k = m_injectedRoutes.erase(k))
    {
        delete (*k);
    }
    Object::DoDispose();
}

template <typename T>
void
GlobalRouter<T>::ClearLSAs()
{
    NS_LOG_FUNCTION(this);
    for (auto i = m_LSAs.begin(); i != m_LSAs.end(); i++)
    {
        NS_LOG_LOGIC("Free LSA");

        GlobalRoutingLSA<T>* p = *i;
        delete p;
        p = nullptr;

        *i = nullptr;
    }
    NS_LOG_LOGIC("Clear list of LSAs");
    m_LSAs.clear();
}

template <typename T>
GlobalRouter<T>::IpAddress
GlobalRouter<T>::GetRouterId() const
{
    NS_LOG_FUNCTION(this);
    return m_routerId;
}

//
// DiscoverLSAs is called on all nodes in the system that have a GlobalRouter
// interface aggregated.  We need to go out and discover any adjacent routers
// and build the Link State Advertisements that reflect them and their associated
// networks.
//
template <typename T>
uint32_t
GlobalRouter<T>::DiscoverLSAs()
{
    NS_LOG_FUNCTION(this);
    Ptr<Node> node = GetObject<Node>();
    NS_ABORT_MSG_UNLESS(node,
                        "GlobalRouter::DiscoverLSAs (): GetObject for <Node> interface failed");
    NS_LOG_LOGIC("For node " << node->GetId());

    ClearLSAs();

    //
    // While building the Router-LSA, keep a list of those NetDevices for
    // which the current node is the designated router and we will later build
    // a NetworkLSA for.
    //
    NetDeviceContainer c;

    //
    // We're aggregated to a node.  We need to ask the node for a pointer to its
    // Ipv4 interface.  This is where the information regarding the attached
    // interfaces lives.  If we're a router, we had better have an Ipv4 interface.
    //
    Ptr<Ip> ipLocal = node->GetObject<Ip>();
    NS_ABORT_MSG_UNLESS(ipLocal,
                        "GlobalRouter::DiscoverLSAs (): GetObject for <Ipv4> interface failed");

    //
    // Every router node originates a Router-LSA
    //
    auto pLSA = new GlobalRoutingLSA<T>;
    pLSA->SetLSType(GlobalRoutingLSA<T>::RouterLSA);
    pLSA->SetLinkStateId(m_routerId);
    pLSA->SetAdvertisingRouter(m_routerId);
    pLSA->SetStatus(GlobalRoutingLSA<T>::LSA_SPF_NOT_EXPLORED);
    pLSA->SetNode(node);

    //
    // Ask the node for the number of net devices attached. This isn't necessarily
    // equal to the number of links to adjacent nodes (other routers) as the number
    // of devices may include those for stub networks (e.g., ethernets, etc.) and
    // bridge devices also take up an "extra" net device.
    //
    uint32_t numDevices = node->GetNDevices();

    //
    // Iterate through the devices on the node and walk the channel to see what's
    // on the other side of the standalone devices..
    //
    for (uint32_t i = 0; i < numDevices; ++i)
    {
        Ptr<NetDevice> ndLocal = node->GetDevice(i);

        if (DynamicCast<LoopbackNetDevice>(ndLocal))
        {
            continue;
        }

        //
        // There is an assumption that bridge ports must never have an IP address
        // associated with them.  This turns out to be a very convenient place to
        // check and make sure that this is the case.
        //
        if (NetDeviceIsBridged(ndLocal))
        {
            int32_t ifIndex = ipLocal->GetInterfaceForDevice(ndLocal);
            NS_ABORT_MSG_IF(
                ifIndex != -1,
                "GlobalRouter::DiscoverLSAs(): Bridge ports must not have an IPv4 interface index");
        }

        //
        // Check to see if the net device we just got has a corresponding IP
        // interface (could be a pure L2 NetDevice) -- for example a net device
        // associated with a bridge.  We are only going to involve devices with
        // IP addresses in routing.
        //
        int32_t interfaceNumber = ipLocal->GetInterfaceForDevice(ndLocal);
        if (interfaceNumber == -1 ||
            !(ipLocal->IsUp(interfaceNumber) && ipLocal->IsForwarding(interfaceNumber)))
        {
            NS_LOG_LOGIC("Net device "
                         << ndLocal
                         << "has no IP interface or is not enabled for forwarding, skipping");
            continue;
        }

        //
        // We have a net device that we need to check out.  If it supports
        // broadcast and is not a point-point link, then it will be either a stub
        // network or a transit network depending on the number of routers on
        // the segment.  We add the appropriate link record to the LSA.
        //
        // If the device is a point to point link, we treat it separately.  In
        // that case, there may be zero, one, or two link records added.
        //

        if (ndLocal->IsBroadcast() && !ndLocal->IsPointToPoint())
        {
            NS_LOG_LOGIC("Broadcast link");
            ProcessBroadcastLink(ndLocal, pLSA, c);
        }
        else if (ndLocal->IsPointToPoint())
        {
            NS_LOG_LOGIC("Point=to-point link");
            ProcessPointToPointLink(ndLocal, pLSA);
        }
        else
        {
            NS_ASSERT_MSG(0, "GlobalRouter::DiscoverLSAs (): unknown link type");
        }
    }

    NS_LOG_LOGIC("========== LSA for node " << node->GetId() << " ==========");
    NS_LOG_LOGIC(*pLSA);
    m_LSAs.push_back(pLSA);
    pLSA = nullptr;

    //
    // Now, determine whether we need to build a NetworkLSA.  This is the case if
    // we found at least one designated router.
    //
    uint32_t nDesignatedRouters = c.GetN();
    if (nDesignatedRouters > 0)
    {
        NS_LOG_LOGIC("Build Network LSAs");
        BuildNetworkLSAs(c);
    }

    //
    // Build injected route LSAs as external routes
    // RFC 2328, section 12.4.4
    //
    for (auto i = m_injectedRoutes.begin(); i != m_injectedRoutes.end(); i++)
    {
        auto pLSA = new GlobalRoutingLSA<T>;
        pLSA->SetLSType(GlobalRoutingLSA<T>::ASExternalLSAs);
        pLSA->SetLinkStateId((*i)->GetDestNetwork());
        pLSA->SetAdvertisingRouter(m_routerId);
        if constexpr (IsIpv4)
        {
            pLSA->SetNetworkLSANetworkMask((*i)->GetDestNetworkMask());
        }
        else
        {
            pLSA->SetNetworkLSANetworkMask((*i)->GetDestNetworkPrefix());
        }
        pLSA->SetStatus(GlobalRoutingLSA<T>::LSA_SPF_NOT_EXPLORED);
        m_LSAs.push_back(pLSA);
    }
    return m_LSAs.size();
}

template <typename T>
void
GlobalRouter<T>::ProcessBroadcastLink(Ptr<NetDevice> nd,
                                      GlobalRoutingLSA<T>* pLSA,
                                      NetDeviceContainer& c)
{
    NS_LOG_FUNCTION(this << nd << pLSA << &c);

    if (nd->IsBridge())
    {
        ProcessBridgedBroadcastLink(nd, pLSA, c);
    }
    else
    {
        ProcessSingleBroadcastLink(nd, pLSA, c);
    }
}

template <typename T>
void
GlobalRouter<T>::ProcessSingleBroadcastLink(Ptr<NetDevice> nd,
                                            GlobalRoutingLSA<T>* pLSA,
                                            NetDeviceContainer& c)
{
    NS_LOG_FUNCTION(this << nd << pLSA << &c);

    auto plr = new GlobalRoutingLinkRecord<T>;
    NS_ABORT_MSG_IF(plr == nullptr,
                    "GlobalRouter::ProcessSingleBroadcastLink(): Can't alloc link record");

    //
    // We have some preliminaries to do to get enough information to proceed.
    // This information we need comes from the internet stack, so notice that
    // there is an implied assumption that global routing is only going to
    // work with devices attached to the internet stack (have an ipv4 interface
    // associated to them.
    //
    Ptr<Node> node = nd->GetNode();

    Ptr<Ip> ipLocal = node->GetObject<Ip>();
    NS_ABORT_MSG_UNLESS(
        ipLocal,
        "GlobalRouter::ProcessSingleBroadcastLink (): GetObject for <Ipv4> interface failed");

    int32_t interfaceLocal = ipLocal->GetInterfaceForDevice(nd);
    NS_ABORT_MSG_IF(
        interfaceLocal == -1,
        "GlobalRouter::ProcessSingleBroadcastLink(): No interface index associated with device");

    if (ipLocal->GetNAddresses(interfaceLocal) > 1)
    {
        NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the primary one");
    }
    IpAddress addrLocal;
    IpAddress addrLinkLocal; // this is the link local associated with the link for ipv6
    if constexpr (IsIpv4)
    {
        addrLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
    }
    else
    {
        addrLinkLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
        addrLocal = ipLocal->GetAddress(interfaceLocal, 1).GetAddress();
    }

    IpMaskOrPrefix maskLocal;
    if constexpr (IsIpv4)
    {
        maskLocal = ipLocal->GetAddress(interfaceLocal, 0).GetMask();
    }
    else
    {
        maskLocal = ipLocal->GetAddress(interfaceLocal, 1).GetPrefix();
    }
    NS_LOG_LOGIC("Working with local address " << addrLocal);
    uint16_t metricLocal = ipLocal->GetMetric(interfaceLocal);

    //
    // Check to see if the net device is connected to a channel/network that has
    // another router on it.  If there is no other router on the link (but us) then
    // this is a stub network.  If we find another router, then what we have here
    // is a transit network.
    //
    ClearBridgesVisited();
    if (!AnotherRouterOnLink(nd))
    {
        //
        // This is a net device connected to a stub network
        //
        NS_LOG_LOGIC("Router-LSA Stub Network");
        plr->SetLinkType(GlobalRoutingLinkRecord<T>::StubNetwork);

        //
        // According to OSPF, the Link ID is the IP network number of
        // the attached network.
        //
        if constexpr (IsIpv4)
        {
            plr->SetLinkId(addrLocal.CombineMask(maskLocal));
        }
        else
        {
            plr->SetLinkId(addrLocal.CombinePrefix(maskLocal));
        }

        //
        // and the Link Data is the network mask; converted to Ipv4Address
        //
        IpAddress maskLocalAddr;
        if constexpr (IsIpv4)
        {
            maskLocalAddr.Set(maskLocal.Get());
        }
        else
        {
            uint8_t buf[16];
            maskLocal.GetBytes(buf);
            maskLocalAddr.Set(buf);
        }
        plr->SetLinkData(maskLocalAddr);
        plr->SetMetric(metricLocal);
        pLSA->AddLinkRecord(plr);
        plr = nullptr;
    }
    else
    {
        //
        // We have multiple routers on a broadcast interface, so this is
        // a transit network.
        //
        NS_LOG_LOGIC("Router-LSA Transit Network");
        plr->SetLinkType(GlobalRoutingLinkRecord<T>::TransitNetwork);

        //
        // By definition, the router with the lowest IP address is the
        // designated router for the network.  OSPF says that the Link ID
        // gets the IP interface address of the designated router in this
        // case.
        //
        ClearBridgesVisited();
        IpAddress designatedRtr;
        designatedRtr = FindDesignatedRouterForLink(nd);

        //
        // Let's double-check that any designated router we find out on our
        // network is really on our network.
        //
        bool isRtrAllOnes = false;
        if constexpr (IsIpv4)
        {
            if (designatedRtr.IsBroadcast())
            {
                isRtrAllOnes = true;
            }
        }
        else
        {
            if (designatedRtr == Ipv6Address::GetOnes())
            {
                isRtrAllOnes = true;
            }
        }

        if (!isRtrAllOnes)
        {
            IpAddress networkHere;
            IpAddress networkThere;
            if constexpr (IsIpv4)
            {
                networkHere = addrLocal.CombineMask(maskLocal);
                networkThere = designatedRtr.CombineMask(maskLocal);
            }
            else
            {
                networkHere = addrLocal.CombinePrefix(maskLocal);
                networkThere = designatedRtr.CombinePrefix(maskLocal);
            }
            NS_ABORT_MSG_UNLESS(
                networkHere == networkThere,
                "GlobalRouter::ProcessSingleBroadcastLink(): Network number confusion ("
                    << addrLocal << "/" << maskLocal.GetPrefixLength() << ", " << designatedRtr
                    << "/" << maskLocal.GetPrefixLength() << ")");
        }
        if (designatedRtr == addrLocal)
        {
            c.Add(nd);
            NS_LOG_LOGIC("Node " << node->GetId() << " elected a designated router");
        }
        plr->SetLinkId(designatedRtr);

        //
        // OSPF says that the Link Data is this router's own IP address.
        //
        plr->SetLinkData(addrLocal);
        plr->SetLinkLocData(addrLinkLocal);
        plr->SetMetric(metricLocal);
        pLSA->AddLinkRecord(plr);
        plr = nullptr;
    }
}

template <typename T>
void
GlobalRouter<T>::ProcessBridgedBroadcastLink(Ptr<NetDevice> nd,
                                             GlobalRoutingLSA<T>* pLSA,
                                             NetDeviceContainer& c)
{
    NS_LOG_FUNCTION(this << nd << pLSA << &c);
    NS_ASSERT_MSG(nd->IsBridge(),
                  "GlobalRouter::ProcessBridgedBroadcastLink(): Called with non-bridge net device");

#if 0
  //
  // It is possible to admit the possibility that a bridge device on a node
  // can also participate in routing.  This would surprise people who don't
  // come from Microsoft-land where they do use such a construct.  Based on
  // the principle of least-surprise, we will leave the relatively simple
  // code in place to do this, but not enable it until someone really wants
  // the capability.  Even then, we will not enable this code as a default
  // but rather something you will have to go and turn on.
  //

  Ptr<BridgeNetDevice> bnd = nd->GetObject<BridgeNetDevice> ();
  NS_ABORT_MSG_UNLESS (bnd, "GlobalRouter::DiscoverLSAs (): GetObject for <BridgeNetDevice> failed");

  //
  // We have some preliminaries to do to get enough information to proceed.
  // This information we need comes from the internet stack, so notice that
  // there is an implied assumption that global routing is only going to
  // work with devices attached to the internet stack (have an ipv4 interface
  // associated to them.
  //
  Ptr<Node> node = nd->GetNode ();
  Ptr<Ip> ipLocal = node->GetObject<Ip> ();
  NS_ABORT_MSG_UNLESS (ipv4Local, "GlobalRouter::ProcessBridgedBroadcastLink (): GetObject for <Ipv4> interface failed");

  int32_t interfaceLocal = ipLocal->GetInterfaceForDevice (nd);
  NS_ABORT_MSG_IF (interfaceLocal == -1, "GlobalRouter::ProcessBridgedBroadcastLink(): No interface index associated with device");

  if (ipLocal->GetNAddresses (interfaceLocal) > 1)
    {
      NS_LOG_WARN ("Warning, interface has multiple IP addresses; using only the primary one");
    }
   IpAddress addrLocal;
    IpAddress addrLinkLocal; // this is the link local associated with the link for ipv6
    if constexpr (IsIpv4)
    {
        addrLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
    }
    else
    {
        addrLinkLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
        addrLocal = ipLocal->GetAddress(interfaceLocal, 1).GetAddress();
    }

    IpMaskOrPrefix maskLocal;
    if constexpr (IsIpv4)
    {
        maskLocal = ipLocal->GetAddress(interfaceLocal, 0).GetMask();
    }
    else
    {
        maskLocal = ipLocal->GetAddress(interfaceLocal, 1).GetPrefix();
    }
  NS_LOG_LOGIC ("Working with local address " << addrLocal);
  uint16_t metricLocal = ipv4Local->GetMetric (interfaceLocal);

  //
  // We need to handle a bridge on the router.  This means that we have been
  // given a net device that is a BridgeNetDevice.  It has an associated Ipv4
  // interface index and address.  Some number of other net devices live "under"
  // the bridge device as so-called bridge ports.  In a nutshell, what we have
  // to do is to repeat what is done for a single broadcast link on all of
  // those net devices living under the bridge (trolls?)
  //

  bool areTransitNetwork = false;
  IpAddress designatedRtr;

  for (uint32_t i = 0; i < bnd->GetNBridgePorts (); ++i)
    {
      Ptr<NetDevice> ndTemp = bnd->GetBridgePort (i);

      //
      // We have to decide if we are a transit network.  This is characterized
      // by the presence of another router on the network segment.  If we find
      // another router on any of our bridged links, we are a transit network.
      //
      ClearBridgesVisited ();
      if (AnotherRouterOnLink (ndTemp))
        {
          areTransitNetwork = true;

          //
          // If we're going to be a transit network, then we have got to elect
          // a designated router for the whole bridge.  This means finding the
          // router with the lowest IP address on the whole bridge.  We ask
          // for the lowest address on each segment and pick the lowest of them
          // all.
          //
          ClearBridgesVisited ();
          IpAddress designatedRtrTemp = FindDesignatedRouterForLink (ndTemp);

          //
          // Let's double-check that any designated router we find out on our
          // network is really on our network.
          //
         bool isRtrAllOnes = false;
        if constexpr (IsIpv4)
        {
            if (designatedRtrTemp.IsBroadcast())
            {
                isRtrAllOnes = true;
            }
        }
        else
        {
            if (designatedRtrTemp == Ipv6Address::GetOnes())
            {
                isRtrAllOnes = true;
            }
        }

        if (!isRtrAllOnes)
        {
            IpAddress networkHere;
            IpAddress networkThere;
            if constexpr (IsIpv4)
            {
                networkHere = addrLocal.CombineMask(maskLocal);
                networkThere = designatedRtrTemp.CombineMask(maskLocal);
            }
            else
            {
                networkHere = addrLocal.CombinePrefix(maskLocal);
                networkThere = designatedRtrTemp.CombinePrefix(maskLocal);
            }
            NS_ABORT_MSG_UNLESS(
                networkHere == networkThere,
                "GlobalRouter::ProcessSingleBroadcastLink(): Network number confusion ("
                    << addrLocal << "/" << maskLocal.GetPrefixLength() << ", " << designatedRtr
                    << "/" << maskLocal.GetPrefixLength() << ")");
        }
          if (designatedRtrTemp < designatedRtr)
            {
              designatedRtr = designatedRtrTemp;
            }
        }
    }
  //
  // That's all the information we need to put it all together, just like we did
  // in the case of a single broadcast link.
  //

  GlobalRoutingLinkRecord *plr = new GlobalRoutingLinkRecord;
  NS_ABORT_MSG_IF (plr == 0, "GlobalRouter::ProcessBridgedBroadcastLink(): Can't alloc link record");

  if (areTransitNetwork == false)
    {
      //
      // This is a net device connected to a bridge of stub networks
      //
      NS_LOG_LOGIC ("Router-LSA Stub Network");
      plr->SetLinkType (GlobalRoutingLinkRecord::StubNetwork);

      //
      // According to OSPF, the Link ID is the IP network number of
      // the attached network.
      //
       if constexpr (IsIpv4)
        {
            plr->SetLinkId(addrLocal.CombineMask(maskLocal));
        }
        else
        {
            plr->SetLinkId(addrLocal.CombinePrefix(maskLocal));
        }
      //
      // and the Link Data is the network mask; converted to Ipv4Address
      //
IpAddress maskLocalAddr;
        if constexpr (IsIpv4)
        {
            maskLocalAddr.Set(maskLocal.Get());
        }
        else
        {
            uint8_t buf[16];
            maskLocal.GetBytes(buf);
            maskLocalAddr.Set(buf);
        }
        plr->SetLinkData(maskLocalAddr);
      plr->SetMetric (metricLocal);
      pLSA->AddLinkRecord (plr);
      plr = 0;
    }
  else
    {
      //
      // We have multiple routers on a bridged broadcast interface, so this is
      // a transit network.
      //
      NS_LOG_LOGIC ("Router-LSA Transit Network");
      plr->SetLinkType (GlobalRoutingLinkRecord::TransitNetwork);

      //
      // By definition, the router with the lowest IP address is the
      // designated router for the network.  OSPF says that the Link ID
      // gets the IP interface address of the designated router in this
      // case.
      //
      if (designatedRtr == addrLocal)
        {
          c.Add (nd);
          NS_LOG_LOGIC ("Node " << node->GetId () << " elected a designated router");
        }
      plr->SetLinkId (designatedRtr);

      //
      // OSPF says that the Link Data is this router's own IP address.
      //
      plr->SetLinkData (addrLocal);
      plr->SetLinkLocData (addrLinkLocal);
      plr->SetMetric (metricLocal);
      pLSA->AddLinkRecord (plr);
      plr = 0;
    }
#endif
}

template <typename T>
void
GlobalRouter<T>::ProcessPointToPointLink(Ptr<NetDevice> ndLocal, GlobalRoutingLSA<T>* pLSA)
{
    NS_LOG_FUNCTION(this << ndLocal << pLSA);

    //
    // We have some preliminaries to do to get enough information to proceed.
    // This information we need comes from the internet stack, so notice that
    // there is an implied assumption that global routing is only going to
    // work with devices attached to the internet stack (have an ipv4 interface
    // associated to them.
    //
    Ptr<Node> nodeLocal = ndLocal->GetNode();

    Ptr<Ip> ipLocal = nodeLocal->GetObject<Ip>();
    NS_ABORT_MSG_UNLESS(
        ipLocal,
        "GlobalRouter::ProcessPointToPointLink (): GetObject for <Ipv4> interface failed");

    int32_t interfaceLocal = ipLocal->GetInterfaceForDevice(ndLocal);
    NS_ABORT_MSG_IF(
        interfaceLocal == -1,
        "GlobalRouter::ProcessPointToPointLink (): No interface index associated with device");

    if ((ipLocal->GetNAddresses(interfaceLocal) == 0))
    {
        NS_LOG_LOGIC("Local interface " << interfaceLocal << " has no address");
        return;
    }

    IpAddress addrLocal;
    IpAddress addrLinkLocal; // this is the link local associated with the link for ipv6
    if constexpr (IsIpv4)
    {
        if (ipLocal->GetNAddresses(interfaceLocal) > 1)
        {
            NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the primary one");
        }
        addrLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
    }
    else
    {
        addrLinkLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
        if (ipLocal->GetNAddresses(interfaceLocal) >
            1) // if there is a globalUnicast address for the interface
        {
            addrLocal = ipLocal->GetAddress(interfaceLocal, 1).GetAddress();
        }
    }
    IpMaskOrPrefix maskLocal;
    if constexpr (IsIpv4)
    {
        maskLocal = ipLocal->GetAddress(interfaceLocal, 0).GetMask();
    }
    else
    {
        if (ipLocal->GetNAddresses(interfaceLocal) >
            1) // if there is a globalUnicast address for the remote interface
        {
            maskLocal = ipLocal->GetAddress(interfaceLocal, 1).GetPrefix();
        }
    }
    NS_LOG_LOGIC("Working with local address " << addrLocal);
    uint16_t metricLocal = ipLocal->GetMetric(interfaceLocal);

    //
    // Now, we're going to walk over to the remote net device on the other end of
    // the point-to-point channel we know we have.  This is where our adjacent
    // router (to use OSPF lingo) is running.
    //
    Ptr<Channel> ch = ndLocal->GetChannel();

    //
    // Get the net device on the other side of the point-to-point channel.
    //
    Ptr<NetDevice> ndRemote = GetAdjacent(ndLocal, ch);

    //
    // The adjacent net device is aggregated to a node.  We need to ask that net
    // device for its node, then ask that node for its Ipv4 interface.  Note a
    // requirement that nodes on either side of a point-to-point link must have
    // internet stacks; and an assumption that point-to-point links are incompatible
    // with bridging.
    //
    Ptr<Node> nodeRemote = ndRemote->GetNode();
    Ptr<Ip> ipRemote = nodeRemote->GetObject<Ip>();
    NS_ABORT_MSG_UNLESS(
        ipRemote,
        "GlobalRouter::ProcessPointToPointLink(): GetObject for remote <Ipv4> failed");

    //
    // Further note the requirement that nodes on either side of a point-to-point
    // link must participate in global routing and therefore have a GlobalRouter
    // interface aggregated.
    //
    Ptr<GlobalRouter> rtrRemote = nodeRemote->GetObject<GlobalRouter>();
    if (!rtrRemote)
    {
        // This case is possible if the remote does not participate in global routing
        return;
    }
    //
    // We're going to need the remote router ID, so we might as well get it now.
    //
    IpAddress rtrIdRemote = rtrRemote->GetRouterId();
    NS_LOG_LOGIC("Working with remote router " << rtrIdRemote);

    //
    // Now, just like we did above, we need to get the IP interface index for the
    // net device on the other end of the point-to-point channel.
    //
    int32_t interfaceRemote = ipRemote->GetInterfaceForDevice(ndRemote);
    NS_ABORT_MSG_IF(interfaceRemote == -1,
                    "GlobalRouter::ProcessPointToPointLinks(): No interface index associated with "
                    "remote device");

    if ((ipRemote->GetNAddresses(interfaceRemote) == 0))
    {
        NS_LOG_LOGIC("Remote interface " << interfaceRemote << " has no address");
    }
    //
    // Now that we have the Ipv4 interface, we can get the (remote) address and
    // mask we need.
    //

    IpAddress addrRemote;
    if constexpr (IsIpv4)
    {
        if (ipRemote->GetNAddresses(interfaceRemote) > 1)
        {
            NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the primary one");
        }
        addrRemote = ipRemote->GetAddress(interfaceRemote, 0).GetAddress();
    }
    else
    {
        if (ipRemote->GetNAddresses(interfaceRemote) >
            1) // if there is a globalUnicast address for the interface
        {
            addrRemote = ipRemote->GetAddress(interfaceRemote, 1).GetAddress();
        }
    }
    IpMaskOrPrefix maskRemote;
    if constexpr (IsIpv4)
    {
        maskRemote = ipRemote->GetAddress(interfaceRemote, 0).GetMask();
    }
    else
    {
        if (ipRemote->GetNAddresses(interfaceRemote) >
            1) // if there is a globalUnicast address for the remote interface
        {
            maskRemote = ipRemote->GetAddress(interfaceRemote, 1).GetPrefix();
        }
    }
    NS_LOG_LOGIC("Working with remote address " << addrRemote);

    //
    // Now we can fill out the link records for this link.  There are always two
    // link records; the first is a point-to-point record describing the link and
    // the second is a stub network record with the network number.
    //
    GlobalRoutingLinkRecord<T>* plr;

    if (ipRemote->IsUp(interfaceRemote))
    {
        NS_LOG_LOGIC("Remote side interface " << interfaceRemote << " is up-- add a type 1 link");

        plr = new GlobalRoutingLinkRecord<T>;
        NS_ABORT_MSG_IF(plr == nullptr,
                        "GlobalRouter::ProcessPointToPointLink(): Can't alloc link record");
        plr->SetLinkType(GlobalRoutingLinkRecord<T>::PointToPoint);
        plr->SetLinkId(rtrIdRemote);
        plr->SetLinkData(addrLocal);
        plr->SetLinkLocData(addrLinkLocal);
        plr->SetMetric(metricLocal);
        pLSA->AddLinkRecord(plr);
        plr = nullptr;
    }

    // Regardless of state of peer, add a type 3 link (RFC 2328: 12.4.1.1)
    // the only exception is if the remote interface has a link local address only, then it behaves
    // as a router and not a host
    if constexpr (!IsIpv4)
    {
        if (ipRemote->GetNAddresses(interfaceRemote) == 1 &&
            ipRemote->GetAddress(interfaceRemote, 0).GetAddress().IsLinkLocal())
        {
            NS_LOG_LOGIC("The remote interface only has a link local address, not adding a type 3 "
                         "link record");
            return;
        }
    }

    plr = new GlobalRoutingLinkRecord<T>;
    NS_ABORT_MSG_IF(plr == nullptr,
                    "GlobalRouter::ProcessPointToPointLink(): Can't alloc link record");
    plr->SetLinkType(GlobalRoutingLinkRecord<T>::StubNetwork);
    plr->SetLinkId(addrRemote);
    if constexpr (IsIpv4)
    {
        plr->SetLinkData(Ipv4Address(maskRemote.Get())); // Frown
    }
    else
    {
        uint8_t buf[16];
        maskRemote.GetBytes(buf);
        plr->SetLinkData(Ipv6Address(buf)); // Frown
    }
    plr->SetMetric(metricLocal);
    pLSA->AddLinkRecord(plr);
    plr = nullptr;
}

template <typename T>
void
GlobalRouter<T>::BuildNetworkLSAs(NetDeviceContainer c)
{
    NS_LOG_FUNCTION(this << &c);

    uint32_t nDesignatedRouters = c.GetN();
    NS_LOG_DEBUG("Number of designated routers: " << nDesignatedRouters);

    for (uint32_t i = 0; i < nDesignatedRouters; ++i)
    {
        //
        // Build one NetworkLSA for each net device talking to a network that we are the
        // designated router for.  These devices are in the provided container.
        //
        Ptr<NetDevice> ndLocal = c.Get(i);
        Ptr<Node> node = ndLocal->GetNode();

        Ptr<Ip> ipLocal = node->GetObject<Ip>();
        NS_ABORT_MSG_UNLESS(
            ipLocal,
            "GlobalRouter::ProcessPointToPointLink (): GetObject for <Ipv4> interface failed");

        int32_t interfaceLocal = ipLocal->GetInterfaceForDevice(ndLocal);
        NS_ABORT_MSG_IF(
            interfaceLocal == -1,
            "GlobalRouter::BuildNetworkLSAs (): No interface index associated with device");

        if (ipLocal->GetNAddresses(interfaceLocal) > 1)
        {
            NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the primary one");
        }
        IpAddress addrLocal;
        if constexpr (IsIpv4)
        {
            addrLocal = ipLocal->GetAddress(interfaceLocal, 0).GetAddress();
        }
        else
        {
            addrLocal = ipLocal->GetAddress(interfaceLocal, 1).GetAddress();
        }
        IpMaskOrPrefix maskLocal;
        if constexpr (IsIpv4)
        {
            maskLocal = ipLocal->GetAddress(interfaceLocal, 0).GetMask();
        }
        else
        {
            maskLocal = ipLocal->GetAddress(interfaceLocal, 1).GetPrefix();
        }

        auto pLSA = new GlobalRoutingLSA<T>;
        NS_ABORT_MSG_IF(pLSA == nullptr,
                        "GlobalRouter::BuildNetworkLSAs(): Can't alloc link record");

        pLSA->SetLSType(GlobalRoutingLSA<T>::NetworkLSA);
        pLSA->SetLinkStateId(addrLocal);
        pLSA->SetAdvertisingRouter(m_routerId);
        pLSA->SetNetworkLSANetworkMask(maskLocal);
        pLSA->SetStatus(GlobalRoutingLSA<T>::LSA_SPF_NOT_EXPLORED);
        pLSA->SetNode(node);

        //
        // Build a list of AttachedRouters by walking the devices in the channel
        // and, if we find a node with a GlobalRouter interface and an IPv4
        // interface associated with that device, we call it an attached router.
        //
        ClearBridgesVisited();
        Ptr<Channel> ch = ndLocal->GetChannel();
        std::size_t nDevices = ch->GetNDevices();
        NS_ASSERT(nDevices);
        NetDeviceContainer deviceList = FindAllNonBridgedDevicesOnLink(ch);
        NS_LOG_LOGIC("Found " << deviceList.GetN() << " non-bridged devices on channel");

        for (uint32_t i = 0; i < deviceList.GetN(); i++)
        {
            Ptr<NetDevice> tempNd = deviceList.Get(i);
            NS_ASSERT(tempNd);
            if (tempNd == ndLocal)
            {
                NS_LOG_LOGIC("Adding " << addrLocal << " to Network LSA");
                pLSA->AddAttachedRouter(addrLocal);
                continue;
            }
            Ptr<Node> tempNode = tempNd->GetNode();

            // Does the node in question have a GlobalRouter interface?  If not it can
            // hardly be considered an attached router.
            //
            Ptr<GlobalRouter> rtr = tempNode->GetObject<GlobalRouter>();
            if (!rtr)
            {
                NS_LOG_LOGIC("Node " << tempNode->GetId()
                                     << " does not have GlobalRouter interface--skipping");
                continue;
            }

            //
            // Does the attached node have an ipv4 interface for the device we're probing?
            // If not, it can't play router.
            //
            Ptr<Ip> tempIp = tempNode->GetObject<Ip>();
            int32_t tempInterface = tempIp->GetInterfaceForDevice(tempNd);

            if (tempInterface != -1)
            {
                Ptr<Ip> tempIp = tempNode->GetObject<Ip>();
                NS_ASSERT(tempIp);
                if (!tempIp->IsUp(tempInterface))
                {
                    NS_LOG_LOGIC("Remote side interface " << tempInterface << " not up");
                }
                else
                {
                    if (tempIp->GetNAddresses(tempInterface) > 1)
                    {
                        NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the "
                                    "primary one");
                    }
                    IpAddress tempAddr;
                    if constexpr (IsIpv4)
                    {
                        tempAddr = tempIp->GetAddress(tempInterface, 0).GetAddress();
                    }
                    else
                    {
                        tempAddr = tempIp->GetAddress(tempInterface, 1).GetAddress();
                    }
                    NS_LOG_LOGIC("Adding " << tempAddr << " to Network LSA");
                    pLSA->AddAttachedRouter(tempAddr);
                }
            }
            else
            {
                NS_LOG_LOGIC("Node " << tempNode->GetId() << " device " << tempNd
                                     << " does not have IPv4 interface; skipping");
            }
        }
        m_LSAs.push_back(pLSA);
        NS_LOG_LOGIC("========== LSA for node " << node->GetId() << " ==========");
        NS_LOG_LOGIC(*pLSA);
        pLSA = nullptr;
    }
}

template <typename T>
NetDeviceContainer
GlobalRouter<T>::FindAllNonBridgedDevicesOnLink(Ptr<Channel> ch) const
{
    NS_LOG_FUNCTION(this << ch);
    NetDeviceContainer c;

    for (std::size_t i = 0; i < ch->GetNDevices(); i++)
    {
        Ptr<NetDevice> nd = ch->GetDevice(i);
        NS_LOG_LOGIC("checking to see if the device " << nd << " is bridged");
        Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged(nd);
        if (bnd && !BridgeHasAlreadyBeenVisited(bnd))
        {
            NS_LOG_LOGIC("Device is bridged by BridgeNetDevice "
                         << bnd << " with " << bnd->GetNBridgePorts() << " ports");
            MarkBridgeAsVisited(bnd);
            // Find all channels bridged together, and recursively call
            // on all other channels
            for (uint32_t j = 0; j < bnd->GetNBridgePorts(); j++)
            {
                Ptr<NetDevice> bridgedDevice = bnd->GetBridgePort(j);
                if (bridgedDevice->GetChannel() == ch)
                {
                    NS_LOG_LOGIC("Skipping my own device/channel");
                    continue;
                }
                NS_LOG_LOGIC("Calling on channel " << bridgedDevice->GetChannel());
                c.Add(FindAllNonBridgedDevicesOnLink(bridgedDevice->GetChannel()));
            }
        }
        else
        {
            NS_LOG_LOGIC("Device is not bridged; adding");
            c.Add(nd);
        }
    }
    NS_LOG_LOGIC("Found " << c.GetN() << " devices");
    return c;
}

//
// Given a local net device, we need to walk the channel to which the net device is
// attached and look for nodes with GlobalRouter interfaces on them (one of them
// will be us).  Of these, the router with the lowest IP address on the net device
// connecting to the channel becomes the designated router for the link.
//
template <typename T>
GlobalRouter<T>::IpAddress
GlobalRouter<T>::FindDesignatedRouterForLink(Ptr<NetDevice> ndLocal) const
{
    NS_LOG_FUNCTION(this << ndLocal);

    Ptr<Channel> ch = ndLocal->GetChannel();
    uint32_t nDevices = ch->GetNDevices();
    NS_ASSERT(nDevices);

    NS_LOG_LOGIC("Looking for designated router off of net device " << ndLocal << " on node "
                                                                    << ndLocal->GetNode()->GetId());

    IpAddress designatedRtr;
    if constexpr (IsIpv4)
    {
        designatedRtr = Ipv4Address::GetBroadcast();
    }
    else
    {
        designatedRtr = Ipv6Address::GetOnes();
    }

    //
    // Look through all of the devices on the channel to which the net device
    // in question is attached.
    //
    for (uint32_t i = 0; i < nDevices; i++)
    {
        Ptr<NetDevice> ndOther = ch->GetDevice(i);
        NS_ASSERT(ndOther);

        Ptr<Node> nodeOther = ndOther->GetNode();

        NS_LOG_LOGIC("Examine channel device " << i << " on node " << nodeOther->GetId());

        //
        // For all other net devices, we need to check and see if a router
        // is present.  If the net device on the other side is a bridged
        // device, we need to consider all of the other devices on the
        // bridge as well (all of the bridge ports.
        //
        NS_LOG_LOGIC("checking to see if the device is bridged");
        Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged(ndOther);
        if (bnd)
        {
            NS_LOG_LOGIC("Device is bridged by BridgeNetDevice " << bnd);

            //
            // When enumerating a bridge, don't count the netdevice we came in on
            //
            if (ndLocal == ndOther)
            {
                NS_LOG_LOGIC("Skip -- it is where we came from.");
                continue;
            }

            //
            // It is possible that the bridge net device is sitting under a
            // router, so we have to check for the presence of that router
            // before we run off and follow all the links
            //
            // We require a designated router to have a GlobalRouter interface and
            // an internet stack that includes the Ipv4 interface.  If it doesn't
            // it can't play router.
            //
            NS_LOG_LOGIC("Checking for router on bridge net device " << bnd);
            Ptr<GlobalRouter> rtr = nodeOther->GetObject<GlobalRouter>();
            Ptr<Ip> ip = nodeOther->GetObject<Ip>();
            if (rtr && ip)
            {
                int32_t interfaceOther = ip->GetInterfaceForDevice(bnd);
                if (interfaceOther != -1)
                {
                    NS_LOG_LOGIC("Found router on bridge net device " << bnd);
                    if (!ip->IsUp(interfaceOther))
                    {
                        NS_LOG_LOGIC("Remote side interface " << interfaceOther << " not up");
                        continue;
                    }
                    if (ip->GetNAddresses(interfaceOther) > 1)
                    {
                        NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the "
                                    "primary one");
                    }
                    IpAddress addrOther;
                    if constexpr (IsIpv4)
                    {
                        addrOther = ip->GetAddress(interfaceOther, 0).GetAddress();
                    }
                    else
                    {
                        addrOther = ip->GetAddress(interfaceOther, 1).GetAddress();
                    }
                    designatedRtr = addrOther < designatedRtr ? addrOther : designatedRtr;
                    NS_LOG_LOGIC("designated router now " << designatedRtr);
                }
            }

            //
            // Check if we have seen this bridge net device already while
            // recursively enumerating an L2 broadcast domain. If it is new
            // to us, go ahead and process it. If we have already processed it,
            // move to the next
            //
            if (BridgeHasAlreadyBeenVisited(bnd))
            {
                NS_ABORT_MSG("ERROR: L2 forwarding loop detected!");
            }

            MarkBridgeAsVisited(bnd);

            NS_LOG_LOGIC("Looking through bridge ports of bridge net device " << bnd);
            for (uint32_t j = 0; j < bnd->GetNBridgePorts(); ++j)
            {
                Ptr<NetDevice> ndBridged = bnd->GetBridgePort(j);
                NS_LOG_LOGIC("Examining bridge port " << j << " device " << ndBridged);
                if (ndBridged == ndOther)
                {
                    NS_LOG_LOGIC("That bridge port is me, don't walk backward");
                    continue;
                }

                NS_LOG_LOGIC("Recursively looking for routers down bridge port " << ndBridged);
                IpAddress addrOther = FindDesignatedRouterForLink(ndBridged);
                designatedRtr = addrOther < designatedRtr ? addrOther : designatedRtr;
                NS_LOG_LOGIC("designated router now " << designatedRtr);
            }
        }
        else
        {
            NS_LOG_LOGIC("This device is not bridged");
            Ptr<Node> nodeOther = ndOther->GetNode();
            NS_ASSERT(nodeOther);

            //
            // We require a designated router to have a GlobalRouter interface and
            // an internet stack that includes the Ipv4 interface.  If it doesn't
            //
            Ptr<GlobalRouter> rtr = nodeOther->GetObject<GlobalRouter>();
            Ptr<Ip> ip = nodeOther->GetObject<Ip>();
            if (rtr && ip)
            {
                int32_t interfaceOther = ip->GetInterfaceForDevice(ndOther);
                if (interfaceOther != -1)
                {
                    if (!ip->IsUp(interfaceOther))
                    {
                        NS_LOG_LOGIC("Remote side interface " << interfaceOther << " not up");
                        continue;
                    }
                    NS_LOG_LOGIC("Found router on net device " << ndOther);
                    if (ip->GetNAddresses(interfaceOther) > 1)
                    {
                        NS_LOG_WARN("Warning, interface has multiple IP addresses; using only the "
                                    "primary one");
                    }
                    IpAddress addrOther;
                    if constexpr (IsIpv4)
                    {
                        addrOther = ip->GetAddress(interfaceOther, 0).GetAddress();
                    }
                    else
                    {
                        addrOther = ip->GetAddress(interfaceOther, 1).GetAddress();
                    }
                    designatedRtr = addrOther < designatedRtr ? addrOther : designatedRtr;
                    NS_LOG_LOGIC("designated router now " << designatedRtr);
                }
            }
        }
    }
    return designatedRtr;
}

//
// Given a node and an attached net device, take a look off in the channel to
// which the net device is attached and look for a node on the other side
// that has a GlobalRouter interface aggregated.  Life gets more complicated
// when there is a bridged net device on the other side.
//
template <typename T>
bool
GlobalRouter<T>::AnotherRouterOnLink(Ptr<NetDevice> nd) const
{
    NS_LOG_FUNCTION(this << nd);

    Ptr<Channel> ch = nd->GetChannel();
    if (!ch)
    {
        // It may be that this net device is a stub device, without a channel
        return false;
    }
    uint32_t nDevices = ch->GetNDevices();
    NS_ASSERT(nDevices);

    NS_LOG_LOGIC("Looking for routers off of net device " << nd << " on node "
                                                          << nd->GetNode()->GetId());

    //
    // Look through all of the devices on the channel to which the net device
    // in question is attached.
    //
    for (uint32_t i = 0; i < nDevices; i++)
    {
        Ptr<NetDevice> ndOther = ch->GetDevice(i);
        NS_ASSERT(ndOther);

        NS_LOG_LOGIC("Examine channel device " << i << " on node " << ndOther->GetNode()->GetId());

        //
        // Ignore the net device itself.
        //
        if (ndOther == nd)
        {
            NS_LOG_LOGIC("Myself, skip");
            continue;
        }

        //
        // For all other net devices, we need to check and see if a router
        // is present.  If the net device on the other side is a bridged
        // device, we need to consider all of the other devices on the
        // bridge.
        //
        NS_LOG_LOGIC("checking to see if device is bridged");
        Ptr<BridgeNetDevice> bnd = NetDeviceIsBridged(ndOther);
        if (bnd)
        {
            NS_LOG_LOGIC("Device is bridged by net device " << bnd);

            //
            // Check if we have seen this bridge net device already while
            // recursively enumerating an L2 broadcast domain. If it is new
            // to us, go ahead and process it. If we have already processed it,
            // move to the next
            //
            if (BridgeHasAlreadyBeenVisited(bnd))
            {
                NS_ABORT_MSG("ERROR: L2 forwarding loop detected!");
            }

            MarkBridgeAsVisited(bnd);

            NS_LOG_LOGIC("Looking through bridge ports of bridge net device " << bnd);
            for (uint32_t j = 0; j < bnd->GetNBridgePorts(); ++j)
            {
                Ptr<NetDevice> ndBridged = bnd->GetBridgePort(j);
                NS_LOG_LOGIC("Examining bridge port " << j << " device " << ndBridged);
                if (ndBridged == ndOther)
                {
                    NS_LOG_LOGIC("That bridge port is me, skip");
                    continue;
                }

                NS_LOG_LOGIC("Recursively looking for routers on bridge port " << ndBridged);
                if (AnotherRouterOnLink(ndBridged))
                {
                    NS_LOG_LOGIC("Found routers on bridge port, return true");
                    return true;
                }
            }
            NS_LOG_LOGIC("No routers on bridged net device, return false");
            return false;
        }

        NS_LOG_LOGIC("This device is not bridged");
        Ptr<Node> nodeTemp = ndOther->GetNode();
        NS_ASSERT(nodeTemp);

        Ptr<GlobalRouter> rtr = nodeTemp->GetObject<GlobalRouter>();
        if (rtr)
        {
            NS_LOG_LOGIC("Found GlobalRouter interface, return true");
            return true;
        }
        else
        {
            NS_LOG_LOGIC("No GlobalRouter interface on device, continue search");
        }
    }
    NS_LOG_LOGIC("No routers found, return false");
    return false;
}

template <typename T>
uint32_t
GlobalRouter<T>::GetNumLSAs() const
{
    NS_LOG_FUNCTION(this);
    return m_LSAs.size();
}

//
// Get the nth link state advertisement from this router.
//
template <typename T>
bool
GlobalRouter<T>::GetLSA(uint32_t n, GlobalRoutingLSA<T>& lsa) const
{
    NS_LOG_FUNCTION(this << n << &lsa);
    NS_ASSERT_MSG(lsa.IsEmpty(), "GlobalRouter::GetLSA (): Must pass empty LSA");
    //
    // All of the work was done in GetNumLSAs.  All we have to do here is to
    // walk the list of link state advertisements created there and return the
    // one the client is interested in.
    //
    auto i = m_LSAs.begin();
    uint32_t j = 0;

    for (; i != m_LSAs.end(); i++, j++)
    {
        if (j == n)
        {
            GlobalRoutingLSA<T>* p = *i;
            lsa = *p;
            return true;
        }
    }

    return false;
}

template <typename T>
void
GlobalRouter<T>::InjectRoute(IpAddress network, IpMaskOrPrefix networkMask)
{
    NS_LOG_FUNCTION(this << network << networkMask);
    auto route = new IpRoutingTableEntry();
    //
    // Interface number does not matter here, using 1.
    //
    *route = IpRoutingTableEntry::CreateNetworkRouteTo(network, networkMask, 1);
    m_injectedRoutes.push_back(route);
}

template <typename T>
GlobalRouter<T>::IpRoutingTableEntry*
GlobalRouter<T>::GetInjectedRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    if (index < m_injectedRoutes.size())
    {
        uint32_t tmp = 0;
        for (auto i = m_injectedRoutes.begin(); i != m_injectedRoutes.end(); i++)
        {
            if (tmp == index)
            {
                return *i;
            }
            tmp++;
        }
    }
    NS_ASSERT(false);
    // quiet compiler.
    return nullptr;
}

template <typename T>
uint32_t
GlobalRouter<T>::GetNInjectedRoutes()
{
    NS_LOG_FUNCTION(this);
    return m_injectedRoutes.size();
}

template <typename T>
void
GlobalRouter<T>::RemoveInjectedRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    NS_ASSERT(index < m_injectedRoutes.size());
    uint32_t tmp = 0;
    for (auto i = m_injectedRoutes.begin(); i != m_injectedRoutes.end(); i++)
    {
        if (tmp == index)
        {
            NS_LOG_LOGIC("Removing route " << index << "; size = " << m_injectedRoutes.size());
            delete *i;
            m_injectedRoutes.erase(i);
            return;
        }
        tmp++;
    }
}

template <typename T>
bool
GlobalRouter<T>::WithdrawRoute(IpAddress network, IpMaskOrPrefix networkMask)
{
    NS_LOG_FUNCTION(this << network << networkMask);
    for (auto i = m_injectedRoutes.begin(); i != m_injectedRoutes.end(); i++)
    {
        if constexpr (IsIpv4)
        {
            if ((*i)->GetDestNetwork() == network && (*i)->GetDestNetworkMask() == networkMask)
            {
                NS_LOG_LOGIC("Withdrawing route to network/mask " << network << "/" << networkMask);
                delete *i;
                m_injectedRoutes.erase(i);
                return true;
            }
        }
        else
        {
            if ((*i)->GetDestNetwork() == network && (*i)->GetDestNetworkPrefix() == networkMask)
            {
                NS_LOG_LOGIC("Withdrawing route to network/mask " << network << "/" << networkMask);
                delete *i;
                m_injectedRoutes.erase(i);
                return true;
            }
        }
    }
    return false;
}

//
// Link through the given channel and find the net device that's on the
// other end.  This only makes sense with a point-to-point channel.
//
template <typename T>
Ptr<NetDevice>
GlobalRouter<T>::GetAdjacent(Ptr<NetDevice> nd, Ptr<Channel> ch) const
{
    NS_LOG_FUNCTION(this << nd << ch);
    NS_ASSERT_MSG(ch->GetNDevices() == 2,
                  "GlobalRouter::GetAdjacent (): Channel with other than two devices");
    //
    // This is a point to point channel with two endpoints.  Get both of them.
    //
    Ptr<NetDevice> nd1 = ch->GetDevice(0);
    Ptr<NetDevice> nd2 = ch->GetDevice(1);
    //
    // One of the endpoints is going to be "us" -- that is the net device attached
    // to the node on which we're running -- i.e., "nd".  The other endpoint (the
    // one to which we are connected via the channel) is the adjacent router.
    //
    if (nd1 == nd)
    {
        return nd2;
    }
    else if (nd2 == nd)
    {
        return nd1;
    }
    else
    {
        NS_ASSERT_MSG(false, "GlobalRouter::GetAdjacent (): Wrong or confused channel?");
        return nullptr;
    }
}

//
// Decide whether or not a given net device is being bridged by a BridgeNetDevice.
//
template <typename T>
Ptr<BridgeNetDevice>
GlobalRouter<T>::NetDeviceIsBridged(Ptr<NetDevice> nd) const
{
    NS_LOG_FUNCTION(this << nd);

    Ptr<Node> node = nd->GetNode();
    uint32_t nDevices = node->GetNDevices();

    //
    // There is no bit on a net device that says it is being bridged, so we have
    // to look for bridges on the node to which the device is attached.  If we
    // find a bridge, we need to look through its bridge ports (the devices it
    // bridges) to see if we find the device in question.
    //
    for (uint32_t i = 0; i < nDevices; ++i)
    {
        Ptr<NetDevice> ndTest = node->GetDevice(i);
        NS_LOG_LOGIC("Examine device " << i << " " << ndTest);

        if (ndTest->IsBridge())
        {
            NS_LOG_LOGIC("device " << i << " is a bridge net device");
            Ptr<BridgeNetDevice> bnd = ndTest->GetObject<BridgeNetDevice>();
            NS_ABORT_MSG_UNLESS(
                bnd,
                "GlobalRouter::DiscoverLSAs (): GetObject for <BridgeNetDevice> failed");

            for (uint32_t j = 0; j < bnd->GetNBridgePorts(); ++j)
            {
                NS_LOG_LOGIC("Examine bridge port " << j << " " << bnd->GetBridgePort(j));
                if (bnd->GetBridgePort(j) == nd)
                {
                    NS_LOG_LOGIC("Net device " << nd << " is bridged by " << bnd);
                    return bnd;
                }
            }
        }
    }
    NS_LOG_LOGIC("Net device " << nd << " is not bridged");
    return nullptr;
}

//
// Start a new enumeration of an L2 broadcast domain by clearing m_bridgesVisited
//
template <typename T>
void
GlobalRouter<T>::ClearBridgesVisited() const
{
    m_bridgesVisited.clear();
}

//
// Check if we have already visited a given bridge net device by searching m_bridgesVisited
//
template <typename T>
bool
GlobalRouter<T>::BridgeHasAlreadyBeenVisited(Ptr<BridgeNetDevice> bridgeNetDevice) const
{
    for (auto iter = m_bridgesVisited.begin(); iter != m_bridgesVisited.end(); ++iter)
    {
        if (bridgeNetDevice == *iter)
        {
            NS_LOG_LOGIC("Bridge " << bridgeNetDevice << " has been visited.");
            return true;
        }
    }
    return false;
}

//
// Remember that we visited a bridge net device by adding it to m_bridgesVisited
//
template <typename T>
void
GlobalRouter<T>::MarkBridgeAsVisited(Ptr<BridgeNetDevice> bridgeNetDevice) const
{
    NS_LOG_FUNCTION(this << bridgeNetDevice);
    m_bridgesVisited.push_back(bridgeNetDevice);
}

/**Explicit initialize the template classes */
/** @brief Stream insertion operator
 *  @returns the reference to the output stream
 */
template std::ostream& operator<< <Ipv4Manager>(std::ostream&, GlobalRoutingLSA<Ipv4Manager>&);
template class GlobalRoutingLinkRecord<Ipv4Manager>;
template class GlobalRoutingLSA<Ipv4Manager>;
/** @brief Stream insertion operator
 *  @returns the reference to the output stream
 */
template std::ostream& operator<< <Ipv6Manager>(std::ostream&, GlobalRoutingLSA<Ipv6Manager>&);
template class GlobalRoutingLinkRecord<Ipv6Manager>;
template class GlobalRoutingLSA<Ipv6Manager>;
NS_OBJECT_TEMPLATE_CLASS_DEFINE(GlobalRouter, Ipv4Manager);
NS_OBJECT_TEMPLATE_CLASS_DEFINE(GlobalRouter, Ipv6Manager);

} // namespace ns3
