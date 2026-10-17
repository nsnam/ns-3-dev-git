/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Craig Dowell (craigdo@ee.washington.edu)
 *           Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef GLOBAL_ROUTER_INTERFACE_H
#define GLOBAL_ROUTER_INTERFACE_H

#include "global-route-manager.h"
#include "ipv4-routing-protocol.h"
#include "ipv4-routing-table-entry.h"
#include "ipv6-l3-protocol.h"
#include "ipv6-routing-protocol.h"
#include "ipv6-routing-table-entry.h"
#include "ipv6.h"

#include "ns3/bridge-net-device.h"
#include "ns3/channel.h"
#include "ns3/ipv4-address.h"
#include "ns3/net-device-container.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

#include <list>
#include <stdint.h>

namespace ns3
{
template <typename T>
class GlobalRouter;
template <typename>
class GlobalRouting;

/**
 * @ingroup globalrouting
 *
 * @brief A single link record for a link state advertisement.
 *
 * The GlobalRoutingLinkRecord is modeled after the OSPF link record field of
 * a Link State Advertisement.  Right now we will only see two types of link
 * records corresponding to a stub network and a point-to-point link (channel).
 */
template <typename T>
class GlobalRoutingLinkRecord
{
    static_assert(
        std::is_same_v<T, Ipv4Manager> || std::is_same_v<T, Ipv6Manager>,
        "T must be either Ipv4Manager or Ipv6Manager when calling GlobalRoutingLinkRecord");
    /// Alias for determining whether the parent is Ipv4RoutingProtocol or Ipv6RoutingProtocol
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Manager, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;

    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;

    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;

    /// Alias for Ipv4Header and Ipv6Header classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;

    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

    /// Alias for Ipv4RoutingTableEntry and Ipv6RoutingTableEntry classes
    using IpRoutingTableEntry =
        typename std::conditional_t<IsIpv4, Ipv4RoutingTableEntry, Ipv6RoutingTableEntry>;

    /// Alias for Ipv4Mask And Ipv6Prefix
    using IpMaskOrPrefix = typename std::conditional_t<IsIpv4, Ipv4Mask, Ipv6Prefix>;

  public:
    template <typename>
    friend class GlobalRoutingLSA; //!< Friend class.

    /**
     * @enum LinkType
     * @brief Enumeration of the possible types of Global Routing Link Records.
     *
     * These values are defined in the OSPF spec.  We currently only use
     * PointToPoint and StubNetwork types.
     */
    enum LinkType
    {
        Unknown = 0,    /**< Uninitialized Link Record */
        PointToPoint,   /**< Record representing a point to point channel */
        TransitNetwork, /**< Unused -- for future OSPF compatibility  */
        StubNetwork,    /**< Record represents a leaf node network */
        VirtualLink     /**< Unused -- for future OSPF compatibility  */
    };

    /**
     * @brief Construct an empty ("uninitialized") Global Routing Link Record.
     *
     * The Link ID and Link Data Ipv4 addresses are set to "0.0.0.0";
     * The Link Type is set to Unknown;
     * The metric is set to 0.
     */
    GlobalRoutingLinkRecord();

    /**
     * Construct an initialized Global Routing Link Record.
     *
     * @param linkType The type of link record to construct.
     * @param linkId The link ID for the record.
     * @param linkData The link data field for the record.
     * @param metric The metric field for the record.
     * @see LinkType
     * @see SetLinkId
     * @see SetLinkData
     */
    GlobalRoutingLinkRecord(LinkType linkType,
                            IpAddress linkId,
                            IpAddress linkData,
                            uint16_t metric);

    /**
     * Construct an initialized Global Routing Link Record.
     *
     * @param linkType The type of link record to construct.
     * @param linkId The link ID for the record.
     * @param linkData The link data field for the record.
     * @param linkLocData The link local data field for the record.
     * @param metric The metric field for the record.
     * @see LinkType
     * @see SetLinkId
     * @see SetLinkData
     */
    GlobalRoutingLinkRecord(LinkType linkType,
                            IpAddress linkId,
                            IpAddress linkData,
                            IpAddress linkLocData,
                            uint16_t metric);

