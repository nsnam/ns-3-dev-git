/*
 * Copyright (c) 2020 Ritsumeikan University, Shiga, Japan.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
 */

#include "lr-wpan-mac-pl-headers.h"

#include <ns3/address-utils.h>
#include <ns3/simulator.h>

namespace ns3
{

/***********************************************************
 *                Beacon MAC Payload
 ***********************************************************/

BeaconPayloadHeader::BeaconPayloadHeader()
{
}

NS_OBJECT_ENSURE_REGISTERED(BeaconPayloadHeader);

TypeId
BeaconPayloadHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BeaconPayloadHeader")
                            .SetParent<Header>()
                            .SetGroupName("LrWpan")
                            .AddConstructor<BeaconPayloadHeader>();
    return tid;
}

TypeId
BeaconPayloadHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
BeaconPayloadHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += m_superframeField.GetSerializedSize();
    size += m_gtsFields.GetSerializedSize();
    size += m_pndAddrFields.GetSerializedSize();

    return size;
}

void
BeaconPayloadHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_superframeField.Serialize(i);
    i = m_gtsFields.Serialize(i);
    i = m_pndAddrFields.Serialize(i);
}

uint32_t
BeaconPayloadHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_superframeField.Deserialize(i);
    i = m_gtsFields.Deserialize(i);
    i = m_pndAddrFields.Deserialize(i);

    return i.GetDistanceFrom(start);
}

void
BeaconPayloadHeader::Print(std::ostream& os) const
{
    os << "| Superframe Spec Field | = " << m_superframeField
       << "| GTS Spec Field | = " << m_gtsFields.GetGtsSpecField()
       << "| Pending Spec Field| =" << m_pndAddrFields.GetPndAddrSpecField();
}

void
BeaconPayloadHeader::SetSuperframeSpecField(SuperframeField sf)
{
    m_superframeField = sf;
}

void
BeaconPayloadHeader::SetGtsFields(GtsFields gtsFields)
{
    m_gtsFields = gtsFields;
}

void
BeaconPayloadHeader::SetPndAddrFields(PendingAddrFields pndAddrFields)
{
    m_pndAddrFields = pndAddrFields;
}

SuperframeField
BeaconPayloadHeader::GetSuperframeSpecField() const
{
    return m_superframeField;
}

GtsFields
BeaconPayloadHeader::GetGtsFields() const
{
    return m_gtsFields;
}

PendingAddrFields
BeaconPayloadHeader::GetPndAddrFields() const
{
    return m_pndAddrFields;
}

/***********************************************************
 *                Command MAC Payload
 ***********************************************************/

CommandPayloadHeader::CommandPayloadHeader()
{
    SetCommandFrameType(CMD_RESERVED);
}

CommandPayloadHeader::CommandPayloadHeader(MacCommand macCmd)
{
    SetCommandFrameType(macCmd);
}

NS_OBJECT_ENSURE_REGISTERED(CommandPayloadHeader);

TypeId
CommandPayloadHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CommandPayloadHeader")
                            .SetParent<Header>()
                            .SetGroupName("LrWpan")
                            .AddConstructor<CommandPayloadHeader>();
    return tid;
}

TypeId
CommandPayloadHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CommandPayloadHeader::GetSerializedSize() const
{
    uint32_t size = 1;
    // TODO: add missing serialize commands size when other commands are added.
    switch (m_cmdFrameId)
    {
    case ASSOCIATION_REQ:
        size += m_capabilityInfo.GetSerializedSize();
        break;
    case ASSOCIATION_RESP:
        size += 3; // (short address + Association Status)
        break;
    case DISASSOCIATION_NOTIF:
        break;
    case DATA_REQ:
        break;
    case PANID_CONFLICT:
        break;
    case ORPHAN_NOTIF:
        break;
    case BEACON_REQ:
        break;
    case COOR_REALIGN:
        size += 8;
        break;
    case GTS_REQ:
        break;
    case CMD_RESERVED:
        break;
    }
    return size;
}

void
CommandPayloadHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_cmdFrameId);
    // TODO: add missing serialize commands when other commands are added.
    switch (m_cmdFrameId)
    {
    case ASSOCIATION_REQ:
        i = m_capabilityInfo.Serialize(i);
        break;
    case ASSOCIATION_RESP:
        WriteTo(i, m_shortAddr);
        i.WriteU8(m_assocStatus);
        break;
    case DISASSOCIATION_NOTIF:
        break;
    case DATA_REQ:
        break;
    case PANID_CONFLICT:
        break;
    case ORPHAN_NOTIF:
        break;
    case BEACON_REQ:
        break;
    case COOR_REALIGN:
        i.WriteU16(m_panid);
        WriteTo(i, m_coordShortAddr);
        i.WriteU8(m_logCh);
        WriteTo(i, m_shortAddr);
        i.WriteU8(m_logChPage);
        break;
    case GTS_REQ:
        break;
    case CMD_RESERVED:
        break;
    }
}

