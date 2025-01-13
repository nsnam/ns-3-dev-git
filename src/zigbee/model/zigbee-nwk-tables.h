/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Ryo Okuda <c611901200@tokushima-u.ac.jp>
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_TABLES_H
#define ZIGBEE_TABLES_H

#include "zigbee-nwk-fields.h"

#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/timer.h"

#include <cassert>
#include <deque>
#include <map>
#include <stdint.h>
#include <sys/types.h>

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup zigbee
 *  Route record states
 */
enum RouteStatus
{
    ROUTE_ACTIVE = 0x0,              //!< Route active
    ROUTE_DISCOVERY_UNDERWAY = 0x1,  //!< Route discovery underway
    ROUTE_DISCOVER_FAILED = 0x2,     //!< Route discovery failed
    ROUTE_INACTIVE = 0x3,            //!< Route inactive
    ROUTE_VALIDATION_UNDERWAY = 0x4, //!< Route discovery validation underway
};

/**
 * @ingroup zigbee
 * The relationship between the neighbor and the current device
 */
enum Relationship
{
    NBR_PARENT = 0x0,        //!< Neighbor is the parent
    NBR_CHILD = 0x01,        //!< Neighbor is the child
    NBR_SIBLING = 0x02,      //!< Neighbor is the sibling
    NBR_NONE = 0x03,         //!< No relationship
    NBR_PREV_CHILD = 0x04,   //!< Neighbor was a previous child
    NBR_UNAUTH_CHILD = 0x05, //!< Neighbor is an unauthenticated child
};

/**
 * @ingroup zigbee
 * The network layer device type
 */
enum NwkDeviceType
{
    ZIGBEE_COORDINATOR = 0x0, //!< Zigbee coordinator
    ZIGBEE_ROUTER = 0x01,     //!< Zigbee router
    ZIGBEE_ENDDEVICE = 0x02   //!< Zigbee end device
};

/**
 * @ingroup zigbee
 * Routing table entry
 * Zigbee Specification r22.1.0, Table 3-66
 */
class RoutingTableEntry : public SimpleRefCount<RoutingTableEntry>
{
  public:
    /**
     * The constructor the routing table entry.
     *
     * @param dst The destination nwkAddress (MAC 16-bit Address).
     * @param status The status of the current entry.
     * @param noRouteCache The value of the route cache flag.
     * @param manyToOne The value of the manyToOne flag.
     * @param routeRecordReq The value of the Route record required flag.
     * @param groupId The value of the group id flag.
     * @param nextHopAddr The value of the 16 bit next hop address.
     */
    RoutingTableEntry(Mac16Address dst,
                      RouteStatus status,
                      bool noRouteCache,
                      bool manyToOne,
                      bool routeRecordReq,
                      bool groupId,
                      Mac16Address nextHopAddr);

    RoutingTableEntry();
    ~RoutingTableEntry();

    /**
     * Set the entry destination nwkAddress (MAC 16-bit address)
     *
     * @param dst The value of the destination nwkAddress
     */
    void SetDestination(Mac16Address dst);

    /**
     * Get the entry destination nwkAddress(MAC 16-bit address)
     *
     * @return The value of the nwkAddress stored in this entry.
     */
    Mac16Address GetDestination() const;

    /**
     * Indicates if the No Route Cache flag is active.
     *
     * @return The value of the no route cache flag.
     */
    bool IsNoRouteCache() const;

    /**
     * Indicates if the Many-to-One flag is active.
     *
     * @return The value of the Many-to-one flag.
     */
    bool IsManyToOne() const;

    /**
     * Indicate if the route record request is active.
     *
     * @return The value of the route record request flag.
     */
    bool IsRouteRecordReq() const;

    /**
     * Indicates if the Group Id flag is active.
     *
     * @return The value of the Group id flag.
     */
    bool IsGroupIdPresent() const;

    /**
     * Set the value of the next hop address.
     *
     * @param nextHopAddr The next hop 16-bit nwkAddress
     */
    void SetNextHopAddr(Mac16Address nextHopAddr);

    /**
     * Get the value of the next hop address.
     *
     * @return Mac16Address The 16 bit next hop address.
     */
    Mac16Address GetNextHopAddr() const;

    /**
     * Set the lifetime of the entry
     * @param lt The time used in the entry lifetime
     */
    void SetLifeTime(Time lt);

    /**
     * Get the value of the entry lifetime.
     * @return the lifetime
     */
    Time GetLifeTime() const;

    /**
     * Set the status of the routing table entry.
     * @param status the status of the routing table entry.
     */
    void SetStatus(RouteStatus status);

    /**
     * Get the status of the routing table entry.
     * @return The status of the routing table entry.
     */
    RouteStatus GetStatus() const;

