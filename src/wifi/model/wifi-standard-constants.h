/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_STANDARD_CONSTANTS_H
#define WIFI_STANDARD_CONSTANTS_H

#include "ns3/nstime.h"
#include "ns3/wifi-export.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of the constants used across wifi module
 */

namespace ns3
{

/// Wi-Fi Time Unit value in microseconds (see IEEE 802.11-2020 sec. 3.1)
/// Used to initialize WIFI_TU
WIFI_EXPORT extern const Time WIFI_TU;

/// aRxPHYStartDelay value to use when waiting for a new frame in the context of EMLSR operations
/// (Sec. 35.3.17 of 802.11be D3.1)
extern const Time EMLSR_RX_PHY_START_DELAY;

/// aSIFSTime value for OFDM (20 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SIFS_TIME_20MHZ;

/// aSlotTime duration for OFDM (20 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SLOT_TIME_20MHZ;

/// aSIFSTime value for OFDM (10 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SIFS_TIME_10MHZ;

/// aSlotTime duration for OFDM (10 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SLOT_TIME_10MHZ;

/// aSIFSTime value for OFDM (5 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SIFS_TIME_5MHZ;

/// aSlotTime duration for OFDM (5 MHz channel spacing)
/// (Table 17-21 "OFDM PHY characteristics" of 802.11-2020)
extern const Time OFDM_SLOT_TIME_5MHZ;

/// aSIFSTime duration for DSSS (Table 16-4 "HR/DSSS PHY characteristics" of 802.11-2020)
extern const Time DSSS_SIFS_TIME;

/// aSlotTime duration for DSSS (Table 16-4 "HR/DSSS PHY characteristics" of 802.11-2020)
extern const Time DSSS_SLOT_TIME;

/// maximum propagation delay
WIFI_EXPORT extern const Time MAX_PROPAGATION_DELAY;

/// The aMediumSyncThreshold defined by Sec. 35.3.16.18.1 of 802.11be D4.0
WIFI_EXPORT extern const Time MEDIUM_SYNC_THRESHOLD;

/// Subcarrier frequency spacing in Hz (Table 19-6 "Timing-related constants" of 802.11-2020)
static constexpr uint32_t SUBCARRIER_FREQUENCY_SPACING{312500};

/// Subcarrier frequency spacing for the HE modulated fields in Hz (Table 27-12 "Timing-related
/// constants" of 802.11ax-2021)
static constexpr uint32_t SUBCARRIER_FREQUENCY_SPACING_HE{78125};

/// Size of the space of sequence numbers
static constexpr uint16_t SEQNO_SPACE_SIZE{4096};

/// Size of the half the space of sequence numbers (used to determine old packets)
static constexpr uint16_t SEQNO_SPACE_HALF_SIZE{SEQNO_SPACE_SIZE / 2};

/// This value conforms to the 802.11 specification
static constexpr uint16_t MAX_MSDU_SIZE{2304};

/// The length in octets of the IEEE 802.11 MAC FCS field
static constexpr uint16_t WIFI_MAC_FCS_LENGTH{4};

/// The minimum value for dot11RTSThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_MIN_RTS_THRESHOLD{0};

/// The maximum value for dot11RTSThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_MAX_RTS_THRESHOLD{4692480};

/// The default value for dot11RTSThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_DEFAULT_RTS_THRESHOLD{WIFI_MAX_RTS_THRESHOLD};

/// The minimum value for dot11FragmentationThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_MIN_FRAG_THRESHOLD{256};

/// The maximum value for dot11FragmentationThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_MAX_FRAG_THRESHOLD{65535};

/// The default value for dot11FragmentationThreshold (C.3 MIB detail in IEEE Std 802.11-2020)
static constexpr uint32_t WIFI_DEFAULT_FRAG_THRESHOLD{WIFI_MAX_FRAG_THRESHOLD};

/// STA_ID to identify a single user (SU)
static constexpr uint16_t SU_STA_ID{65535};

/// Empty 242-tone RU identifier for HE (Section 26.11.7 802.11ax-2021)
static constexpr uint8_t EMPTY_242_TONE_HE_RU{113};

/// The minimum value for the association ID (Sec. 9.4.1.8 of 802.11-2020)
static constexpr uint16_t MIN_AID{1};

/// The maximum value for the association ID (Sec. 9.4.1.8 of 802.11-2020)
static constexpr uint16_t MAX_AID{2007};

/// STA_ID for a RU that is intended for no user (Section 26.11.1 802.11ax-2021)
static constexpr uint16_t NO_USER_STA_ID{2046};

/// Unassigned 242-tone RU identifier for EHT (Table Table 36-34 IEEE 802.11be D7.0)
static constexpr uint8_t UNASSIGNED_242_TONE_EHT_RU{27};

/// The maximum value for the association ID updated since 802.11be (Sec. 9.4.1.8 of 802.11be D7.0)
static constexpr uint16_t EHT_MAX_AID{2006};

/// AID value for Special User Info field in trigger frames (Sec. 9.3.1.22.3 of 802.11be D7.0)
static constexpr uint16_t AID_SPECIAL_USER{2007};

} // namespace ns3

#endif /* WIFI_STANDARD_CONSTANTS_H */