uint32_t
CommandPayloadHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_cmdFrameId = static_cast<MacCommand>(i.ReadU8());
    // TODO: add missing deserialize commands when other commands are added.
    switch (m_cmdFrameId)
    {
    case ASSOCIATION_REQ:
        i = m_capabilityInfo.Deserialize(i);
        break;
    case ASSOCIATION_RESP:
        ReadFrom(i, m_shortAddr);
        m_assocStatus = static_cast<AssocStatus>(i.ReadU8());
        break;
    case DISASSOCIATION_NOTIF:
        break;
    case DATA_REQ:
        break;
    case PANID_CONFLICT:
        break;
    case ORPHAN_NOTIF:
        break;
    case BEACON_REQ:
        break;
    case COOR_REALIGN:
        m_panid = i.ReadU16();
        ReadFrom(i, m_coordShortAddr);
        m_logCh = i.ReadU8();
        ReadFrom(i, m_shortAddr);
        m_logChPage = i.ReadU8();
        break;
    case GTS_REQ:
        break;
    case CMD_RESERVED:
        break;
    }

    return i.GetDistanceFrom(start);
}

void
CommandPayloadHeader::Print(std::ostream& os) const
{
    os << "| MAC Command Frame ID | = " << static_cast<uint32_t>(m_cmdFrameId);
    switch (m_cmdFrameId)
    {
    case ASSOCIATION_REQ:
        os << "| Device Type FFD | = " << m_capabilityInfo.IsDeviceTypeFfd()
           << "| Alternative Power Source available | = " << m_capabilityInfo.IsPowSrcAvailable()
           << "| Receiver on when Idle | = " << m_capabilityInfo.IsReceiverOnWhenIdle()
           << "| Security capable | = " << m_capabilityInfo.IsSecurityCapability()
           << "| Allocate address on | = " << m_capabilityInfo.IsShortAddrAllocOn();
        break;
    case ASSOCIATION_RESP:
        os << "| Assigned Short Address | = " << m_shortAddr
           << "| Status Response | = " << m_assocStatus;
        break;
    case DISASSOCIATION_NOTIF:
        break;
    case DATA_REQ:
        break;
    case PANID_CONFLICT:
        break;
    case ORPHAN_NOTIF:
        break;
    case BEACON_REQ:
        break;
    case COOR_REALIGN:
        os << "| PAN identifier| = " << m_panid
           << "| PAN Coord Short address| = " << m_coordShortAddr
           << "| Channel Num.| = " << static_cast<uint32_t>(m_logCh)
           << "| Short address| = " << m_shortAddr
           << "| Page Num.| = " << static_cast<uint32_t>(m_logChPage);
        break;
    case GTS_REQ:
        break;
    case CMD_RESERVED:
        break;
    }
}

void
CommandPayloadHeader::SetCommandFrameType(MacCommand macCommand)
{
    m_cmdFrameId = macCommand;
}

void
CommandPayloadHeader::SetCapabilityField(CapabilityField cap)
{
    NS_ASSERT(m_cmdFrameId == ASSOCIATION_REQ);
    m_capabilityInfo = cap;
}

void
CommandPayloadHeader::SetCoordShortAddr(Mac16Address addr)
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    m_coordShortAddr = addr;
}

void
CommandPayloadHeader::SetChannel(uint8_t channel)
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    m_logCh = channel;
}

void
CommandPayloadHeader::SetPage(uint8_t page)
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    m_logChPage = page;
}

void
CommandPayloadHeader::SetPanId(uint16_t id)
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    m_panid = id;
}

CommandPayloadHeader::MacCommand
CommandPayloadHeader::GetCommandFrameType() const
{
    switch (m_cmdFrameId)
    {
    case 0x01:
        return ASSOCIATION_REQ;
    case 0x02:
        return ASSOCIATION_RESP;
    case 0x03:
        return DISASSOCIATION_NOTIF;
    case 0x04:
        return DATA_REQ;
    case 0x05:
        return PANID_CONFLICT;
    case 0x06:
        return ORPHAN_NOTIF;
    case 0x07:
        return BEACON_REQ;
    case 0x08:
        return COOR_REALIGN;
    case 0x09:
        return GTS_REQ;
    default:
        return CMD_RESERVED;
    }
}

void
CommandPayloadHeader::SetShortAddr(Mac16Address shortAddr)
{
    NS_ASSERT(m_cmdFrameId == ASSOCIATION_RESP || m_cmdFrameId == COOR_REALIGN);
    m_shortAddr = shortAddr;
}

void
CommandPayloadHeader::SetAssociationStatus(AssocStatus status)
{
    NS_ASSERT(m_cmdFrameId == ASSOCIATION_RESP);
    m_assocStatus = status;
}

Mac16Address
CommandPayloadHeader::GetShortAddr() const
{
    return m_shortAddr;
}

CommandPayloadHeader::AssocStatus
CommandPayloadHeader::GetAssociationStatus() const
{
    NS_ASSERT(m_cmdFrameId == ASSOCIATION_RESP);
    return m_assocStatus;
}

CapabilityField
CommandPayloadHeader::GetCapabilityField() const
{
    NS_ASSERT(m_cmdFrameId == ASSOCIATION_REQ);
    return m_capabilityInfo;
}

Mac16Address
CommandPayloadHeader::GetCoordShortAddr() const
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    return m_coordShortAddr;
}

uint8_t
CommandPayloadHeader::GetChannel() const
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    return m_logCh;
}

uint8_t
CommandPayloadHeader::GetPage() const
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    return m_logChPage;
}

uint16_t
CommandPayloadHeader::GetPanId() const
{
    NS_ASSERT(m_cmdFrameId == COOR_REALIGN);
    return m_panid;
}

} // namespace ns3