    /**
     * Print the values of the routing table entry.
     *
     * @param stream The stream object used to print.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

  private:
    Mac16Address m_destination;   //!< The 16 bit network address or group id of this route.
                                  //!< If the dst device is a router, coordinator or end device, and
                                  //!< nwkAddrAlloc has a value of STOCHASTIC_ALLOC this field shall
                                  //!< contain the actual 16 bit addr of that device. If the dst
                                  //!< device is an end device and nwkAddrAlloc has a value of
                                  //!< DISTRIBUTED_ALLOC, this field shall contain the 16 bit
                                  //!< nwk address of the device's parent.
    RouteStatus m_status;         //!< The status of the route.
                                  //!< Also see Zigbee specification r22.1.0, Table 3-67
    bool m_noRouteCache{true};    //!< A flag indicating that the destination indicated by
                                  //!< this address does not store source routes.
    bool m_manyToOne{false};      //!< A flag indicating  that the destination is a concentrator
                                  //!< that issued a  many-to-one route request
    bool m_routeRecordReq{false}; //!< A flag indicating that the route record command frame should
                                  //!< be sent to the destination prior to the next data packet.
    bool m_groupId{false};        //!< A flag indicating that the destination address is a group id.
    Mac16Address m_nextHopAddr;   //!< The 16 bit network address of the next hop on the way to the
                                  //!< destination.
    Time m_lifeTime;              //!< Indicates the lifetime of the entry
};

/**
 *  The Network layer Route Discovery Table Entry
 *  See Zigbee specification r22.1.0, Table 3-68
 */
class RouteDiscoveryTableEntry : public SimpleRefCount<RouteDiscoveryTableEntry>
{
  public:
    /**
     * The constructor of the route discovery entry
     *
     * @param rreqId The sequence number used by the RREQ command frame
     * @param src The source address of the RREQ initiator.
     * @param snd The address of the device that has sent most recent lowest cost RREQ.
     * @param forwardCost The accumulated path cost from the RREQ src to this device.
     * @param residualCost The accumulated path cost from this device to the destination.
     * @param expTime The expiration time of this entry.
     */
    RouteDiscoveryTableEntry(uint8_t rreqId,
                             Mac16Address src,
                             Mac16Address snd,
                             uint8_t forwardCost,
                             uint8_t residualCost,
                             Time expTime);

    RouteDiscoveryTableEntry();
    ~RouteDiscoveryTableEntry();

    /**
     * Get the route request id (The sequence number used by the RREQ command frame).
     *
     * @return The route request id (RREQ command frame sequence number).
     */
    uint8_t GetRreqId() const;

    /**
     * Get the source address of the entry's RREQ initiator.
     *
     * @return The address of the RREQ initiator.
     */
    Mac16Address GetSourceAddr() const;

    /**
     * Get the sender address of the entry.
     *
     * @return The sender address
     */
    Mac16Address GetSenderAddr() const;

    /**
     * Get the forward cost of this entry.
     *
     * @return The forward cost
     */
    uint8_t GetForwardCost() const;

    /**
     * Get the value of a residual cost (pathcost) updated by a RREP
     * in this entry.
     *
     * @return The residual cost.
     */
    uint8_t GetResidualCost() const;

    /**
     * Set the forward cost of this entry
     *
     * @param pathCost The pathCost of the most optimal route found.
     */
    void SetForwardCost(uint8_t pathCost);

    /**
     * Set the sender address of this entry
     * @param sender The last hop sender of the last optimal route found.
     */
    void SetSenderAddr(Mac16Address sender);

    /**
     * Set the resulting pathcost on a reception of a RREP previously requested.
     *
     * @param pathcost The pathcost contained in the RREP
     */
    void SetResidualCost(uint8_t pathcost);

    /**
     * Get the expiration time of this entry
     *
     * @return The expiration time of this entry.
     */
    Time GetExpTime() const;

    /**
     * Set the expiration time of the route discovery entry.
     *
     * @param exp The expiration time.
     */
    void SetExpTime(Time exp);

    /**
     * Print the values of the route discovery table entry
     *
     * @param stream The stream object used to print.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

  private:
    uint8_t m_routeRequestId;  //!< The sequence number for a RREQ command frame that is incremented
                               //!< each time a device initiates a RREQ.
    Mac16Address m_sourceAddr; //!< The 16-bit network address of the RREQ initiator.
    Mac16Address m_senderAddr; //!< The 16-bit network address of the device that has sent
                               //!< the most recent lowest cost RREQ command frame corresponding to
                               //!< this entry's RREQ id and source addr. This field is used to
                               //!< determine the path that an eventual RREP command should follow.
    uint8_t m_forwardCost;     //!< The accumulated path cost from the source of the RREQ to the
                               //!< current device.
    uint8_t m_residualCost;    //!< The accumulated path cost from the current device to the
                               //!< destination device.
    Time m_expirationTime;     //!< A time stamp indicating the expiration time. The default value
                               //!< of the entry is equals to current time + nwkcRouteDiscoveryTime.
};

/**
 *  The Network layer Neighbor Table Entry
 *  See Zigbee specification r22.1.0, Table 3-63
 */
class NeighborTableEntry : public SimpleRefCount<NeighborTableEntry>
{
  public:
    NeighborTableEntry();
    ~NeighborTableEntry();

