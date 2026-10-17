/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_ZDO_CLIENT_HEADER_H
#define ZIGBEE_ZDO_CLIENT_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup zigbee
 *
 * Zigbee Device Profile (ZDP) Client Services Commands
 * Zigbee Specification r22.1.0, Section 2.4.3.1, Table 2-44
 *
 * The numbers used by this profile represent the matching Cluster ID used when sending
 * data meant for the ZDO using the APS layer.
 * The ZDP command frame is sent in the payload of an APS frame with the Profile ID (0x0000) and
 * the ZDP command frame is sent to the ZDP server on the destination device.
 */
enum ZdpClustersClientCmds : std::uint16_t
{
    NWK_ADDR_REQ = 0x0000,                //!< Network Address Request command (Optional)
    IEEE_ADDR_REQ = 0x0001,               //!< IEEE Address Request command (Optional)
    NODE_DESC_REQ = 0x0002,               //!< Node Descriptor Request command (Mandatory)
    POWER_DESC_REQ = 0x0003,              //!< Power Descriptor Request command (Optional)
    SIMPLE_DESC_REQ = 0x0004,             //!< Simple Descriptor Request command (Optional)
    ACTIVE_EP_REQ = 0x0005,               //!< Active Endpoint Request command (Optional)
    MATCH_DESC_REQ = 0x0006,              //!< Match Descriptor Request command (Optional)
    COMPLEX_DESC_REQ = 0x0010,            //!< Complex Descriptor Request command (Optional)
    USER_DESC_REQ = 0x0011,               //!< User Descriptor Request command (Optional)
    DISCOVERY_CACHE_REQ = 0x0012,         //!< Discovery Cache Request command (Optional)
    DEVICE_ANNCE = 0x0013,                //!< Device Announcement command (Optional)
    PARENT_ANNCE = 0x0014,                //!< Parent Announcement command (Mandatory)
    SYSTEM_SERVER_DISCOVERY_REQ = 0x0015, //!< System Server Discovery Request command (Optional)
    DISCOVERY_STORE_REQ = 0x0016,         //!< Discovery Store Request command (Optional)
    NODE_DESC_STORE_REQ = 0x0017,         //!< Node Descriptor Store Request command (Optional)
    POWER_DESC_STORE_REQ = 0x0018,        //!< Power Descriptor Store Request command (Optional)
    ACTIVE_EP_STORE_REQ = 0x0019,         //!< Active Endpoint Store Request command (Optional)
    SIMPLE_DESC_STORE_REQ = 0x001A,       //!< Simple Descriptor Store Request command (Optional)
    REMOVE_NODE_CACHE_REQ = 0x001B,       //!< Remove Node Cache Request command (Optional)
    FIND_NODE_CACHE_REQ = 0x001C,         //!< Find Node Cache Request command (Optional)
    EXTENDED_SIMPLE_DESC_REQ = 0x001D,    //!< Extended Simple Descriptor Request command (Optional)
    EXTENDED_ACTIVE_EP_REQ = 0x001E       //!< Extended Active Endpoint Request command (Optional)
};

/**
 *  Implements the NWK_addr_req command
 *  See Zigbee Specification r22.1.0, Section 2.4.3.1.1
 */
class NwkAddrRequestCmd : public Header
{
  public:
    NwkAddrRequestCmd();
    ~NwkAddrRequestCmd() override;

    /**
     * Get the type ID.
     *
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Set the transaction sequence number.
     *
     * @param tsn The transaction sequence number.
     */
    void SetTsn(uint8_t tsn);

    /**
     * Get the transaction sequence number.
     *
     * @return The transaction sequence number.
     */
    uint8_t GetTsn() const;

    /**
     * Set the IEEE address used as base for the network address request.
     *
     * @param ieeeAddress The IEEE address.
     */
    void SetIeeeAddress(Mac64Address ieeeAddress);

    /**
     * Get the IEEE address set for the network address request.
     *
     * @return The IEEE address.
     */
    Mac64Address GetIeeeAddress();

    /**
     * Set the request type, either single device or extended response.
     * Note:Extended response is not currently supported.
     *
     * @param type The request type.
     */
    void SetRequestType(uint8_t type);

    /**
     * Get the request type previously set for the network address request.
     *
     * @return The request type.
     */
    uint8_t GetRequestType() const;

    /**
     * Set the start index. Used only for the pagination of extended responses.
     * Note: Extended response is not currently supported. Therefore this is only
     * a placeholder for future use.
     *
     * @param index The start index.
     */
    void SetStartIndex(uint8_t index);

    /**
     * Get the start index.
     *
     * @return The start index.
     */
    uint8_t GetStartIndex() const;

    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_tsn{0};           //!< The transaction sequence number for the request.
    Mac64Address m_ieeeAddress; //!< The IEEE address of the device to be queried.
    uint8_t m_requestType{0};   //!< Single device or extended response type
    uint8_t m_startIndex{0};    //!< The starting index for the response, used for pagination.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_ZDO_CLIENT_HEADER_H */
