/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 *
 */

#include "zigbee-nwk-fields.h"

#include "ns3/log.h"

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeNwkFields");

CapabilityInformation::CapabilityInformation()
{
    m_allocateAddr = false;
    m_deviceType = ROUTER;
    m_powerSource = OTHER_POWER_SOURCE;
    m_receiverOnWhenIdle = true;
    m_securityCapability = false;
    m_allocateAddr = true;
}

CapabilityInformation::CapabilityInformation(uint8_t bitmap)
{
    SetCapability(bitmap);
}

uint8_t
CapabilityInformation::GetCapability() const
{
    uint8_t capability;

    capability = m_alternatePanCoord & (0x01);          // Bits 0
    capability |= ((m_deviceType & 0x01) << 1);         // Bit 1
    capability |= ((m_powerSource & 0x01) << 2);        // Bit 2
    capability |= ((m_receiverOnWhenIdle & 0x01) << 3); // Bit 3
                                                        // Bit 4-5 (Reserved)
    capability |= ((m_securityCapability & 0x01) << 6); // Bit 6
    capability |= ((m_allocateAddr & 0x01) << 7);       // Bit 7

    return capability;
}

bool
CapabilityInformation::IsAlternatePanCoord() const
{
    return m_alternatePanCoord;
}

MacDeviceType
CapabilityInformation::GetDeviceType() const
{
    return m_deviceType;
}

PowerSource
CapabilityInformation::GetPowerSource() const
{
    return m_powerSource;
}

bool
CapabilityInformation::IsReceiverOnWhenIdle() const
{
    return m_receiverOnWhenIdle;
}

bool
CapabilityInformation::IsAllocateAddrOn() const
{
    return m_allocateAddr;
}

void
CapabilityInformation::SetCapability(uint8_t capability)
{
    m_alternatePanCoord = (capability) & (0x01);                           // Bit 0
    m_deviceType = static_cast<MacDeviceType>((capability >> 1) & (0x01)); // Bit 1
    m_powerSource = static_cast<PowerSource>((capability >> 2) & (0x01));  // Bit 2
    m_receiverOnWhenIdle = (capability >> 3) & (0x01);                     // Bit 3
                                                                           // Bit 4-5 (Reserved)
    m_securityCapability = (capability >> 6) & (0x01);                     // Bit 6
    m_allocateAddr = (capability >> 7) & (0x01);                           // Bit 7
}

void
CapabilityInformation::SetDeviceType(MacDeviceType devType)
{
    m_deviceType = devType;
}

void
CapabilityInformation::SetPowerSource(PowerSource powerSource)
{
    m_powerSource = powerSource;
}

void
CapabilityInformation::SetReceiverOnWhenIdle(bool value)
{
    m_receiverOnWhenIdle = value;
}

void
CapabilityInformation::SetAllocateAddrOn(bool value)
{
    m_allocateAddr = value;
}

SuperframeInformation::SuperframeInformation()
{
}

SuperframeInformation::SuperframeInformation(uint16_t bitmap)
{
    SetSuperframe(bitmap);
}

void
SuperframeInformation::SetSuperframe(uint16_t superFrmSpec)
{
    m_sspecBcnOrder = (superFrmSpec) & (0x0F);         // Bits 0-3
    m_sspecSprFrmOrder = (superFrmSpec >> 4) & (0x0F); // Bits 4-7

    m_sspecPanCoor = (superFrmSpec >> 14) & (0x01);     // Bit 14
    m_sspecAssocPermit = (superFrmSpec >> 15) & (0x01); // Bit 15
}

uint8_t
SuperframeInformation::GetBeaconOrder() const
{
    return m_sspecBcnOrder;
}

uint8_t
SuperframeInformation::GetFrameOrder() const
{
    return m_sspecSprFrmOrder;
}

bool
SuperframeInformation::IsPanCoor() const
{
    return m_sspecPanCoor;
}

bool
SuperframeInformation::IsAssocPermit() const
{
    return m_sspecAssocPermit;
}

} // namespace zigbee
} // namespace ns3