    /**
     * @brief Destroy a Global Routing Link Record.
     *
     * Currently does nothing.  Here as a placeholder only.
     */
    ~GlobalRoutingLinkRecord();

    /**
     * Get the Link ID field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link ID will be the Router ID
     * of the neighboring router.
     *
     * For an OSPF type 3 link (StubNetwork), the Link ID will be the adjacent
     * neighbor's IP address
     *
     * @returns The Ipv4Address corresponding to the Link ID field of the record.
     */
    IpAddress GetLinkId() const;

    /**
     * @brief Set the Link ID field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link ID must be the Router ID
     * of the neighboring router.
     *
     * For an OSPF type 3 link (StubNetwork), the Link ID must be the adjacent
     * neighbor's IP address
     *
     * @param addr An Ipv4Address to store in the Link ID field of the record.
     */
    void SetLinkId(IpAddress addr);

    /**
     * @brief Get the Link Data field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link Data will be the IP
     * address of the node of the local side of the link.
     *
     * For an OSPF type 3 link (StubNetwork), the Link Data will be the
     * network mask
     *
     * @returns The Ipv4Address corresponding to the Link Data field of the record.
     */
    IpAddress GetLinkData() const;

    /**
     * @brief Set the Link Data field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link Data must be the IP
     * address of the node of the local side of the link.
     *
     * For an OSPF type 3 link (StubNetwork), the Link Data must be set to the
     * network mask
     *
     * @param addr An Ipv4Address to store in the Link Data field of the record.
     */
    void SetLinkData(IpAddress addr);

    /**
     * @brief Get the Link Type field of the Global Routing Link Record.
     *
     * The Link Type describes the kind of link a given record represents.  The
     * values are defined by OSPF.
     *
     * @see LinkType
     * @returns The LinkType of the current Global Routing Link Record.
     */
    LinkType GetLinkType() const;

    /**
     * @brief Set the Link Type field of the Global Routing Link Record.
     *
     * The Link Type describes the kind of link a given record represents.  The
     * values are defined by OSPF.
     *
     * @see LinkType
     * @param linkType The new LinkType for the current Global Routing Link Record.
     */
    void SetLinkType(LinkType linkType);

    /**
     * @brief Get the Metric Data field of the Global Routing Link Record.
     *
     * The metric is an abstract cost associated with forwarding a packet across
     * a link.  A sum of metrics must have a well-defined meaning.  That is, you
     * shouldn't use bandwidth as a metric (how does the sum of the bandwidth of
     * two hops relate to the cost of sending a packet); rather you should use
     * something like delay.
     *
     * @returns The metric field of the Global Routing Link Record.
     */
    uint16_t GetMetric() const;

    /**
     * @brief Set the Metric Data field of the Global Routing Link Record.
     *
     * The metric is an abstract cost associated with forwarding a packet across
     * a link.  A sum of metrics must have a well-defined meaning.  That is, you
     * shouldn't use bandwidth as a metric (how does the sum of the bandwidth of
     * two hops relate to the cost of sending a packet); rather you should use
     * something like delay.
     *
     * @param metric The new metric for the current Global Routing Link Record.
     */
    void SetMetric(uint16_t metric);

    /**
     * @brief Set the Link Local Data field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link Local Data must be the Link Local IP
     * address of the node of the local side of the link.
     *
     * For an OSPF type 3 link (StubNetwork), the Link Local data field is not used.
     *
     * @param addr An Ipv6Address to store in the Link Data field of the record.
     */
    void SetLinkLocData(IpAddress addr);

    /**
     * @brief Set the Link Local Data field of the Global Routing Link Record.
     *
     * For an OSPF type 1 link (PointToPoint) the Link Local Data must be the Link Local IP
     * address of the node of the local side of the link.
     *
     * For an OSPF type 3 link (StubNetwork), the Link Local data field is not used.
     *
     * @returns addr An Ipv6Address to store in the Link Data field of the record.
     */
    IpAddress GetLinkLocData();

