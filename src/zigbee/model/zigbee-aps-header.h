/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_APS_HEADER_H
#define ZIGBEE_APS_HEADER_H

#include "ns3/header.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"

namespace ns3
{
namespace zigbee
{

/**
 *  @ingroup zigbee
 *
 *  Values of the Frame Type Sub-Field.
 *  Zigbee Specification r22.1.0, Table 2-20
 */
enum ApsFrameType : uint8_t
{
    APS_DATA = 0x00,
    APS_COMMAND = 0x01,
    APS_ACK = 0x02,
    APS_INTERPAN_APS = 0x03
};

/**
 *  @ingroup zigbee
 *
 *  Values of the Delivery Mode Sub-Field.
 *  Zigbee Specification r22.1.0, Table 2-21
 */
enum ApsDeliveryMode : uint8_t
{
    APS_UCST = 0x00,
    APS_BCST = 0x02,
    APS_GROUP_ADDRESSING = 0x03
};

/**
 *  @ingroup zigbee
 *
 *  Table 2-22 Values of the Fragmentation Sub-Field
 *  Zigbee Specification r22.1.0, Table 2-22
 */
enum ApsFragmentation : uint8_t
{
    NOT_FRAGMENTED = 0x00,
    FIRST_FRAGMENT = 0x01,
    OTHER_FRAGMENT = 0x02
};

/**
 * @ingroup zigbee
 *
 * Defines the APS header use by data transfer and commands issued from
 * the APS layer. Zigbee Specification r22.1.0, Section 2.2.5.1.
 *
 */
class ZigbeeApsHeader : public Header
{
  public:
    ZigbeeApsHeader();
    ~ZigbeeApsHeader() override;

    /**
     * Set the Frame type defined in the APS layer
     *
     * @param frameType The frame type to set on the header
     */
    void SetFrameType(enum ApsFrameType frameType);

    /**
     * Get the frame time present in the header
     *
     * @return ApsFrameType
     */
    ApsFrameType GetFrameType() const;

    /**
     * Defines the mode in which this frame should be transmitted.
     *
     * @param mode The delivery mode to set on the APS header
     */
    void SetDeliveryMode(enum ApsDeliveryMode mode);

    /**
     * Get the transmission mode set on the header.
     *
     * @return ApsDeliveryMode The delivery mode of the header
     */
    ApsDeliveryMode GetDeliveryMode() const;

    /**
     * Set whether or not security should be used to transmit
     * the frame.
     *
     * @param enabled True if security should be used
     */
    void SetSecurity(bool enabled);

    /**
     * Returns whether or not security is enabled for the present frame.alignas
     *
     * @return True if security is enabled for the frame.
     */
    bool IsSecurityEnabled() const;

    /**
     * Set the acknowledment flag in the APS header.
     *
     * @param request True if the frame should be acknowledged
     */
    void SetAckRequest(bool request);

    /**
     * Indicates if the present frame requires acknowledgment.
     *
     * @return True if the frame requires acknowledgment
     */
    bool IsAckRequested() const;

    /**
     * Enables or disables the usage of the extended header.
     * Only used when fragmentation is used.
     *
     * @param present True if the extended header (fragmentation) is present.
     */
    void SetExtHeaderPresent(bool present);

    /**
     * Indicates whether or not the extended header is present in the frame.
     * @return True if the extended header is present.
     */
    bool IsExtHeaderPresent() const;

    /**
     * Set the Bitmap representing the framecontrol portion of the APS header.
     *
     * @param frameControl The bitmap representing the framecontrol portion.
     */
    void SetFrameControl(uint8_t frameControl);

    /**
     * Get the frame control portion (bitmap) of the APS header.
     *
     * @return The frame control portion of the APS header.
     */
    uint8_t GetFrameControl() const;

    /**
     * Set the destination endpoint in the APS header.
     *
     * @param endpoint The destination endpoint.
     */
    void SetDstEndpoint(uint8_t endpoint);

    /**
     * Get the destination endpoint in the APS header.
     *
     * @return The destination endpoint.
     */
    uint8_t GetDstEndpoint() const;

    /**
     * Set the group address in the APS header.
     *
     * @param groupAddress The group address to set.
     */
    void SetGroupAddress(uint16_t groupAddress);

    /**
     * Get the group address in the APS header.
     *
     * @return The group address.
     */
    uint16_t GetGroupAddress() const;

    /**
     * Set the cluster id in the APS header.
     *
     * @param clusterId The cluster id
     */
    void SetClusterId(uint16_t clusterId);

    /**
     * Get the cluster id in the APS header.
     * @return The cluster id.
     */
    uint16_t GetClusterId() const;

    /**
     * Set the profile ID in the APS header.
     *
     * @param profileId The profile ID.
     */
    void SetProfileId(uint16_t profileId);

    /**
     * Get the profile ID in the APS header.
     * @return The profile ID.
     */
    uint16_t GetProfileId() const;

    /**
     * Set the source endpoint in the APS header.
     *
     * @param endpoint The source endpoint.
     */
    void SetSrcEndpoint(uint8_t endpoint);

    /**
     * Get the source endpoint in the APS header.
     * @return The source endpoint.
     */
    uint8_t GetSrcEndpoint() const;

    /**
     * Set the value of the APS counter in the APS header.
     * @param counter The APS counter value.
     */
    void SetApsCounter(uint8_t counter);

    /**
     * Get the APS counter value present in the APS header.
     * @return The APS counter value.
     */
    uint8_t GetApsCounter() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

  private:
    // Frame Control field bits
    ApsFrameType m_frameType;       //!< Frame control field: Frame type
    ApsDeliveryMode m_deliveryMode; //!< Frame control field: Delivery mode
    bool m_ackFormat;               //!< Frame control field: Acknowledgment format
    bool m_security;                //!< Frame control field: Security
    bool m_ackRequest;              //!< Frame control field: Acknowledge requested
    bool m_extHeaderPresent;        //!< Frame control field: Extended header present

    // Addressing fields
    uint8_t m_dstEndpoint;   //!< Addressing field: Destination endpoint.
    uint16_t m_groupAddress; //!< Addressing field: Group or 16-bit address.
    uint16_t m_clusterId;    //!< Addressing field: Cluster ID.
    uint16_t m_profileId;    //!< Addressing field: Profile ID.
    uint8_t m_srcEndpoint;   //!< Addressing field: Source endpoint.

    uint8_t m_apsCounter; //!< APS counter field

    // Extended header fields
    ApsFragmentation m_fragmentation; //!< Extended header field: Fragmentation block type
    uint8_t m_blockNumber;            //!< Extended header field: Block number
    uint8_t m_ackBitfield;            //!< Extended header field: Acknowledgement bit field
};

} // namespace zigbee
} // namespace ns3

#endif // ZIGBEE_APS_HEADER_H