    /**
     * Construct a new Neighbor Table Entry object. This constructor do not include
     * the optional or additional fields.
     *
     * @param extAddr The EUI-64 address of the device to register (IEEEaddress)
     * @param nwkAddr The short address of the device to register (The MAC 16 address)
     * @param deviceType The type of neighbor device
     * @param rxOnWhenIdle Indication of whether the neighbor enabled during idle periods.
     * @param endDevConfig The end device configuration
     * @param timeoutCounter The remaining current time in seconds.
     * @param devTimeout The timeout in seconds of the end device
     * @param relationship The relationship between the neighbor and the current device
     * @param txFailure Indicates whether or not a previous Tx to the device was successful.
     * @param lqi The link quality indicator (LQI) value
     * @param outgoingCost Cost of the link measured by the neighbor
     * @param age Num of nwkLinkStatusPeriods since the link status cmd was received.
     * @param keepaliveRx Indicates if at least one keep alive was received since the router was
     * rebooted.
     * @param macInterfaceIndex The MAC interface index
     */
    NeighborTableEntry(Mac64Address extAddr,
                       Mac16Address nwkAddr,
                       NwkDeviceType deviceType,
                       bool rxOnWhenIdle,
                       uint16_t endDevConfig,
                       Time timeoutCounter,
                       Time devTimeout,
                       Relationship relationship,
                       uint8_t txFailure,
                       uint8_t lqi,
                       uint8_t outgoingCost,
                       uint8_t age,
                       bool keepaliveRx,
                       uint8_t macInterfaceIndex);

    /**
     * Get the entry registered IEEE address (EUI-64 address).
     *
     * @return The entry EUI-64 address.
     */
    Mac64Address GetExtAddr() const;

    /**
     * Get the entry registered Network address (MAC short address).
     *
     * @return The 16 bit address registered in this entry.
     */
    Mac16Address GetNwkAddr() const;

    /**
     * Get the device type of this neighbor device.
     *
     * @return The type of device registered for this neighbor.
     */
    NwkDeviceType GetDeviceType() const;

    /**
     * Return true is neighboring device is on when idle.
     *
     * @return True if device is on when idle.
     */
    bool IsRxOnWhenIdle() const;

    /**
     * Get the end device configuration object.
     *
     * @return The end device configuration in this entry.
     */
    uint16_t GetEndDevConfig() const;

    /**
     * Get the timeout counter object.
     *
     * @return The extended PAN Id registered to this entry.
     */
    Time GetTimeoutCounter() const;

    /**
     * Get the device timeout object.
     *
     * @return The device timeout time
     */
    Time GetDevTimeout() const;

    /**
     * Get the relationship object.
     *
     * @return The relationship between the neighbor and current device.
     */
    uint8_t GetRelationship() const;

    /**
     * Get the Tx Failure object
     *
     * @return A value indicating if a prev Tx to the device was successful.
     */
    uint8_t GetTxFailure() const;

    /**
     * Get the LQI value from this device to an entry neighbor
     *
     * @return The LQI value
     */
    uint8_t GetLqi() const;

    /**
     * Get the outgoing cost object.
     *
     * @return The cost of the outgoing link as measured by a neighbor.
     */
    uint8_t GetOutgoingCost() const;

    /**
     * Get the number of nwkLinkStatusPeriod intervals since the
     * link status command was received.
     *
     * @return The number of nwkLinkStatusPeriod intervals
     */
    uint8_t GetAge() const;

    /**
     * Get the time in symbols at which the last beacon frame
     * was received from the neighbor.
     *
     * @return time in symbols since last beacon frame.
     */
    uint64_t GetIncBeaconTimestamp() const;

    /**
     * Get the transmission time difference in symbols, between the
     * neighbor's beacon and its parent beacon.
     *
     * @return Time difference (symbols) between beacon and parent beacon.
     */
    uint64_t GetBeaconTxTimeOffset() const;

    /**
     * Get the MAC Interface Index object
     *
     * @return The MAC interface index registered for this entry
     */
    uint8_t GetMacInterfaceIndex() const;

    /**
     * Get the number of unicast bytes transmitted to the neighbor
     * registered in this entry.
     *
     * @return Unicast transmitted bytes.
     */
    uint32_t GetMacUcstBytesTx() const;

    /**
     * Get the number of unicast bytes received to the neighbor
     * registered in this entry.
     *
     * @return Unicast transmitted bytes.
     */
    uint32_t GetMacUcstBytesRx() const;