  private:
    /**
     * m_linkId and m_linkData are defined by OSPF to have different meanings
     * depending on the type of link a given link records represents.  They work
     * together.
     *
     * For Type 1 link (PointToPoint), set m_linkId to Router ID of
     * neighboring router.
     *
     * For Type 3 link (Stub), set m_linkId to neighbor's IP address
     */
    IpAddress m_linkId;

    /**
     * m_linkId and m_linkData are defined by OSPF to have different meanings
     * depending on the type of link a given link records represents.  They work
     * together.
     *
     * For Type 1 link (PointToPoint), set m_linkData to local IP address
     *
     * For Type 3 link (Stub), set m_linkData to mask
     */
    IpAddress m_linkData; // for links to RouterLSA,

    /**
     * m_linkLocData is not defined by the OSPF spec. This field is used to store the link local
     * address associated with the link on the local side.
     *
     * For Type 1 link (PointToPoint), set m_linkLocData to Link local IP address on the local side
     *
     * For Type 3 link (Stub), this is not used.
     */
    IpAddress
        m_linkLocData; // for the Link local address associated with the link on the local side

    /**
     * The type of the Global Routing Link Record.  Defined in the OSPF spec.
     * We currently only use PointToPoint and StubNetwork types.
     */
    LinkType m_linkType;

    /**
     * The metric for a given link.
     *
     * A metric is abstract cost associated with forwarding a packet across a
     * link.  A sum of metrics must have a well-defined meaning.  That is, you
     * shouldn't use bandwidth as a metric (how does the sum of the bandwidth
     * of two hops relate to the cost of sending a packet); rather you should
     * use something like delay.
     */
    uint16_t m_metric;
};

/**
 * @brief a Link State Advertisement (LSA) for a router, used in global
 * routing.
 *
 * Roughly equivalent to a global incarnation of the OSPF link state header
 * combined with a list of Link Records.  Since it's global, there's
 * no need for age or sequence number.  See \RFC{2328}, Appendix A.
 */
template <typename T>
class GlobalRoutingLSA
{
    static_assert(std::is_same_v<T, Ipv4Manager> || std::is_same_v<T, Ipv6Manager>,
                  "T must be either Ipv4Manager or Ipv6Manager when calling GlobalRoutingLSA");
    /// Alias for determining whether the parent is Ipv4RoutingProtocol or Ipv6RoutingProtocol
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Manager, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;

    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;

    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;

    /// Alias for Ipv4Header and Ipv6Header classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;

    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

    /// Alias for Ipv4RoutingTableEntry and Ipv6RoutingTableEntry classes
    using IpRoutingTableEntry =
        typename std::conditional_t<IsIpv4, Ipv4RoutingTableEntry, Ipv6RoutingTableEntry>;

    /// Alias for Ipv4Mask And Ipv6Prefix
    using IpMaskOrPrefix = typename std::conditional_t<IsIpv4, Ipv4Mask, Ipv6Prefix>;

  public:
    /**
     * @enum LSType
     * @brief corresponds to LS type field of \RFC{2328} OSPF LSA header
     */
    enum LSType
    {
        Unknown = 0, /**< Uninitialized Type */
        RouterLSA,
        NetworkLSA,
        SummaryLSA,
        SummaryLSA_ASBR,
        ASExternalLSAs
    };

    /**
     * @enum SPFStatus
     * @brief Enumeration of the possible values of the status flag in the Routing
     * Link State Advertisements.
     */
    enum SPFStatus
    {
        LSA_SPF_NOT_EXPLORED = 0, /**< New vertex not yet considered */
        LSA_SPF_CANDIDATE,        /**< Vertex is in the SPF candidate queue */
        LSA_SPF_IN_SPFTREE        /**< Vertex is in the SPF tree */
    };

    /**
     * @brief Create a blank Global Routing Link State Advertisement.
     *
     * On completion Ipv4Address variables initialized to 0.0.0.0 and the
     * list of Link State Records is empty.
     */
    GlobalRoutingLSA();

    /**
     * @brief Create an initialized Global Routing Link State Advertisement.
     *
     * On completion the list of Link State Records is empty.
     *
     * @param status The status to of the new LSA.
     * @param linkStateId The Ipv4Address for the link state ID field.
     * @param advertisingRtr The Ipv4Address for the advertising router field.
     */
    GlobalRoutingLSA(SPFStatus status, IpAddress linkStateId, IpAddress advertisingRtr);

