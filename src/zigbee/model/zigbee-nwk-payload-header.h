/*
 * Copyright (c) 2024 Tokushima University, Tokushima, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *  Authors: Ryo Okuda <c611901200@tokushima-u.ac.jp>
 *           Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_PAYLOAD_HEADERS_H
#define ZIGBEE_PAYLOAD_HEADERS_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"

namespace ns3
{
namespace zigbee
{

/**
 * Zigbee Specification, Payload command types
 */
enum NwkCommandType
{
    ROUTE_REQ_CMD = 0x01,        //!< Route request command
    ROUTE_REP_CMD = 0x02,        //!< Route response command
    NWK_STATUS_CMD = 0X03,       //!< Network status command
    LEAVE_CMD = 0x04,            //!< Leave network command
    ROUTE_RECORD_CMD = 0x05,     //!< Route record command
    REJOIN_REQ_CMD = 0x06,       //!< Rejoin request command
    REJOIN_RESP_CMD = 0x07,      //!< Rejoin response command
    LINK_STATUS_CMD = 0x08,      //!< Link status command
    NWK_REPORT_CMD = 0x09,       //!< Network report command
    NWK_UPDATE_CMD = 0x0a,       //!< Network update command
    TIMEOUT_REQ_CMD = 0x0b,      //!< Time out request command
    TIMEOUT_RESP_CMD = 0x0c,     //!< Time out response command
    LINK_POWER_DELTA_CMD = 0x0d, //!< Link power delta command
};

/**
 * Zigbee Specification 3.4.1.3.1 , Table 3-50
 * Values of the many to one command option field.
 */
enum ManyToOne
{
    NO_MANY_TO_ONE = 0,
    ROUTE_RECORD = 1,
    NO_ROUTE_RECORD = 2
};

/**
 * @ingroup zigbee
 * Represent the static portion of the zigbee payload header that describes
 * the payload command type.
 */
class ZigbeePayloadType : public Header
{
  public:
    ZigbeePayloadType();
    /**
     * Constructor
     * @param nwkCmd the command type of this command header
     */
    ZigbeePayloadType(enum NwkCommandType nwkCmd);
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the command frame type
     * @param nwkCmd the command frame type
     */
    void SetCmdType(enum NwkCommandType nwkCmd);
    /**
     * Get the command frame type
     * @return The command type from the command payload header
     */
    NwkCommandType GetCmdType() const;

  private:
    NwkCommandType m_nwkCmdType; //!< The network command Type
};

/**
 * @ingroup zigbee
 * Represent a variable portion of the zigbee payload header that includes
 * the route request command
 */
class ZigbeePayloadRouteRequestCommand : public Header
{
  public:
    ZigbeePayloadRouteRequestCommand();
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the command option field Many To One
     *
     * @param manyToOne  The Many To One field ()
     */
    void SetCmdOptManyToOneField(enum ManyToOne manyToOne);

    /**
     * Get the command option field Many To One
     *
     * @return The command option field
     */
    uint8_t GetCmdOptManyToOneField() const;

    /**
     * Set the Route request identifier
     * @param id the route request identifier
     */
    void SetRouteReqId(uint8_t id);

    /**
     * Get the Route request identifier
     * @return the route request identifier
     */
    uint8_t GetRouteReqId() const;

    /**
     * Set Destination address
     * @param addr The destination address (16 bit)
     */
    void SetDstAddr(Mac16Address addr);

    /**
     * Get the Destination address
     * @return the Destination address (16bits)
     */
    Mac16Address GetDstAddr() const;

    /**
     * Set the path cost
     * @param cost the path cost
     */
    void SetPathCost(uint8_t cost);

    /**
     * Set the path cost
     * @return the path cost
     */
    uint8_t GetPathCost() const;

    /**
     * Describe whether or not the destination IEEE Address field is present in the Route Request
     *
     * @return True if the command option destination IEEE Address field is active
     */
    bool IsDstIeeeAddressPresent() const;

    /**
     *  Set the destination IEEE address
     *  @param dst The destination IEEE address (64 bits)
     */
    void SetDstIeeeAddr(Mac64Address dst);