    /**
     * Get the extended PAN identifier
     *
     * @return The extended PAN identifier.
     */
    uint64_t GetExtPanId() const;

    /**
     * Get the logical channel used by the the neighbor in this entry.
     *
     * @return The channel number used by this neighbor.
     */
    uint8_t GetLogicalCh() const;

    /**
     * The depth of the neighbor device.
     *
     * @return The depth of the neighbor device in this entryj
     */
    uint8_t GetDepth() const;

    /**
     * Get the value of the beacon order set in this neighbor.
     * See IEEE 802.15.4-2011.
     *
     * @return The value of the beacon order registered for this neighbor entry.
     */
    uint8_t GetBeaconOrder() const;

    /**
     * Get the the value of the potential parent field.
     *
     * @return A confirmation of whether of not the device has been discarded a potential parent.
     */
    uint8_t IsPotentialParent() const;

    /**
     * Set the entry registered Network address (MAC short address).
     *
     * @param nwkAddr The 16 bit address to set in this entry.
     */
    void SetNwkAddr(Mac16Address nwkAddr);

    /**
     * Set the device type of this neighbor device.
     *
     * @param devType The device to set for this entry.
     */
    void SetDeviceType(NwkDeviceType devType);

    /**
     * Set the device is on when idle flag.
     *
     * @param onWhenIdle Set if device is on when idle.
     */
    void SetRxOnWhenIdle(bool onWhenIdle);

    /**
     * Set the end device configuration.
     *
     * @param conf The end device configuration to set in this entry.
     */
    void SetEndDevConfig(uint16_t conf);

    /**
     * Set the timeout counter object.
     *
     * @param counter The timeout counter used by this entry.
     */
    void SetTimeoutCounter(Time counter);

    /**
     * Set the device timeout object.
     *
     * @param timeout The device timeout time
     */
    void SetDevTimeout(Time timeout);

    /**
     * Set the relationship object.
     *
     * @param relationship The relationship between the neighbor and current device.
     */
    void SetRelationship(Relationship relationship);

    /**
     * Set the Tx Failure object
     *
     * @param failure A value indicating if a prev Tx to the device was successful.
     */
    void SetTxFailure(uint8_t failure);

    /**
     * Set the Link quality indicator value from this device to an entry neighbor
     *
     * @param lqi The LQI value
     */
    void SetLqi(uint8_t lqi);

    /**
     * Set the outgoing cost object.
     *
     * @param cost The cost of the outgoing link as measured by a neighbor.
     */
    void SetOutgoingCost(uint8_t cost);

    /**
     * Set the number of nwkLinkStatusPeriod intervals since the
     * link status command was received.
     *
     * @param age The number of nwkLinkStatusPeriod intervals
     */
    void SetAge(uint8_t age);

    /**
     * Set the time in symbols at which the last beacon frame
     * was received from the neighbor.
     *
     * @param timestamp time in symbols since last beacon frame.
     */
    void SetIncBeaconTimestamp(uint64_t timestamp);

    /**
     * Set the transmission time difference in symbols, between the
     * neighbor's beacon and its parent beacon.
     *
     * @param offset Time difference (symbols) between beacon and parent beacon.
     */
    void SetBeaconTxTimeOffset(uint64_t offset);

    /**
     * Set the MAC Interface Index object
     *
     * @param index The MAC interface index registered for this entry
     */
    void SetMacInterfaceIndex(uint8_t index);

    /**
     * Set the number of unicast bytes transmitted to the neighbor
     * registered in this entry.
     *
     * @param txBytes Unicast transmitted bytes.
     */
    void SetMacUcstBytesTx(uint32_t txBytes);

    /**
     * Set the number of unicast bytes received to the neighbor
     * registered in this entry.
     *
     * @param rxBytes Unicast transmitted bytes.
     */
    void SetMacUcstBytesRx(uint32_t rxBytes);

    /**
     * Set the extended PAN identifier
     *
     * @param extPanId The extended PAN identifier.
     */
    void SetExtPanId(uint64_t extPanId);

    /**
     * Set the logical channel used by the the neighbor in this entry.
     *
     * @param channel The channel number used by this neighbor.
     */
    void SetLogicalCh(uint8_t channel);

    /**
     * Set the depth of the neighbor device.
     *
     * @param depth The depth of the neighbor device in this entryj
     */
    void SetDepth(uint8_t depth);

    /**
     * Set the value of the beacon order set in this neighbor.
     * See IEEE 802.15.4-2011.
     *
     * @param bo The value of the beacon order registered for this neighbor entry.
     */
    void SetBeaconOrder(uint8_t bo);

    /**
     * Set the the value of the potential parent field.
     *
     * @param confirm A confirm of whether or not the device has been discarded a potential parent.
     */
    void SetPotentialParent(bool confirm);