    /**
     * @brief Copy constructor for a Global Routing Link State Advertisement.
     *
     * Takes a piece of memory and constructs a semantically identical copy of
     * the given LSA.
     *
     * @param lsa The existing LSA to be used as the source.
     */
    GlobalRoutingLSA(GlobalRoutingLSA& lsa);

    /**
     * @brief Destroy an existing Global Routing Link State Advertisement.
     *
     * Any Global Routing Link Records present in the list are freed.
     */
    ~GlobalRoutingLSA();

    /**
     * @brief Assignment operator for a Global Routing Link State Advertisement.
     *
     * Takes an existing Global Routing Link State Advertisement and overwrites
     * it to make a semantically identical copy of a given prototype LSA.
     *
     * If there are any Global Routing Link Records present in the existing
     * LSA, they are freed before the assignment happens.
     *
     * @param lsa The existing LSA to be used as the source.
     * @returns Reference to the overwritten LSA.
     */
    GlobalRoutingLSA& operator=(const GlobalRoutingLSA& lsa);

    /**
     * @brief Copy any Global Routing Link Records in a given Global Routing Link
     * State Advertisement to the current LSA.
     *
     * Existing Link Records are not deleted -- this is a concatenation of Link
     * Records.
     *
     * @see ClearLinkRecords ()
     * @param lsa The LSA to copy the Link Records from.
     */
    void CopyLinkRecords(const GlobalRoutingLSA& lsa);

    /**
     * @brief Add a given Global Routing Link Record to the LSA.
     *
     * @param lr The Global Routing Link Record to be added.
     * @returns The number of link records in the list.
     */
    uint32_t AddLinkRecord(GlobalRoutingLinkRecord<T>* lr);

    /**
     * @brief Return the number of Global Routing Link Records in the LSA.
     *
     * @returns The number of link records in the list.
     */
    uint32_t GetNLinkRecords() const;

    /**
     * @brief Return a pointer to the specified Global Routing Link Record.
     *
     * @param n The LSA number desired.
     * @returns The number of link records in the list.
     */
    GlobalRoutingLinkRecord<T>* GetLinkRecord(uint32_t n) const;

    /**
     * @brief Release all of the Global Routing Link Records present in the Global
     * Routing Link State Advertisement and make the list of link records empty.
     */
    void ClearLinkRecords();

    /**
     * @brief Check to see if the list of Global Routing Link Records present in the
     * Global Routing Link State Advertisement is empty.
     *
     * @returns True if the list is empty, false otherwise.
     */
    bool IsEmpty() const;

    /**
     * @brief Print the contents of the Global Routing Link State Advertisement and
     * any Global Routing Link Records present in the list.  Quite verbose.
     * @param os the output stream
     */
    void Print(std::ostream& os) const;

    /**
     * @brief Return the LSType field of the LSA
     * @returns The LS Type.
     */
    LSType GetLSType() const;
    /**
     * @brief Set the LS type field of the LSA
     * @param typ the LS Type.
     */
    void SetLSType(LSType typ);

    /**
     * @brief Get the Link State ID as defined by the OSPF spec.  We always set it
     * to the router ID of the router making the advertisement.
     *
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     * @returns The Ipv4Address stored as the link state ID.
     */
    IpAddress GetLinkStateId() const;

    /**
     * @brief Set the Link State ID is defined by the OSPF spec.  We always set it
     * to the router ID of the router making the advertisement.
     * @param addr IPv4 address which will act as ID
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     */
    void SetLinkStateId(IpAddress addr);

    /**
     * @brief Get the Advertising Router as defined by the OSPF spec.  We always
     * set it to the router ID of the router making the advertisement.
     *
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     * @returns The Ipv4Address stored as the advertising router.
     */
    IpAddress GetAdvertisingRouter() const;

    /**
     * @brief Set the Advertising Router as defined by the OSPF spec.  We always
     * set it to the router ID of the router making the advertisement.
     *
     * @param rtr ID of the router making advertisement
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     */
    void SetAdvertisingRouter(IpAddress rtr);

