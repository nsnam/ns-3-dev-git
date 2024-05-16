/*
 * Copyright (c) 2016
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "block-ack-type.h"

#include "ns3/fatal-error.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include <list>
#include <map>
#include <set>

namespace ns3
{

class WifiMacHeader;
class Packet;

/**
 * Wifi direction. Values are those defined for the TID-to-Link Mapping Control Direction
 * field in IEEE 802.11be D3.1 Figure 9-1002ap
 */
enum class WifiDirection : uint8_t
{
    DOWNLINK = 0,
    UPLINK = 1,
    BOTH_DIRECTIONS = 2,
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param direction the direction
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiDirection& direction)
{
    switch (direction)
    {
    case WifiDirection::DOWNLINK:
        return (os << "DOWNLINK");
    case WifiDirection::UPLINK:
        return (os << "UPLINK");
    case WifiDirection::BOTH_DIRECTIONS:
        return (os << "BOTH_DIRECTIONS");
    default:
        NS_FATAL_ERROR("Invalid direction");
        return (os << "INVALID");
    }
}

/// @brief TID-indexed map of the link set to which the TID is mapped
using WifiTidLinkMapping = std::map<uint8_t, std::set<uint8_t>>;

/**
 * Convert from dBm to Watts.
 *
 * \param dbm the power in dBm
 *
 * \return the equivalent Watts for the given dBm
 */
double DbmToW(double dbm);
/**
 * Convert from dB to ratio.
 *
 * \param db the value in dB
 *
 * \return ratio in linear scale
 */
double DbToRatio(double db);
/**
 * Convert from Watts to dBm.
 *
 * \param w the power in Watts
 *
 * \return the equivalent dBm for the given Watts
 */
double WToDbm(double w);
/**
 * Convert from ratio to dB.
 *
 * \param ratio the ratio in linear scale
 *
 * \return the value in dB
 */
double RatioToDb(double ratio);
/**
 * Return the total Ack size (including FCS trailer).
 *
 * \return the total Ack size in bytes
 */
uint32_t GetAckSize();
/**
 * Return the total BlockAck size (including FCS trailer).
 *
 * \param type the BlockAck type
 * \return the total BlockAck size in bytes
 */
uint32_t GetBlockAckSize(BlockAckType type);
/**
 * Return the total BlockAckRequest size (including FCS trailer).
 *
 * \param type the BlockAckRequest type
 * \return the total BlockAckRequest size in bytes
 */
uint32_t GetBlockAckRequestSize(BlockAckReqType type);
/**
 * Return the total MU-BAR size (including FCS trailer).
 *
 * \param types the list of Block Ack Request types of the individual BARs
 * \return the total MU-BAR size in bytes
 */
uint32_t GetMuBarSize(std::list<BlockAckReqType> types);
/**
 * Return the total RTS size (including FCS trailer).
 *
 * \return the total RTS size in bytes
 */
uint32_t GetRtsSize();
/**
 * Return the total CTS size (including FCS trailer).
 *
 * \return the total CTS size in bytes
 */
uint32_t GetCtsSize();
/**
 * \param seq MPDU sequence number
 * \param winstart sequence number window start
 * \param winsize the size of the sequence number window
 * \returns true if in the window
 *
 * This method checks if the MPDU's sequence number is inside the scoreboard boundaries or not
 */
bool IsInWindow(uint16_t seq, uint16_t winstart, uint16_t winsize);
/**
 * Add FCS trailer to a packet.
 *
 * \param packet the packet to add a trailer to
 */
void AddWifiMacTrailer(Ptr<Packet> packet);
/**
 * Return the total size of the packet after WifiMacHeader and FCS trailer
 * have been added.
 *
 * \param packet the packet to be encapsulated with WifiMacHeader and FCS trailer
 * \param hdr the WifiMacHeader
 * \param isAmpdu whether packet is part of an A-MPDU
 * \return the total packet size
 */
uint32_t GetSize(Ptr<const Packet> packet, const WifiMacHeader* hdr, bool isAmpdu);

/**
 * Check if the given TID-to-Link Mappings are valid for a negotiation type of 1. Specifically,
 * it is checked whether all TIDs are mapped to the same set of links.
 *
 * \param dlLinkMapping the given TID-to-Link Mapping for Downlink
 * \param ulLinkMapping the given TID-to-Link Mapping for Uplink
 * \return whether the given TID-to-Link Mappings are valid for a negotiation type of 1
 */
bool TidToLinkMappingValidForNegType1(const WifiTidLinkMapping& dlLinkMapping,
                                      const WifiTidLinkMapping& ulLinkMapping);

/// Size of the space of sequence numbers
static constexpr uint16_t SEQNO_SPACE_SIZE = 4096;

/// Size of the half the space of sequence numbers (used to determine old packets)
static constexpr uint16_t SEQNO_SPACE_HALF_SIZE = SEQNO_SPACE_SIZE / 2;

/// Link ID for single link operations (helps tracking places where correct link
/// ID is to be used to support multi-link operations)
static constexpr uint8_t SINGLE_LINK_OP_ID = 0;

/// Invalid link identifier
static constexpr uint8_t WIFI_LINKID_UNDEFINED = 0xff;

/// Wi-Fi Time Unit value in microseconds (see IEEE 802.11-2020 sec. 3.1)
/// Used to initialize WIFI_TU
constexpr int WIFI_TU_US = 1024;

/// Wi-Fi Time Unit (see IEEE 802.11-2020 sec. 3.1)
extern const Time WIFI_TU;

} // namespace ns3

#endif /* WIFI_UTILS_H */
