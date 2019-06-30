/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
#include "wifi-preamble.h"
#include "wifi-mode.h"

namespace ns3 {

class WifiNetDevice;
class WifiMacHeader;
class WifiMode;
class Packet;
class Time;

/**
 * Convert from dBm to Watts.
 *
 * \param dbm the power in dBm
 *
 * \return the equivalent Watts for the given dBm
 */
double DbmToW (double dbm);
/**
 * Convert from dB to ratio.
 *
 * \param db the value in dB
 *
 * \return ratio in linear scale
 */
double DbToRatio (double db);
/**
 * Convert from Watts to dBm.
 *
 * \param w the power in Watts
 *
 * \return the equivalent dBm for the given Watts
 */
double WToDbm (double w);
/**
 * Convert from ratio to dB.
 *
 * \param ratio the ratio in linear scale
 *
 * \return the value in dB
 */
double RatioToDb (double ratio);
/**
 * Convert the guard interval to nanoseconds based on the WifiMode.
 *
 * \param mode the WifiMode
 * \param device pointer to the WifiNetDevice object
 *
 * \return the guard interval duration in nanoseconds
 */
uint16_t ConvertGuardIntervalToNanoSeconds (WifiMode mode, const Ptr<WifiNetDevice> device);
/**
 * Convert the guard interval to nanoseconds based on the WifiMode.
 *
 * \param mode the WifiMode
 * \param htShortGuardInterval whether HT/VHT short guard interval is enabled
 * \param heGuardInterval the HE guard interval duration
 *
 * \return the guard interval duration in nanoseconds
 */
uint16_t ConvertGuardIntervalToNanoSeconds (WifiMode mode, bool htShortGuardInterval, Time heGuardInterval);
/**
 * Return the preamble to be used for the transmission.
 *
 * \param modulation the modulation selected for the transmission
 * \param useShortPreamble whether short preamble should be used
 * \param useGreenfield whether HT Greenfield should be used
 *
 * \return the preamble to be used for the transmission
 */
WifiPreamble GetPreambleForTransmission (WifiModulationClass modulation, bool useShortPreamble, bool useGreenfield);
/**
 * Return the channel width that corresponds to the selected mode (instead of
 * letting the PHY's default channel width). This is especially useful when using
 * non-HT modes with HT/VHT/HE capable stations (with default width above 20 MHz).
 *
 * \param mode selected WifiMode
 * \param maxSupportedChannelWidth maximum channel width supported by the PHY layer
 * \return channel width adapted to the selected mode
 */
uint16_t GetChannelWidthForTransmission (WifiMode mode, uint16_t maxSupportedChannelWidth);
/**
 * Return whether the modulation class of the selected mode for the
 * control answer frame is allowed.
 *
 * \param modClassReq modulation class of the request frame
 * \param modClassAnswer modulation class of the answer frame
 *
 * \return true if the modulation class of the selected mode for the
 * control answer frame is allowed, false otherwise
 */
bool IsAllowedControlAnswerModulationClass (WifiModulationClass modClassReq, WifiModulationClass modClassAnswer);
/**
 * Return the total Ack size (including FCS trailer).
 *
 * \return the total Ack size in bytes
 */
uint32_t GetAckSize (void);
/**
 * Return the total BlockAck size (including FCS trailer).
 *
 * \param type the BlockAck type
 * \return the total BlockAck size in bytes
 */
uint32_t GetBlockAckSize (BlockAckType type);
/**
 * Return the total BlockAckRequest size (including FCS trailer).
 *
 * \param type the BlockAckRequest type
 * \return the total BlockAckRequest size in bytes
 */
uint32_t GetBlockAckRequestSize (BlockAckType type);
/**
 * Return the total RTS size (including FCS trailer).
 *
 * \return the total RTS size in bytes
 */
uint32_t GetRtsSize (void);
/**
 * Return the total CTS size (including FCS trailer).
 *
 * \return the total CTS size in bytes
 */
uint32_t GetCtsSize (void);
/**
 * \param seq MPDU sequence number
 * \param winstart sequence number window start
 * \param winsize the size of the sequence number window
 * \returns true if in the window
 *
 * This method checks if the MPDU's sequence number is inside the scoreboard boundaries or not
 */
bool IsInWindow (uint16_t seq, uint16_t winstart, uint16_t winsize);
/**
 * Add FCS trailer to a packet.
 *
 * \param packet the packet to add a trailer to
 */
void AddWifiMacTrailer (Ptr<Packet> packet);
/**
 * Return the total size of the packet after WifiMacHeader and FCS trailer
 * have been added.
 *
 * \param packet the packet to be encapsulated with WifiMacHeader and FCS trailer
 * \param hdr the WifiMacHeader
 * \param isAmpdu whether packet is part of an A-MPDU
 * \return the total packet size
 */
uint32_t GetSize (Ptr<const Packet> packet, const WifiMacHeader *hdr, bool isAmpdu);
/**
 * Get the maximum PPDU duration (see Section 10.14 of 802.11-2016) for
 * the PHY layers defining the aPPDUMaxTime characteristic (HT, VHT and HE).
 * Return zero otherwise.
 *
 * \param preamble the preamble type
 *
 * \return the maximum PPDU duration, if defined, and zero otherwise
 */
Time GetPpduMaxTime (WifiPreamble preamble);

/**
 * Return whether the preamble is a HT format preamble.
 *
 * \param preamble the preamble type
 *
 * \return true if the preamble is a HT format preamble,
 *         false otherwise
 */
bool IsHt (WifiPreamble preamble);
/**
 * Return whether the preamble is a VHT format preamble.
 *
 * \param preamble the preamble type
 *
 * \return true if the preamble is a VHT format preamble,
 *         false otherwise
 */
bool IsVht (WifiPreamble preamble);
/**
 * Return whether the preamble is a HE format preamble.
 *
 * \param preamble the preamble type
 *
 * \return true if the preamble is a HE format preamble,
 *         false otherwise
 */
bool IsHe (WifiPreamble preamble);

/// Size of the space of sequence numbers
const uint16_t SEQNO_SPACE_SIZE = 4096;

/// Size of the half the space of sequence numbers (used to determine old packets)
const uint16_t SEQNO_SPACE_HALF_SIZE = SEQNO_SPACE_SIZE / 2;
} // namespace ns3

#endif /* WIFI_UTILS_H */