    /**
     * @brief For a Network LSA, set the Network Mask field that precedes
     * the list of attached routers.
     * @param mask the Network Mask field.
     */
    void SetNetworkLSANetworkMask(IpMaskOrPrefix mask);

    /**
     * @brief For a Network LSA, get the Network Mask field that precedes
     * the list of attached routers.
     *
     * @returns the NetworkLSANetworkMask
     */
    IpMaskOrPrefix GetNetworkLSANetworkMask() const;

    /**
     * @brief Add an attached router to the list in the NetworkLSA
     *
     * @param addr The Ipv4Address of the interface on the network link
     * @returns The number of addresses in the list.
     */
    uint32_t AddAttachedRouter(IpAddress addr);

    /**
     * @brief Return the number of attached routers listed in the NetworkLSA
     *
     * @returns The number of attached routers.
     */
    uint32_t GetNAttachedRouters() const;

    /**
     * @brief Return an Ipv4Address corresponding to the specified attached router
     *
     * @param n The attached router number desired (number in the list).
     * @returns The Ipv4Address of the requested router
     */
    IpAddress GetAttachedRouter(uint32_t n) const;

    /**
     * @brief Get the SPF status of the advertisement.
     *
     * @see SPFStatus
     * @returns The SPFStatus of the LSA.
     */
    SPFStatus GetStatus() const;

    /**
     * @brief Set the SPF status of the advertisement
     * @param status SPF status to set
     * @see SPFStatus
     */
    void SetStatus(SPFStatus status);

    /**
     * @brief Get the Node pointer of the node that originated this LSA
     * @returns Node pointer
     */
    Ptr<Node> GetNode() const;

    /**
     * @brief Set the Node pointer of the node that originated this LSA
     * @param node Node pointer
     */
    void SetNode(Ptr<Node> node);

  private:
    /**
     * The type of the LSA.  Each LSA type has a separate advertisement
     * format.
     */
    LSType m_lsType;
    /**
     * The Link State ID is defined by the OSPF spec.  We always set it to the
     * router ID of the router making the advertisement.
     *
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     */
    IpAddress m_linkStateId;

    /**
     * The Advertising Router is defined by the OSPF spec.  We always set it to
     * the router ID of the router making the advertisement.
     *
     * @see RoutingEnvironment::AllocateRouterId ()
     * @see GlobalRouting::GetRouterId ()
     */
    IpAddress m_advertisingRtr;

    /**
     * A convenience typedef to avoid too much writers cramp.
     */
    typedef std::list<GlobalRoutingLinkRecord<T>*> ListOfLinkRecords_t;

    /**
     * Each Link State Advertisement contains a number of Link Records that
     * describe the kinds of links that are attached to a given node.  We
     * consider PointToPoint and StubNetwork links.
     *
     * m_linkRecords is an STL list container to hold the Link Records that have
     * been discovered and prepared for the advertisement.
     *
     * @see GlobalRouting::DiscoverLSAs ()
     */
    ListOfLinkRecords_t m_linkRecords;

    /**
     * Each Network LSA contains the network mask of the attached network
     */
    IpMaskOrPrefix m_networkLSANetworkMask;

    /**
     * A convenience typedef to avoid too much writers cramp.
     */
    typedef std::list<IpAddress> ListOfAttachedRouters_t;

    /**
     * Each Network LSA contains a list of attached routers
     *
     * m_attachedRouters is an STL list container to hold the addresses that have
     * been discovered and prepared for the advertisement.
     *
     * @see GlobalRouting::DiscoverLSAs ()
     */
    ListOfAttachedRouters_t m_attachedRouters;