    /**
     * Get the destination IEEE address
     * @return The destination IEEE address (64bits)
     */
    Mac64Address GetDstIeeeAddr() const;

  private:
    /**
     * Set the complete command option field of the route request command.
     * @param cmdOptionField The 8 bits representing the complete command option field.
     */
    void SetCmdOptionField(uint8_t cmdOptionField);

    /**
     * Get the 8bits representing the complete command option field of the route request command.
     * @return The command option field (8 bits)
     */
    uint8_t GetCmdOptionField() const;

    /* Command options field (1 Octet) */
    ManyToOne m_cmdOptManyToOne; //!< Many to One Subfield (Bits 3-4)
    bool m_cmdOptDstIeeeAddr;    //!< Destination IEEE Address Flag (Bit 5)
    bool m_cmdOptMcst;           //!< Multicast Flag (Bit 6)
                                 // Bits 0-2,7 reserved

    /* Route Request command fields */
    uint8_t m_routeReqId;       //!< Route request identifier (1 Octet)
    Mac16Address m_dstAddr;     //!< Destination address (2 Octets)
    uint8_t m_pathCost;         //!< Path Cost (1 Octet)
    Mac64Address m_dstIeeeAddr; //!< Destination IEEE address (0-8 Octets)
};

/**
 * @ingroup zigbee
 * Represent a variable portion of the zigbee payload header that includes
 * the route reply command
 */
class ZigbeePayloadRouteReplyCommand : public Header
{
  public:
    ZigbeePayloadRouteReplyCommand();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the command option
     * @param option the command option
     */
    void SetCmdOption(uint8_t option);
    /**
     * Get the command option
     * @return the command option
     */
    uint8_t GetCmdOption() const;
    /**
     * Set the Route request identifier
     * @param rid the route request identifier
     */
    void SetRouteReqId(uint8_t rid);
    /**
     * Get the Route request identifier
     * @return the Radius
     */
    uint8_t GetRouteReqId() const;
    /**
     * Set Originator address
     * @param addr The originator address (16 bit)
     */
    void SetOrigAddr(Mac16Address addr);
    /**
     * Get the Originator address
     * @return the originator address (16bits)
     */
    Mac16Address GetOrigAddr() const;
    /**
     * Set Responder address
     * @param addr the responder address (16 bit)
     */
    void SetRespAddr(Mac16Address addr);
    /**
     * Get the Responder address
     * @return the responder address (16bits)
     */
    Mac16Address GetRespAddr() const;
    /**
     * Set the path cost
     * @param cost the path cost
     */
    void SetPathCost(uint8_t cost);
    /**
     * Get the path cost
     * @return the path cost
     */
    uint8_t GetPathCost() const;
    /**
     *  Set the Originator IEEE address
     *  @param orig The originator IEEE address (64 bits)
     */
    void SetOrigIeeeAddr(Mac64Address orig);
    /**
     * Get the Originator IEEE address
     * @return The originator IEEE address (64bits)
     */
    Mac64Address GetOrigIeeeAddr() const;
    /**
     *  Set the Responder IEEE address
     *  @param resp The responder IEEE address (64 bits)
     */
    void SetRespIeeeAddr(Mac64Address resp);
    /**
     * Get the Responder IEEE address
     * @return The responder IEEE address (64bits)
     */
    Mac64Address GetRespIeeeAddr() const;

  private:
    /* Command options field (1 Octet) */
    bool m_cmdOptOrigIeeeAddr; //!< Originator IEEE address flag (Bit 4)
    bool m_cmdOptRespIeeeAddr; //!< Responder IEEE address flag (Bit 5)
    bool m_cmdOptMcst;         //!< Multicast flag (Bit 6)
                               // Bits 0-3,7 reserved

    /* Route Reply command fields */
    uint8_t m_routeReqId;        //!< Route request identifier (1 Octet)
    Mac16Address m_origAddr;     //!< Originator address (2 Octets)
    Mac16Address m_respAddr;     //!< Responder address (2 Octets)
    uint8_t m_pathCost;          //!< Path cost (1 Octet)
    Mac64Address m_origIeeeAddr; //!< Originator IEEE address (0-8 Octets)
    Mac64Address m_respIeeeAddr; //!< Responder IEEE address (0-8 Octets)
};

