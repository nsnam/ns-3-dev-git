/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 *
 */

#ifndef ZIGBEE_NWK_FIELDS_H
#define ZIGBEE_NWK_FIELDS_H

#include "ns3/buffer.h"

#include <array>

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup zigbee
 * The device Type
 * Zigbee Specification r22.1.0 (Table 3-62 or Table 3-63)
 */
enum MacDeviceType
{
    ENDDEVICE = 0, //!< End device or router treated as an end device.
    ROUTER = 1     //!< Router device.
};

/**
 * @ingroup zigbee
 * The power source capabilities.
 * Zigbee Specification r22.1.0 (Table 3-62)
 */
enum PowerSource
{
    OTHER_POWER_SOURCE = 0, //!< Other power source.
    MAINPOWER = 1           //!< Mains-powered device.
};

/**
 * @ingroup zigbee
 * Requested Timeout Field
 * See Zigbee Specification r22.1.0, 3.4.11.3.1
 *
 * List the requested timeout values in minutes
 */
static const double RequestedTimeoutField
    [15]{0.166667, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};

/**
 * @ingroup zigbee
 * Represent the the Capability Information Bit fields
 * See zigbe Specification r22.1.0, Table 3-62
 */
class CapabilityInformation
{
  public:
    CapabilityInformation();

    /**
     * Constructor using the capability in a bitmap form.
     *
     * @param bitmap The bitmap representing the capability.
     */
    CapabilityInformation(uint8_t bitmap);

    /**
     * Used to obtain the complete capability information bit map.
     *
     * @return The bit map with the capability information
     */
    uint8_t GetCapability() const;

    /**
     * This field will always have a value of false in implementations of
     * this specification.
     *
     * @return false for implementations of this specification.
     */
    bool IsAlternatePanCoord() const;

    /**
     * This field will have a value of ROUTER if the joining device is a
     * Zigbee router. It will have a value of ENDDEVICE if the devices is a
     * Zigbee end device or else a router-capable device that is joining  as
     * an end device.
     *
     * @return The device type used
     */
    MacDeviceType GetDeviceType() const;

    /**
     * This field will be set to the value of lowest-order bit of the
     * PowerSource parameter passed to the NLME-JOIN.request primitive.
     *
     * @return MAINPOWER or OTHER_POWER_SOURCE
     */
    PowerSource GetPowerSource() const;

    /**
     * This field will be set to the value of the lowest-order bit of the
     * RxOnWhenIdle parameter passed  to the NLME-JOIN.request primitive
     *
     * @return True Rx enabled when the device is idle | False = the receiver is disabled when idle.
     *
     */
    bool IsReceiverOnWhenIdle() const;

    /**
     * This field will have a value of true in implementations of this specification,
     * indicating that the joining device must be issued a 16 bit network address,
     * except in the case where a device has self-selected its address while using the
     * NWK rejoin command to join a network for the first time in a secure manner.
     * In this case, it shall have a value of false.
     *
     * @return True = The device require a 16 bit address allocation, False = otherwise.
     */
    bool IsAllocateAddrOn() const;

    /**
     * Set the Capability Information bit map
     *
     * @param capability The 8 bit map representing the full capability
     */
    void SetCapability(uint8_t capability);

    /**
     * Set the device type bit for the capability information field.
     *
     * @param devType The device type field to set in the capability information.
     */
    void SetDeviceType(MacDeviceType devType);

    /**
     * Set the power source bit for the capability information field.
     *
     * @param powerSource The power source field to set in the capability information.
     */
    void SetPowerSource(PowerSource powerSource);

    /**
     * Set the Receiver On When Idle bit for the capability information field.
     *
     * @param value True if the receiver should remain on when idle.
     */
    void SetReceiverOnWhenIdle(bool value);

    /**
     * Set the Allocate Addr On for the capability information field.
     *
     * @param value True if the device requires to have its 16 bit network address allocated.
     */
    void SetAllocateAddrOn(bool value);

  private:
    bool m_alternatePanCoord{false};      //!< (Bit 0) The alternate PAN coordinator bit field.
    MacDeviceType m_deviceType{ROUTER};   //!< (Bit 1) The device type bit field.
    PowerSource m_powerSource{MAINPOWER}; //!< (Bit 2) The power source  bits field.
    bool m_receiverOnWhenIdle{true};      //!< (Bit 3) The receiver on when idle bit field.
    bool m_securityCapability{false};     //!< (Bit 6) The security capability bit field.
    bool m_allocateAddr{true};            //!< (Bit 7) The allocate address bit field.
};

/**
 * @ingroup lr-wpan
 * Represent the Superframe Specification information field.
 * See IEEE 802.15.4-2011   Section 5.2.2.1.2 Figure 41
 */
class SuperframeInformation
{
  public:
    SuperframeInformation();

    /**
     * Create a superframe Specification Information field with
     * the information specified in the bitmap.
     *
     * @param bitmap The superframe in bitmap form
     */
    SuperframeInformation(uint16_t bitmap);

    /**
     * Set the whole Superframe Specification Information field.
     * @param superFrm The Superframe Specification information field.
     */
    void SetSuperframe(uint16_t superFrm);

    /**
     * Get the Superframe Specification Beacon Order field.
     * @return the Superframe Specification Beacon Order field.
     */
    uint8_t GetBeaconOrder() const;

    /**
     * Get the Superframe Specification Frame Order field.
     * @return The Superframe Specification Frame Order field.
     */
    uint8_t GetFrameOrder() const;

    /**
     * Check if the PAN Coordinator bit is enabled.
     * @returns true if the PAN Coordinator bit is enabled
     */
    bool IsPanCoor() const;

    /**
     * Check if the Association Permit bit is enabled.
     * @returns true if the Association Permit bit is enabled
     */
    bool IsAssocPermit() const;

  private:
    // Superframe Specification field
    // See IEEE 802.14.15-2011 5.2.2.1.2
    uint8_t m_sspecBcnOrder;    //!< Superframe Specification field Beacon Order (Bit 0-3)
    uint8_t m_sspecSprFrmOrder; //!< Superframe Specification field Superframe Order (Bit 4-7)

    bool m_sspecPanCoor;     //!< Superframe Specification field PAN Coordinator (Bit 14)
    bool m_sspecAssocPermit; //!< Superframe Specification field Association Permit (Bit 15)
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_NWK_FIELDS_H */