    /**
     * This is a tristate flag used internally in the SPF computation to mark
     * if an SPFVertex (a data structure representing a vertex in the SPF tree
     * -- a router) is new, is a candidate for a shortest path, or is in its
     * proper position in the tree.
     */
    SPFStatus m_status;
    uint32_t m_node_id; //!< node ID
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the reference to the output stream
 * @param lsa the LSA
 * @returns the reference to the output stream
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, GlobalRoutingLSA<T>& lsa);

/**
 * @brief An interface aggregated to a node to provide global routing info
 *
 * An interface aggregated to a node that provides global routing information
 * to a global route manager.  The presence of the interface indicates that
 * the node is a router.  The interface is the mechanism by which the router
 * advertises its connections to neighboring routers.  We're basically
 * allowing the route manager to query for link state advertisements.
 */
template <typename T>
class GlobalRouter : public Object
{
    static_assert(std::is_same_v<T, Ipv4Manager> || std::is_same_v<T, Ipv6Manager>,
                  "T must be either Ipv4Manager or Ipv6Manager when calling GlobalRouter");

    /// Alias for determining whether the parent is Ipv4RoutingProtocol or Ipv6RoutingProtocol
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4Manager, T>;

    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;

    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;

    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;

    /// Alias for Ipv4Header and Ipv6Header classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;

    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

    /// Alias for Ipv4RoutingTableEntry and Ipv6RoutingTableEntry classes
    using IpRoutingTableEntry =
        typename std::conditional_t<IsIpv4, Ipv4RoutingTableEntry, Ipv6RoutingTableEntry>;

    /// Alias for Ipv4Mask And Ipv6Prefix
    using IpMaskOrPrefix = typename std::conditional_t<IsIpv4, Ipv4Mask, Ipv6Prefix>;

    /// Alias for Ipv4RoutingProtocol and Ipv6RoutingProtocol classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;

  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Create a Global Router class
     */
    GlobalRouter();

    // Delete copy constructor and assignment operator to avoid misuse
    GlobalRouter(const GlobalRouter&) = delete;
    GlobalRouter& operator=(const GlobalRouter&) = delete;

    /**
     * @brief Set the specific Global Routing Protocol to be used
     * @param routing the routing protocol
     */
    void SetRoutingProtocol(Ptr<GlobalRouting<IpRoutingProtocol>> routing);

    /**
     * @brief Get the specific Global Routing Protocol used
     * @returns the routing protocol
     */
    Ptr<GlobalRouting<IpRoutingProtocol>> GetRoutingProtocol();

    /**
     * @brief Get the Router ID associated with this Global Router.
     *
     * The Router IDs are allocated in the RoutingEnvironment -- one per Router,
     * starting at 0.0.0.1 and incrementing with each instantiation of a router.
     *
     * @see RoutingEnvironment::AllocateRouterId ()
     * @returns The Router ID associated with the Global Router.
     */
    IpAddress GetRouterId() const;

    /**
     * @brief Walk the connected channels, discover the adjacent routers and build
     * the associated number of Global Routing Link State Advertisements that
     * this router can export.
     *
     * This is a fairly expensive operation in that every time it is called
     * the current list of LSAs is built by walking connected point-to-point
     * channels and peeking into adjacent IPV4 stacks to get address information.
     * This is done to allow for limited dynamics of the Global Routing
     * environment.  By that we mean that you can discover new link state
     * advertisements after a network topology change by calling DiscoverLSAs
     * and then by reading those advertisements.
     *
     * @see GlobalRoutingLSA
     * @see GlobalRouter::GetLSA ()
     * @returns The number of Global Routing Link State Advertisements.
     */
    uint32_t DiscoverLSAs();

    /**
     * @brief Get the Number of Global Routing Link State Advertisements that this
     * router can export.
     *
     * To get meaningful information you must have previously called DiscoverLSAs.
     * After you know how many LSAs are present in the router, you may call
     * GetLSA () to retrieve the actual advertisement.
     *
     * @see GlobalRouterLSA
     * @see GlobalRouting::DiscoverLSAs ()
     * @see GlobalRouting::GetLSA ()
     * @returns The number of Global Routing Link State Advertisements.
     */
    uint32_t GetNumLSAs() const;