/**
 * @ingroup zigbee
 * Represents the payload portion of a beacon frame.
 * See Zigbee specification r22.1.0, Table 3-71 (NWK layer Information fields)
 */
class ZigbeeBeaconPayload : public Header
{
  public:
    ZigbeeBeaconPayload();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the network profile identifier.
     *
     * @param stackProfile the stack Profile in use
     */
    void SetStackProfile(uint8_t stackProfile);

    /**
     * Get the Protocol Id used.
     * In this standard this value is always 0.
     *
     * @return The protocol id used in this standard.
     */
    uint8_t GetProtocolId() const;

    /**
     * Get the Stack Profile used.
     *
     * @return The stack profile identifier used.
     */
    uint8_t GetStackProfile() const;

    /**
     * Set the Router Capacity capability
     * True = The device is able to accept join.request
     * from router-capable devices
     *
     * @param routerCapacity The router capacity capability
     */
    void SetRouterCapacity(bool routerCapacity);

    /**
     * Get the router capacity capability.
     *
     * @return Whether or not is capable of accepting join request
     *         from router-capable devices.
     */
    bool GetRouterCapacity() const;

    /**
     * Set the cevice depth object
     *
     * @param deviceDepth The current device depth.
     */
    void SetDeviceDepth(uint8_t deviceDepth);

    /**
     * Get the device depth.
     *
     * @return The device depth
     */
    uint8_t GetDeviceDepth() const;

    /**
     * Set the end device Capacity
     *
     * @param endDevCapacity  The end device capacity
     */
    void SetEndDevCapacity(bool endDevCapacity);

    /**
     * Get the end dev capacity
     *
     * @return Whether or not the device is capable of accepting
     *         join requests from end devices.
     */
    bool GetEndDevCapacity() const;

    /**
     * Set the extended PAN id. This should be the value of the
     * zigbee coordinator EUI-64 address (IEEE address).
     *
     * @param extPanId The extended PAN id
     */
    void SetExtPanId(uint64_t extPanId);

    /**
     * Get the extended PAN identifier
     *
     * @return The extended PAN identifier
     */
    uint64_t GetExtPanId() const;

    /**
     * Set the Tx Offset time in symbols
     *
     * @param txOffset The Tx offset time in symbols
     */
    void SetTxOffset(uint32_t txOffset);

    /**
     * Get the Tx Offset time in symbols
     *
     * @return uint32_t The Tx offset time in symbols
     */
    uint32_t GetTxOffset() const;

    /**
     * Set the value of the nwkUpdateId to this beacon payload
     *
     * @param nwkUpdateId The nwkUpdateId to set to the beacon payload.
     */
    void SetNwkUpdateId(uint8_t nwkUpdateId);

    /**
     * Get the value of the nwkUpdateId set to this beacon payload
     *
     * @return The value of the nwkUpdateId in the beacon payload.
     */
    uint8_t GetNwkUpdateId() const;

  private:
    uint8_t m_protocolId{0}; //!< Identifies the network layer in use, in this specification this
                             //!< value is always 0.
    uint8_t m_stackProfile;  //!< The zigbee stack profile identifier.
    uint8_t m_protocolVer;   //!< The version of the zigbee protocol.
    bool m_routerCapacity{false}; //!< True if this device is capable of accepting
                                  //!< join requests from router capable devices.
                                  //!< This value should match the value of RoutersAllowed for the
                                  //!< MAC interface this beacon is being sent from.
    uint8_t m_deviceDepth;        //!< The network depth of this device.
                                  //!< 0x00 indicates this device is the ZC.
    bool m_endDevCapacity{false}; //!< True if the device is capable of accepting join request from
                                  //!< end devices seeking to join the network.
    uint64_t m_extPanId; //!< The globally unique id for the PAN. By default this is the EUI-64 bit
                         //!< (IEEE address) of the Zigbee coordinator that formed the network.
    uint32_t m_txOffset; //!< This indicates the difference in time, measured in symbols, between
                         //!< the beacon transmission time of its parent
    uint8_t m_nwkUpdateId; //!< The value of nwkUpdateId from the NIB.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_PAYLOAD_HEADER_H */
