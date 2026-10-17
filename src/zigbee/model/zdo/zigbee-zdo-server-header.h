/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_ZDO_SERVER_HEADER_H
#define ZIGBEE_ZDO_SERVER_HEADER_H

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
 * Zigbee Device Profile (ZDP) Server Services Commands
 * Zigbee Specification r22.1.0, Section 2.4.4.2, Table 2-91
 *
 * The numbers used by this profile represent the matching Cluster ID used when sending
 * data meant for the ZDO using the APS layer.
 * The ZDP command frame is sent in the payload of an APS frame with the Profile ID (0x0000) and
 * the ZDP command frame is sent to the ZDP client on the destination device.
 */
enum ZdpClustersServerCmds : std::uint16_t
{
    NWK_ADDR_RSP = 0x8000,                //!< Network Address Response command (Mandatory)
    IEEE_ADDR_RSP = 0x8001,               //!< IEEE Address Response command (Mandatory)
    NODE_DESC_RSP = 0x8002,               //!< Node Descriptor Response command (Mandatory)
    POWER_DESC_RSP = 0x8003,              //!< Power Descriptor Response command (Mandatory)
    SIMPLE_DESC_RSP = 0x8004,             //!< Simple Descriptor Response command (Mandatory)
    ACTIVE_EP_RSP = 0x8005,               //!< Active Endpoint Response command (Mandatory)
    MATCH_DESC_RSP = 0x8006,              //!< Match Descriptor Response command (Mandatory)
    COMPLEX_DESC_RSP = 0x8010,            //!< Complex Descriptor Response command (Optional)
    USER_DESC_RSP = 0x8011,               //!< User Descriptor Response command (Optional)
    USER_DESC_CONF = 0x8014,              //!< Discovery Cache Response command (Optional)
    PARENT_ANNCE_RSP = 0x801F,            //!< Parent Announcement Response command (Mandatory)
    SYSTEM_SERVER_DISCOVERY_RSP = 0x8015, //!< System Server Discovery Response command (Optional)
    DISCOVERY_STORE_RSP = 0x8016,         //!< Discovery Store Response command (Optional)
    NODE_DESC_STORE_RSP = 0x8017,         //!< Node Descriptor Store Response command (Optional)
    POWER_DESC_STORE_RSP = 0x8018,        //!< Power Descriptor Store Response command (Optional)
    ACTIVE_EP_STORE_RSP = 0x8019,         //!< Active Endpoint Store Response command (Optional)
    SIMPLE_DESC_STORE_RSP = 0x801A,       //!< Simple Descriptor Store Response command (Optional)
    REMOVE_NODE_CACHE_RSP = 0x801B,       //!< Remove Node Cache Response command (Optional)
    FIND_NODE_CACHE_RSP = 0x801C,         //!< Find Node Cache Response command (Optional)
    EXTENDED_SIMPLE_DESC_RSP = 0x801D, //!< Extended Simple Descriptor Response command (Optional)
    EXTENDED_ACTIVE_EP_RSP = 0x801E    //!< Extended Active Endpoint Response command (Optional)
};

/**
 * NWK_addr_rsp Command Frame
 * See Zigbee Specification  Section 2.4.4.2.1, Figure 2-64
 */
class NwkAddrResponseCmd : public Header
{
  public:
    NwkAddrResponseCmd();
    ~NwkAddrResponseCmd() override;

    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Set the transaction sequence number of the NWK_addr_rsp command.
     * This value must match the one used in the corresponding NWK_addr_req.
     *
     * @param tsn The transaction sequence number to set.
     */
    void SetTsn(uint8_t tsn);

    /**
     * Get the transaction sequence number of the NWK_addr_rsp command.
     *
     * @return The transaction sequence number.
     */
    uint8_t GetTsn() const;

    /**
     * Set the status of the NWK_addr_rsp command.
     *
     * @param status The status value to set.
     */
    void SetStatus(uint8_t status);

    /**
     * Get the status of the NWK_addr_rsp command.
     *
     * @return The status value.
     */
    uint8_t GetStatus() const;

    /**
     * Set the IEEE address of the remote device.
     *
     * @param ieeeAddrRemoteDev The IEEE address of the remote device.
     */
    void SetIeeeAddrRemoteDev(Mac64Address ieeeAddrRemoteDev);

    /**
     * Get the IEEE address of the remote device.
     *
     * @return The IEEE address of the remote device.
     */
    Mac64Address GetIeeeAddrRemoteDev();

    /**
     * Set the network address of the remote device.
     *
     * @param nwkAddrRemoteDev The network address of the remote device.
     */
    void SetNwkAddrRemoteDev(Mac16Address nwkAddrRemoteDev);

    /**
     * Get the network address of the remote device.
     *
     * @return The network address of the remote device.
     */
    Mac16Address GetNwkAddrRemoteDev();

    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_tsn{0};    //!< The transaction sequence number of the NWK_addr_rsp command.
    uint8_t m_status{0}; //!< The status of the NWK_addr_rsp command.
    Mac64Address m_ieeeAddrRemoteDev; //!< The IEEE address of the remote device.
    Mac16Address m_nwkAddrRemoteDev;  //!< The network address of the remote device.
    // TODO fields for the optional extended response such as the number of associated devices
    // and the list of associated devices need to be added here when the extended response is
    // implemented.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_ZDO_SERVER_HEADER_H */