    /**
     * Print the values of the neighbor table entry.
     *
     * @param stream The stream object used to print.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

  private:
    Mac64Address m_extAddr;     //!< The IEEE EUI-64 bit address that is unique to every device.
    Mac16Address m_nwkAddr;     //!< The 16 bit network address of the neighboring device.
    NwkDeviceType m_deviceType; //!< The type of neighbor device
    bool m_rxOnWhenIdle;     //!< Indicates if the neighbor receiver is enabled during idle periods.
    uint16_t m_endDevConfig; //!< The end device configuration.
                             //!< See Zigbee Specification r22.1.0, 3.4.11.3.2
    Time m_timeoutCounter; //!< Indicates the current time remaining in seconds, for the end device.
    Time m_devTimeout; //!< This field indicates the timeout, in seconds, for the end device child.
    Relationship m_relationship; //!< Relationship between the neighbor and the current device.
    uint8_t m_txFailure;         //!< A value indicating if previous transmissions to the device
                                 //!< were successful or not. Higher values indicate more failures.
    uint8_t m_lqi;          //!< The estimated link quality for RF transmissions from this device
                            //!< See Zigbee specification r22.1.0, 3.6.3.1
    uint8_t m_outgoingCost; //!< The cost of the outgoing link as measured by the neighbor.
                            //!< A value of 0 indicates no outgoing cost available.
    uint8_t m_age; //!< The number of nwkLinkStatusPeriod intervals  since link status command
                   //!< was received. Mandatory if m_nwkSymLink = true
    uint64_t m_incBeaconTimestamp; //!< The time in symbols at which the last beacon frame
                                   //!< was received from the neighbor. This is equal to the
                                   //!< timestamp taken when the beacon frame was received
                                   //!< (Optional Field) See IEEE 802.15.4-2015[B1]
    uint64_t m_beaconTxTimeOffset; //!< The transmission time difference in symbols, between the
                                   //!< neighbor's beacon and its parent beacon (Optional field).
    bool m_keepaliveRx;          //!< This value indicates at least one keepalive has been received
                                 //!< from the end device since the router has rebooted.
    uint8_t m_macInterfaceIndex; //!< This is an index into the MAC Interface Table indicating
                                 //! what interface the neighbor or child is bound to.
    uint32_t m_macUcstBytesTx;   //!< The number of bytes transmitted via MAC unicast to the
                                 //!< neighbor (Optional field).
    uint32_t m_macUcstBytesRx;   //!< The number of bytes received via MAC unicasts from this
                                 //!< neighbor (Optional field).

    // Additional Neighbor Table Fields (Optional fields)
    // See Zigbee specification r22.1.0, Table 3-64

    uint64_t m_extPanId; //!< The extendend PAN id (based on the Zigbee coordinator EUI-64 address)
    uint8_t m_logicalCh; //!< The logical channel on which the network is operating.
    uint8_t m_depth;     //!< The tree depth of the neighbor device
    uint8_t m_bo{15};    //!< The beacon order of the device (See IEEE 802.15.4-2011)
    bool m_potentialParent{true}; //!< An indication of whether the device has been
                                  //!< ruled out as a potential parent.
};

/**
 *  A table that keeps record of neighbors devices NWK extended PAN ids (64 bits)
 *  and their related 16 bit MAC pan id. This is not explicitly mentioned
 *  in the Zigbee specification but it is required to keep record of this to
 *  enable the join process through association. This approach is also used by
 *  other stacks like Zboss stack 1.0.
 */
class PanIdTable
{
  public:
    /**
     * The constructor of the PanIdTable.
     */
    PanIdTable();

    /**
     * Add an entry to the PAN Id table.
     *
     * @param extPanId The extended PAN identifier (64 bits).
     * @param panId The 16 bit PAN id used by the MAC layer .
     */
    void AddEntry(uint64_t extPanId, uint16_t panId);

    /**
     *  Get the 16 bit MAC PAN id based on the reference extended PAN id.
     *
     * @param extPanId The NWK extended PAN identifier (64 bits).
     * @param panId The returned 16 bit PAN Id.
     * @return True if the entry was found
     */
    bool GetEntry(uint64_t extPanId, uint16_t& panId);

    /**
     * Dispose of the table and all its elements
     */
    void Dispose();

  private:
    std::map<uint64_t, uint16_t> m_panIdTable; //!< The Map object that represents
                                               //!< the table of PAN ids.
};

/**
 * The route request retry table entry.
 * It represents information stored about future route requests retries.
 */
class RreqRetryTableEntry : public SimpleRefCount<RreqRetryTableEntry>
{
  public:
    /**
     * The constructor of the RREQ retry table entry
     *
     * @param rreqId The RREQ ID
     * @param rreqRetryEvent The EventId of the next RREQ retry callback
     * @param rreqRetryCount The current number of RREQ retries for this RREQ ID
     */
    RreqRetryTableEntry(uint8_t rreqId, EventId rreqRetryEvent, uint8_t rreqRetryCount);

