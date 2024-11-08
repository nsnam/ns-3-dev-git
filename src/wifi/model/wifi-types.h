/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_TYPES_H
#define WIFI_TYPES_H

#include "wifi-units.h"

namespace ns3
{

/**
 * @ingroup wifi
 * Enumeration of the possible channel widths
 */
enum class WifiChannelWidthType : uint8_t
{
    UNKNOWN = 0,
    CW_20MHZ,
    CW_22MHZ,
    CW_5MHZ,
    CW_10MHZ,
    CW_40MHZ,
    CW_80MHZ,
    CW_160MHZ,
    CW_80_PLUS_80MHZ,
    CW_2160MHZ,
    MAX,
};

/**
 * @ingroup wifi
 * The type of an MPDU.
 */
enum MpduType
{
    /** The MPDU is not part of an A-MPDU */
    NORMAL_MPDU,
    /** The MPDU is a single MPDU */
    SINGLE_MPDU,
    /** The MPDU is the first aggregate in an A-MPDU with multiple MPDUs, but is not the last
     * aggregate */
    FIRST_MPDU_IN_AGGREGATE,
    /** The MPDU is part of an A-MPDU with multiple MPDUs, but is neither the first nor the last
     * aggregate */
    MIDDLE_MPDU_IN_AGGREGATE,
    /** The MPDU is the last aggregate in an A-MPDU with multiple MPDUs */
    LAST_MPDU_IN_AGGREGATE
};

/// SignalNoiseDbm structure
struct SignalNoiseDbm
{
    dBm_u signal; ///< signal strength
    dBm_u noise;  ///< noise power
};

/// MpduInfo structure
struct MpduInfo
{
    MpduType type;          ///< type of MPDU
    uint32_t mpduRefNumber; ///< MPDU ref number
};

/// RxSignalInfo structure containing info on the received signal
struct RxSignalInfo
{
    double snr; ///< SNR in linear scale
    dBm_u rssi; ///< RSSI
};

/**
 * @ingroup wifi
 * @brief Enumeration of frequency channel types
 */
enum class FrequencyChannelType : uint8_t
{
    DSSS = 0,
    OFDM,
    CH_80211P
};

} // namespace ns3

#endif /* WIFI_TYPES_H */
