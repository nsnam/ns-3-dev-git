/*
 * Copyright (c) 2016
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "block-ack-type.h"
#include "wifi-types.h"

#include "ns3/fatal-error.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include <list>
#include <map>
#include <set>

namespace ns3
{

class Mac48Address;
class WifiMacHeader;
class Packet;
class WifiMac;

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
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param direction the direction
 * @returns a reference to the stream
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
 * @param val the value in dBm
 *
 * @return the equivalent Watts for the given dBm
 */
Watt_u DbmToW(dBm_u val);
/**
 * Convert from dB to ratio.
 *
 * @param val the value in dB
 *
 * @return ratio in linear scale
 */
double DbToRatio(dB_u val);
/**
 * Convert from Watts to dBm.
 *
 * @param val the value in Watts
 *
 * @return the equivalent dBm for the given Watts
 */
dBm_u WToDbm(Watt_u val);
/**
 * Convert from ratio to dB.
 *
 * @param ratio the ratio in linear scale
 *
 * @return the value in dB
 */
dB_u RatioToDb(double ratio);

/**
 * Convert from MHz to Hz.
 *
 * @param val the value in MHz
 *
 * @return the value in Hz
 */
inline Hz_u
MHzToHz(MHz_u val)
{
    return val * 1e6;
}

/**
 * Convert from Hz to MHz.
 *
 * @param val the value in Hz
 *
 * @return the value in MHz
 */
inline MHz_u
HzToMHz(Hz_u val)
{
    return val * 1e-6;
}

/**
 * Return the number of 20 MHz subchannels covering the channel width.
 *
 * @param channelWidth the channel width
 * @return the number of 20 MHz subchannels
 */
inline std::size_t
Count20MHzSubchannels(MHz_u channelWidth)
{
    NS_ASSERT(static_cast<uint16_t>(channelWidth) % 20 == 0);
    return channelWidth / MHz_u{20};
}

/**
 * Return the number of 20 MHz subchannels covering the channel width between a lower frequency and
 * an upper frequency. This function should only be called when the channel width between the lower
 * frequency and the upper frequency is a multiple of 20 MHz.
 *
 * @param lower the lower frequency
 * @param upper the upper frequency
 * @return the number of 20 MHz subchannels
 */
inline std::size_t
Count20MHzSubchannels(MHz_u lower, MHz_u upper)
{
    NS_ASSERT(upper >= lower);
    const auto width = upper - lower;
    NS_ASSERT((static_cast<uint16_t>(width) % 20 == 0));
    return Count20MHzSubchannels(width);
}

/**
 * Return the total Ack size (including FCS trailer).
 *
 * @return the total Ack size in bytes
 */
uint32_t GetAckSize();
/**
 * Return the total BlockAck size (including FCS trailer).
 *
 * @param type the BlockAck type
 * @return the total BlockAck size in bytes
 */
uint32_t GetBlockAckSize(BlockAckType type);
/**
 * Return the total BlockAckRequest size (including FCS trailer).
 *
 * @param type the BlockAckRequest type
 * @return the total BlockAckRequest size in bytes
 */
uint32_t GetBlockAckRequestSize(BlockAckReqType type);
/**
 * Return the total MU-BAR size (including FCS trailer).
 *
 * @param types the list of Block Ack Request types of the individual BARs
 * @return the total MU-BAR size in bytes
 */
uint32_t GetMuBarSize(std::list<BlockAckReqType> types);
/**
 * Return the total RTS size (including FCS trailer).
 *
 * @return the total RTS size in bytes
 */
uint32_t GetRtsSize();
/**
 * Return the total CTS size (including FCS trailer).
 *
 * @return the total CTS size in bytes
 */
uint32_t GetCtsSize();
/**
 * @param seq MPDU sequence number
 * @param winstart sequence number window start
 * @param winsize the size of the sequence number window
 * @returns true if in the window
 *
 * This method checks if the MPDU's sequence number is inside the scoreboard boundaries or not
 */
bool IsInWindow(uint16_t seq, uint16_t winstart, uint16_t winsize);
/**
 * Add FCS trailer to a packet.
 *
 * @param packet the packet to add a trailer to
 */
void AddWifiMacTrailer(Ptr<Packet> packet);
/**
 * Return the total size of the packet after WifiMacHeader and FCS trailer
 * have been added.
 *
 * @param packet the packet to be encapsulated with WifiMacHeader and FCS trailer
 * @param hdr the WifiMacHeader
 * @param isAmpdu whether packet is part of an A-MPDU
 * @return the total packet size
 */
uint32_t GetSize(Ptr<const Packet> packet, const WifiMacHeader* hdr, bool isAmpdu);

/**
 * Check if the given TID-to-Link Mappings are valid for a negotiation type of 1. Specifically,
 * it is checked whether all TIDs are mapped to the same set of links.
 *
 * @param dlLinkMapping the given TID-to-Link Mapping for Downlink
 * @param ulLinkMapping the given TID-to-Link Mapping for Uplink
 * @return whether the given TID-to-Link Mappings are valid for a negotiation type of 1
 */
bool TidToLinkMappingValidForNegType1(const WifiTidLinkMapping& dlLinkMapping,
                                      const WifiTidLinkMapping& ulLinkMapping);

/**
 * Check whether a MAC destination address corresponds to a groupcast transmission.
 *
 * @param adr the MAC address
 * @return true if the MAC address is a group address that is not a broadcast address
 */
bool IsGroupcast(const Mac48Address& adr);

/**
 * Return whether a given packet is transmitted using the GCR service.
 *
 * @param mac a pointer to the wifi MAC
 * @param hdr the MAC header of the packet to check
 * @return true if the packet is transmitted using the GCR service, false otherwise
 */
bool IsGcr(Ptr<WifiMac> mac, const WifiMacHeader& hdr);

/**
 * Get the MAC address of the individually addressed recipient to use for a given packet.
 * If this is a groupcast packet to be transmitted with the GCR service, the GCR manager is
 * requested to return which individually addressed recipient to use. Otherwise, it corresponds to
 * the address1 of the MAC header.
 *
 * @param mac a pointer to the wifi MAC
 * @param hdr the MAC header of the packet to check
 * @return the MAC address of the individually addressed recipient to use
 */
Mac48Address GetIndividuallyAddressedRecipient(Ptr<WifiMac> mac, const WifiMacHeader& hdr);

/// Size of the space of sequence numbers
static constexpr uint16_t SEQNO_SPACE_SIZE = 4096;

/// Size of the half the space of sequence numbers (used to determine old packets)
static constexpr uint16_t SEQNO_SPACE_HALF_SIZE = SEQNO_SPACE_SIZE / 2;

/// Link ID for single link operations (helps tracking places where correct link
/// ID is to be used to support multi-link operations)
static constexpr uint8_t SINGLE_LINK_OP_ID = 0;

/// Invalid link identifier
static constexpr uint8_t WIFI_LINKID_UNDEFINED = 0xff;

/// Invalid TID identifier
static constexpr uint8_t WIFI_TID_UNDEFINED = 0xff;

/// Wi-Fi Time Unit value in microseconds (see IEEE 802.11-2020 sec. 3.1)
/// Used to initialize WIFI_TU
constexpr int WIFI_TU_US = 1024;

/// Wi-Fi Time Unit (see IEEE 802.11-2020 sec. 3.1)
extern const Time WIFI_TU;

} // namespace ns3

#endif /* WIFI_UTILS_H */