    /**
     * Get the RREQ Id from the entry
     *
     * @return The RREQ ID from the entry
     */
    uint8_t GetRreqId() const;

    /**
     * Set the RREQ retry count from the entry
     *
     * @param rreqRetryCount The value of the RREQ retry count
     */
    void SetRreqRetryCount(uint8_t rreqRetryCount);

    /**
     * Get the RREQ retry count from the entry
     *
     * @return The RREQ retry count
     */
    uint8_t GetRreqRetryCount() const;

    /**
     *  Set the event id of the RREQ retry
     *
     * @param eventId The event id corresponding to the next RREQ retry callback
     */
    void SetRreqEventId(EventId eventId);

    /**
     * Get the RREQ id of the RREQ retry
     *
     * @return The event id
     */
    EventId GetRreqEventId();

    /**
     * Print the values of the RREQ retry table entry.
     *
     * @param stream The stream object used to print.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

  private:
    uint8_t m_rreqId;           //!< The RREQ ID
    EventId m_rreqRetryEventId; //!< The event id of the next RREQ retry callback
    uint8_t m_rreqRetryCount;   //!< The number of RREQ retries
};

/**
 * A broadcast Transaction Record (BTR)
 * As described in Table 3-70
 */
class BroadcastTransactionRecord : public SimpleRefCount<BroadcastTransactionRecord>
{
  public:
    BroadcastTransactionRecord();
    ~BroadcastTransactionRecord();

    /**
     * Construct a new Broadcast Transaction Record (BTR) entry.
     *
     * @param srcAddr The source address of the BTR.
     * @param seq The sequence number of the BTR.
     * @param exp The expiration time of the BTR.
     */
    BroadcastTransactionRecord(Mac16Address srcAddr, uint8_t seq, Time exp);

    /**
     * Get the source address of the BTR.
     *
     * @return The source address.
     */
    Mac16Address GetSrcAddr() const;

    /**
     * Get the sequence number of the BTR.
     *
     * @return The sequence number.
     */
    uint8_t GetSeqNum() const;

    /**
     * Get the value of the expiration time in the BTR.
     *
     * @return The expiration time
     */
    Time GetExpirationTime() const;

    /**
     * Get the broadcast retry count value
     *
     * @return The broadcast retry count value
     */
    uint8_t GetBcstRetryCount() const;

    /**
     * Set the source address of the BTR
     *
     * @param srcAddr The source address of the BTR
     */
    void SetSrcAddr(Mac16Address srcAddr);

    /**
     * Set the sequence number of the BTR
     *
     * @param seq The sequence number of the BTR
     */
    void SetSeqNum(uint8_t seq);

    /**
     * Set the expiration time object
     *
     * @param exp The expiration time of the BTR
     */
    void SetExpirationTime(Time exp);

    /**
     * Set the Broadcast retry count object
     *
     * @param count  The value of the counter
     */
    void SetBcstRetryCount(uint8_t count);

    /**
     * Print the values of the BTR.
     *
     * @param stream The stream object used to print.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

  private:
    Mac16Address m_srcAddr;        //!< The 16-bit network address of the broadcast initiator.
    uint8_t m_sequenceNumber;      //!< The RREQ sequence number of the initiator's broadcast.
    Time m_expirationTime;         //!< An indicator of when the entry expires
    uint8_t m_broadcastRetryCount; //!< The number of times this BCST has been retried.
};

/**
 *  The network layer Routing Table.
 *  See Zigbee specification r22.1.0, 3.6.3.2
 */
class RoutingTable
{
  public:
    /**
     * The constructuctor of a Routing Table object
     */
    RoutingTable();

    /**
     * Adds an entry to the routing table.
     *
     * @param rt The routing table entry
     * @return True if the entry was added to the table.
     */
    bool AddEntry(Ptr<RoutingTableEntry> rt);

    /**
     * Look for an specific entry in the routing table.
     *
     * @param dstAddr The value of the destination address used to look for an entry.
     * @param entryFound The returned entry if found
     * @return True if the entry was found in the routing table.
     */
    bool LookUpEntry(Mac16Address dstAddr, Ptr<RoutingTableEntry>& entryFound);

    /**
     * Purge old entries from the routing table.
     */
    void Purge();

    /**
     * Identify and mark entries as ROUTE_INACTIVE status for entries who
     * have exceeded their lifetimes.
     */
    void IdentifyExpiredEntries();

    /**
     * Remove an entry from the routing table.
     *
     * @param dst The MAC 16 bit destination address of the entry to remove.
     */
    void Delete(Mac16Address dst);