    /**
     * @brief Get a Global Routing Link State Advertisements that this router has
     * said that it can export.
     *
     * This is a fairly inexpensive expensive operation in that the hard work
     * was done in GetNumLSAs.  We just copy the indicated Global Routing Link
     * State Advertisement into the requested GlobalRoutingLSA object.
     *
     * You must call GlobalRouter::GetNumLSAs before calling this method in
     * order to discover the adjacent routers and build the advertisements.
     * GetNumLSAs will return the number of LSAs this router advertises.
     * The parameter n (requested LSA number) must be in the range 0 to
     * GetNumLSAs() - 1.
     *
     * @see GlobalRoutingLSA
     * @see GlobalRouting::GetNumLSAs ()
     * @param n The index number of the LSA you want to read.
     * @param lsa The GlobalRoutingLSA class to receive the LSA information.
     * @returns The number of Global Router Link State Advertisements.
     */
    bool GetLSA(uint32_t n, GlobalRoutingLSA<T>& lsa) const;

    /**
     * @brief Inject a route to be circulated to other routers as an external
     * route
     *
     * @param network The Network to inject
     * @param networkMask The Network Mask to inject
     */
    void InjectRoute(IpAddress network, IpMaskOrPrefix networkMask);

    /**
     * @brief Get the number of injected routes that have been added
     * to the routing table.
     * @return number of injected routes
     */
    uint32_t GetNInjectedRoutes();

    /**
     * @brief Return the injected route indexed by i
     * @param i the index of the route
     * @return a pointer to that Ipv4RoutingTableEntry is returned
     *
     */
    IpRoutingTableEntry* GetInjectedRoute(uint32_t i);

    /**
     * @brief Withdraw a route from the global unicast routing table.
     *
     * Calling this function will cause all indexed routes numbered above
     * index i to have their index decremented.  For instance, it is possible to
     * remove N injected routes by calling RemoveInjectedRoute (0) N times.
     *
     * @param i The index (into the injected routing list) of the route to remove.
     *
     * @see GlobalRouter::WithdrawRoute ()
     */
    void RemoveInjectedRoute(uint32_t i);

    /**
     * @brief Withdraw a route from the global unicast routing table.
     *
     * @param network The Network to withdraw
     * @param networkMask The Network Mask to withdraw
     * @return whether the operation succeeded (will return false if no such route)
     *
     * @see GlobalRouter::RemoveInjectedRoute ()
     */
    bool WithdrawRoute(IpAddress network, IpMaskOrPrefix networkMask);

  private:
    ~GlobalRouter() override;

    /**
     * @brief Clear list of LSAs
     */
    void ClearLSAs();

    /**
     * @brief Link through the given channel and find the net device that's on the other end.
     *
     * This only makes sense with a point-to-point channel.
     *
     * @param nd outgoing NetDevice
     * @param ch channel
     * @returns the NetDevice on the other end
     */
    Ptr<NetDevice> GetAdjacent(Ptr<NetDevice> nd, Ptr<Channel> ch) const;

    /**
     * @brief Finds a designated router
     *
     * Given a local net device, we need to walk the channel to which the net device is
     * attached and look for nodes with GlobalRouter interfaces on them (one of them
     * will be us).  Of these, the router with the lowest IP address on the net device
     * connecting to the channel becomes the designated router for the link.
     *
     * @param ndLocal local NetDevice to scan
     * @returns the IP address of the designated router
     */
    IpAddress FindDesignatedRouterForLink(Ptr<NetDevice> ndLocal) const;

    /**
     * @brief Checks for the presence of another router on the NetDevice
     *
     * Given a node and an attached net device, take a look off in the channel to
     * which the net device is attached and look for a node on the other side
     * that has a GlobalRouter interface aggregated.
     *
     * @param nd NetDevice to scan
     * @returns true if a router is found
     */
    bool AnotherRouterOnLink(Ptr<NetDevice> nd) const;

    /**
     * @brief Process a generic broadcast link
     *
     * @param nd the NetDevice
     * @param pLSA the Global LSA
     * @param c the returned NetDevice container
     */
    void ProcessBroadcastLink(Ptr<NetDevice> nd, GlobalRoutingLSA<T>* pLSA, NetDeviceContainer& c);

    /**
     * @brief Process a single broadcast link
     *
     * @param nd the NetDevice
     * @param pLSA the Global LSA
     * @param c the returned NetDevice container
     */
    void ProcessSingleBroadcastLink(Ptr<NetDevice> nd,
                                    GlobalRoutingLSA<T>* pLSA,
                                    NetDeviceContainer& c);

