/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Ryo Okuda  <c611901200@tokushima-u.ac.jp>
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_NWK_HEADER_H
#define ZIGBEE_NWK_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"

namespace ns3
{

namespace zigbee
{

/**
 * Zigbee Specification r22.1.0, Values of the frame Type Sub-field (Table 3-46)
 */
enum NwkType
{
    DATA = 0,        //!< Data frame type
    NWK_COMMAND = 1, //!< Network command frame type
    INTER_PAN = 3    //!< Inter-Pan frame type
};

/**
 * Zigbee Specification r22.1.0, Values of the discover route sub-field (Table 3-47)
 */
enum DiscoverRouteType
{
    SUPPRESS_ROUTE_DISCOVERY = 0x00, //!< Suppress route discovery
    ENABLE_ROUTE_DISCOVERY = 0x01    //!< Enable route discovery
};

/**
 * Zigbee Specification r22.1.0, Multicast Mode Sub-Field (Table 3-7)
 */
enum MulticastMode
{
    NONMEMBER = 0, //!< Non member multicast mode
    MEMBER = 1,    //!< Member multicast mode
};

/**
 * @ingroup zigbee
 * Represent the NWK Header with the Frame Control and Routing fields
 * Zigbee Specification r22.1.0, General NPDU Frame Format (Section 3.3.1)
 */
class ZigbeeNwkHeader : public Header
{
  public:
    ZigbeeNwkHeader();

    ~ZigbeeNwkHeader() override;

    /**
     * Set the Frame type used (Data or Command)
     * @param nwkType The frame type
     */
    void SetFrameType(enum NwkType nwkType);

    /**
     * Set the Frame Control field "Frame Type" bits
     * @return The frame type
     */
    NwkType GetFrameType() const;

    /**
     * Set the Protocol version
     * @param ver The protocol version
     */
    void SetProtocolVer(uint8_t ver);

    /**
     * Get Frame protocol version
     * @return The protocol version used by the frame
     */
    uint8_t GetProtocolVer() const;

    /**
     *  Suppress or enable route discovery for this frame
     * @param discoverRoute Suppress or enable route discovery
     */
    void SetDiscoverRoute(enum DiscoverRouteType discoverRoute);

    /**
     * Get the status of frame discovery route (suppress or enabled)
     * @return The discovery route status
     */
    DiscoverRouteType GetDiscoverRoute() const;

    /**
     * Set to true the value of the multicast flag in the frame control field.
     */
    void SetMulticast();

    /**
     * Inform whether or not the current frame is used in multicast.
     * @return  True = MCST false = UCST or BCST
     */
    bool IsMulticast() const;

    /**
     * Inform whether or not security is enabled.
     * @return True security enabled.
     */
    bool IsSecurityEnabled() const;

    /**
     * This flag is used by the NWK to indicate if the the initiator device of the
     * message is an end device and the nwkParentInformation field of the NIB has other value than
     * 0.
     */
    void SetEndDeviceInitiator();

    /**
     * Get whether or not the source of this message was an end device.
     * @return True of the source of the message was an end device.
     */
    bool GetEndDeviceInitiator() const;

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
     * Set Source address
     * @param addr The source address (16 bit)
     */
    void SetSrcAddr(Mac16Address addr);

    /**
     * Get the Source address
     * @return the Source address (16bits)
     */
    Mac16Address GetSrcAddr() const;

    /**
     * Set the Radius
     * @param radius radius
     */
    void SetRadius(uint8_t radius);

    /**
     * Get the Radius
     * @return the Radius
     */
    uint8_t GetRadius() const;

    /**
     * Set the Sequence number
     * @param seqNum sequence number
     */
    void SetSeqNum(uint8_t seqNum);

    /**
     * Get the frame Sequence number
     * @return the sequence number
     */
    uint8_t GetSeqNum() const;

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

    /**
     * Set the source IEEE address
     * @param src The source IEEE address
     */
    void SetSrcIeeeAddr(Mac64Address src);

    /**
     * Get the source IEEE address
     * @return The source IEEE address (64bits)
     */
    Mac64Address GetSrcIeeeAddr() const;

    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;

  private:
    /**
     * Set the Frame control field
     * @param frameControl 16 bits representing the frame control
     */
    void SetFrameControl(uint16_t frameControl);
    /**
     * Get the Frame control field
     * @return The 16 bits of the Frame control field
     */
    uint16_t GetFrameControl() const;
    /**
     * Set the Multicast control field
     * @param multicastControl 8 bits representing the multicast control field
     */
    void SetMulticastControl(uint8_t multicastControl);
    /**
     * Get The Multicast control field
     * @return The 8 bits representing the multicast control field
     */
    uint8_t GetMulticastControl() const;

    /* Frame Control (2 Octets) */

    NwkType m_fctrlFrmType;             //!< Frame type (Bit 0-1)
    uint8_t m_fctrlPrtVer;              //!< Protocol version (Bit 2-5)
    DiscoverRouteType m_fctrlDscvRoute; //!< Discover route (Bit 6-7)
    bool m_fctrlMcst;                   //!< Multicast flag (Bit 8)
    bool m_fctrlSecurity;               //!< Security flag (Bit 9)
    bool m_fctrlSrcRoute;               //!< Source route flag (Bit 10)
    bool m_fctrlDstIEEEAddr;            //!< Destination IEEE address flag (Bit 11)
    bool m_fctrlSrcIEEEAddr;            //!< Source IEEE address flag (Bit 12)
    bool m_fctrlEndDevIni;              //!< End device initiator flag (Bit 13)
                                        // Reserved (Bits 14-15)

    /* Routing Fields (variable) */

    Mac16Address m_dstAddr;     //!< Destination address (2 Octets)
    Mac16Address m_srcAddr;     //!< Source address (2 Octets)
    uint8_t m_radius;           //!< Radius (1 Octet)
    uint8_t m_seqNum;           //!< Sequence number (1 Octet)
    Mac64Address m_dstIeeeAddr; //!< Destination IEEE address (0 or 8 Octets)
    Mac64Address m_srcIeeeAddr; //!< Source IEEE address (0 or 8 Octets)
    // Multicast Control (1 Octet)
    MulticastMode m_mcstMode{NONMEMBER}; //!< Multicast mode sub-field (Bits 0-1)
    uint8_t m_nonMemberRadius;           //!< Non member radius sub-field (Bits 2-4)
    uint8_t m_maxNonMemberRadius;        //!< Max non member radius sub-field (Bit 5-7)

    // TODO: Add source route subframe
    // uint8_t m_relayCount; //!< Relay Count Sub-Field
    // uint8_t m_relayIndex; //!< Relay Index
    // TODO: Add Relay List when supported
};
} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_NWK_HEADER_H */