    /**
     * Delete the first occrurance of an expired entry (ROUTE_INACTIVE status)
     */
    void DeleteExpiredEntry();

    /**
     * Print the Routing table.
     *
     * @param stream The stream object used to print the routing table.
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

    /**
     * Dispose of the table and all its elements
     */
    void Dispose();

    /**
     * Get the size of the routing table.
     *
     * @return The current size of the routing table
     */
    uint32_t GetSize();

    /**
     * Set the maximum size of the routing table
     *
     * @param size The size of the routing table.
     */
    void SetMaxTableSize(uint32_t size);

    /**
     *  Get the maximum size of the routing table.
     *
     *  @return The maximum size of the routing table.
     */
    uint32_t GetMaxTableSize() const;

  private:
    uint32_t m_maxTableSize;                           //!< The maximum size of the routing table;
    std::deque<Ptr<RoutingTableEntry>> m_routingTable; //!< The object that
                                                       //!< represents the routing table.
};

/**
 *  The network layer Route Discovery Table
 *  See Zigbee specification r22.1.0, 3.6.3.2
 */
class RouteDiscoveryTable
{
  public:
    /**
     * Constructor of route discovery table
     */
    RouteDiscoveryTable();

    /**
     * Add an entry to the route discovery table, in essence the contents of a RREQ command.
     *
     * @param rt The route discovery table entry.
     * @return True if the entry was added successfully
     */
    bool AddEntry(Ptr<RouteDiscoveryTableEntry> rt);

    /**
     * Look up for a route discovery table entry, the seareched entry must match the id
     * and the src address of the initiator.
     *
     * @param id The RREQ id (RREQ command sequence number)
     * @param src The 16 bit address of the initiator device.
     * @param entryFound The returned entry if found
     * @return True if element found
     */
    bool LookUpEntry(uint8_t id, Mac16Address src, Ptr<RouteDiscoveryTableEntry>& entryFound);

    /**
     * Purge old entries from the route discovery table.
     */
    void Purge();

    /**
     * Delete an entry from the route discovery table.
     *
     * @param id The RREQ id of the entry to delete.
     * @param src The src address of the initiator of the entry to delete.
     */
    void Delete(uint8_t id, Mac16Address src);

    /**
     * Print the contents of the route discovery table
     *
     * @param stream The stream object used to print the table.
     */
    void Print(Ptr<OutputStreamWrapper> stream);

    /**
     * Dispose of the table and all its elements
     */
    void Dispose();

  private:
    uint32_t m_maxTableSize; //!< The maximum size of the route discovery table
    std::deque<Ptr<RouteDiscoveryTableEntry>> m_routeDscTable; //!< The route discovery table object
};

/**
 *  The network layer Network Table
 *  See Zigbee specification r22.1.0, 3.6.1.5
 */
class NeighborTable
{
  public:
    /**
     * The neighbor table constructor
     */
    NeighborTable();

    /**
     * Add an entry to the neighbor table.
     *
     * @param entry The entry to be added to the neighbor table.
     * @return True if the entry was added to the table.
     */
    bool AddEntry(Ptr<NeighborTableEntry> entry);

    /**
     *  Look and return and entry if exists in the neighbor table.
     *
     * @param nwkAddr The network address used to look for an element in the table.
     * @param entryFound The returned entry if found in the neighbor table.
     * @return True if the entry was found false otherwise.
     */
    bool LookUpEntry(Mac16Address nwkAddr, Ptr<NeighborTableEntry>& entryFound);

    /**
     *  Look and return and entry if exists in the neighbor table.
     *
     * @param extAddr The extended address (EUI-64) used to look for an element in the table.
     * @param entryFound The returned entry if found in the neighbor table.
     * @return True if the entry was found false otherwise.
     */
    bool LookUpEntry(Mac64Address extAddr, Ptr<NeighborTableEntry>& entryFound);

    /**
     *  Look for this device Parent neighbor (A.K.A coordinator).
     *
     * @param entryFound The returned entry if found in the neighbor table.
     * @return True if the parent was found in the neighbor table.
     */
    bool GetParent(Ptr<NeighborTableEntry>& entryFound);

    /**
     * Perform a search for the best candidate parent based on some attributes.
     * See Association Join process , Zigbee specification R21, Section 3.6.1.4.1
     *
     * @param epid The extended PAN id of the network in which we look for parent candidates.
     * @param entryFound The returned entry if found
     * @return True if element found
     */
    bool LookUpForBestParent(uint64_t epid, Ptr<NeighborTableEntry>& entryFound);

    /**
     * Remove old entries from the neighbor table.
     */
    void Purge();

    /**
     * Delete the specified entry from the neighbor table.
     *
     * @param extAddr The EUI-64 of the entry to be removed from the neighbor table
     */
    void Delete(Mac64Address extAddr);