    /**
     * @brief Process a bridged broadcast link
     *
     * @param nd the NetDevice
     * @param pLSA the Global LSA
     * @param c the returned NetDevice container
     */
    void ProcessBridgedBroadcastLink(Ptr<NetDevice> nd,
                                     GlobalRoutingLSA<T>* pLSA,
                                     NetDeviceContainer& c);

    /**
     * @brief Process a point to point link
     *
     * @param ndLocal the NetDevice
     * @param pLSA the Global LSA
     */
    void ProcessPointToPointLink(Ptr<NetDevice> ndLocal, GlobalRoutingLSA<T>* pLSA);

    /**
     * @brief Build one NetworkLSA for each net device talking to a network that we are the
     * designated router for.
     *
     * @param c the devices.
     */
    void BuildNetworkLSAs(NetDeviceContainer c);

    /**
     * @brief Return a container of all non-bridged NetDevices on a link
     *
     * This method will recursively find all of the 'edge' devices in an
     * L2 broadcast domain.  If there are no bridged devices, then the
     * container returned is simply the set of devices on the channel
     * passed in as an argument.  If the link has bridges on it
     * (and therefore multiple ns3::Channel objects interconnected by
     * bridges), the method will find all of the non-bridged devices
     * in the L2 broadcast domain.
     *
     * @param ch a channel from the link
     * @returns the NetDeviceContainer.
     */
    NetDeviceContainer FindAllNonBridgedDevicesOnLink(Ptr<Channel> ch) const;

    /**
     * @brief Decide whether or not a given net device is being bridged by a BridgeNetDevice.
     *
     * @param nd the NetDevice
     * @returns the BridgeNetDevice smart pointer or null if not found
     */
    Ptr<BridgeNetDevice> NetDeviceIsBridged(Ptr<NetDevice> nd) const;

    typedef std::list<GlobalRoutingLSA<T>*> ListOfLSAs_t; //!< container for the GlobalRoutingLSAs
    ListOfLSAs_t m_LSAs;                                  //!< database of GlobalRoutingLSAs

    IpAddress m_routerId;                                    //!< router ID (its IPv4 address)
    Ptr<GlobalRouting<IpRoutingProtocol>> m_routingProtocol; //!< the Ipv4GlobalRouting in use

    typedef typename std::list<IpRoutingTableEntry*>
        InjectedRoutes; //!< container of Ipv4RoutingTableEntry
    typedef typename std::list<IpRoutingTableEntry*>::const_iterator
        InjectedRoutesCI; //!< Const Iterator to container of Ipv4RoutingTableEntry
    typedef typename std::list<IpRoutingTableEntry*>::iterator
        InjectedRoutesI;             //!< Iterator to container of Ipv4RoutingTableEntry
    InjectedRoutes m_injectedRoutes; //!< Routes we are exporting

    // Declared mutable so that const member functions can clear it
    // (supporting the logical constness of the search methods of this class)
    /**
     * Container of bridges visited.
     */
    mutable std::vector<Ptr<BridgeNetDevice>> m_bridgesVisited;
    /**
     * Clear the list of bridges visited on the link
     */
    void ClearBridgesVisited() const;
    /**
     * When recursively checking for devices on the link, check whether a
     * given device has already been visited.
     *
     * @param device the bridge device to check
     * @return true if bridge has already been visited
     */
    bool BridgeHasAlreadyBeenVisited(Ptr<BridgeNetDevice> device) const;
    /**
     * When recursively checking for devices on the link, mark a given device
     * as having been visited.
     *
     * @param device the bridge device to mark
     */
    void MarkBridgeAsVisited(Ptr<BridgeNetDevice> device) const;

    // inherited from Object
    void DoDispose() override;
};

/**
 * @ingroup globalrouting
 * Create the typedef Ipv4GlobalRouting with T as Ipv4RoutingProtocol
 */
typedef GlobalRouter<Ipv4Manager> Ipv4GlobalRouter;

} // namespace ns3

#endif /* GLOBAL_ROUTER_INTERFACE_H */