    /**
     * Print the neighbor table
     *
     * @param stream The output stream where the table is printed
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

    /**
     * Get the size of the neighbor table.
     *
     * @return The current size of the neighbor table
     */
    uint32_t GetSize();

    /**
     * Set the maximum size of the neighbor table
     *
     * @param size The size of the neighbor table.
     */
    void SetMaxTableSize(uint32_t size);

    /**
     *  Get the maximum size of the neighbor table.
     *
     *  @return The maximum size of the neighbor table.
     */
    uint32_t GetMaxTableSize() const;

    /**
     * Dispose of the table and all its elements
     */
    void Dispose();

  private:
    /**
     *  Get the link cost based on the link quality indicator (LQI) value.
     *  The calculation of the link cost is based on a non-linear mapping of
     *  the lqi. A link cost of 1 is the best possible value (equivalent to 255 LQI).
     *
     *  @param lqi The link quality indicator value (0-255)
     *  @return The link cost value (1-7)
     */
    uint8_t GetLinkCost(uint8_t lqi) const;

    std::deque<Ptr<NeighborTableEntry>> m_neighborTable; //!< The neighbor table object
    uint32_t m_maxTableSize;                             //!< The maximum size of the neighbor table
};

/**
 *  A table storing information about upcoming route request retries. This table is use to keep
 *  track of all RREQ retry attempts from a given device.
 */
class RreqRetryTable
{
  public:
    /**
     * Adds an entry to the table
     *
     * @param entry The RREQ retry table entry
     * @return True if the entry was added to the table.
     */
    bool AddEntry(Ptr<RreqRetryTableEntry> entry);

    /**
     *  Look up for an entry in the table
     *
     *  @param rreqId The RREQ ID used to look up for an entry in the table
     *  @param entryFound The returned entry if found in the table
     *  @return True if a searched entry is found
     */
    bool LookUpEntry(uint8_t rreqId, Ptr<RreqRetryTableEntry>& entryFound);

    /**
     *  Delete an entry from the table using the RREQ ID
     *
     *  @param rreqId The RREQ ID use to delete an entry from the table
     */
    void Delete(uint8_t rreqId);

    /**
     * Print the neighbor table
     *
     * @param stream The output stream where the table is printed
     */
    void Print(Ptr<OutputStreamWrapper> stream) const;

    /**
     * Dispose of the table and all its elements
     */
    void Dispose();

  private:
    std::deque<Ptr<RreqRetryTableEntry>>
        m_rreqRetryTable; //!< The Table containing  RREQ Table entries.
};

/**
 * The Broadcast Transaction Table (BTT)
 * The BTT is used to keep track of data broadcast transactions.
 * The broadcast of link status request and route requests (RREQ) commands
 * are handled differently and not recorded by this table.
 * See Zigbee specification r22.1.0, Section 3.6.5
 */
class BroadcastTransactionTable
{
  public:
    BroadcastTransactionTable();

    /**
     * Add a broadcast transaction record (BTR) to the broadcast transaction table(BTT).
     *
     * @param entry The object representing the BTR
     * @return True if the object was added.
     */
    bool AddEntry(Ptr<BroadcastTransactionRecord> entry);

    /**
     * Get the current Size of the broadcast transaction table (BTT).
     *
     * @return uint32_t
     */
    uint32_t GetSize();

    /**
     * Set the maximum size of the broadcast transaction table (BTT)
     *
     * @param size The size of the broadcast transaction table (BTT).
     */
    void SetMaxTableSize(uint32_t size);

    /**
     *  Get the maximum size of the broadcast transaction table (BTT)
     *
     *  @return The maximum size of the broadcast transaction table (BTT).
     */
    uint32_t GetMaxTableSize() const;

    /**
     * Look up for broadcast transaction record in the broadcast transaction table (BTT).
     *
     * @param seq The sequence number of the broadcasted frame.
     * @param entryFound The returned entry if found in the table
     *
     * @return True if a searched entry is found
     */
    bool LookUpEntry(uint8_t seq, Ptr<BroadcastTransactionRecord>& entryFound);

    /**
     * Purge expired entries from the broadcast transaction table (BTT).
     */
    void Purge();

    /**
     * Dispose of all broadcast transaction records (BTR) in the broadcast transaction table(BTT).
     */
    void Dispose();

    /**
     * Print the broadcast transaction table (BTT)
     *
     * @param stream The output stream where the table is printed
     */
    void Print(Ptr<OutputStreamWrapper> stream);

  private:
    uint32_t m_maxTableSize; //!< The maximum size of the Broadcast Transaction table
    std::deque<Ptr<BroadcastTransactionRecord>>
        m_broadcastTransactionTable; //!< The list object representing the broadcast transaction
                                     //!< table (BTT)
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_TABLES_H */
