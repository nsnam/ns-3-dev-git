/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_TYPES_H
#define WIFI_TYPES_H

#include "ns3/si-units.h"

#include <compare>
#include <map>
#include <ostream>

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
    CW_320MHZ,
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

/// SignalNoise structure (internal API)
struct SignalNoise
{
    dBm_t signal; ///< signal strength
    dBm_t noise;  ///< noise power
};

/// SignalNoiseDbm structure (external API)
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
    dBm_t rssi; ///< RSSI
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

/**
 * Struct defining a frequency range between minFrequency (MHz) and maxFrequency (MHz).
 */
struct FrequencyRange
{
    MHz_t minFrequency{0.0}; ///< the minimum frequency
    MHz_t maxFrequency{0.0}; ///< the maximum frequency

    /**
     * @brief spaceship operator.
     *
     * @param range the frequency range
     * @returns -1 if the provided range is located at a lower minimum frequency, 0 if the provided
     * range is identical or 1 if the provided range is located at a higher minimum frequency
     */
    std::partial_ordering operator<=>(const FrequencyRange& range) const = default;
};

/**
 * Serialize FrequencyRange values to ostream (human-readable).
 *
 * @param os the output stream
 * @param freqRange the FrequencyRange
 *
 * @return std::ostream
 */
inline std::ostream&
operator<<(std::ostream& os, const FrequencyRange& freqRange)
{
    os << "[" << freqRange.minFrequency << " - " << freqRange.maxFrequency << "]";
    return os;
}

/// Identifier for the frequency range covering the whole wifi spectrum
constexpr FrequencyRange WHOLE_WIFI_SPECTRUM = {MHz_t{2401}, MHz_t{7125}};

/// Identifier for the frequency range covering the wifi spectrum in the 2.4 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_2_4_GHZ = {MHz_t{2401}, MHz_t{2483}};

/// Identifier for the frequency range covering the wifi spectrum in the 5 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_5_GHZ = {MHz_t{5170}, MHz_t{5915}};

/// Identifier for the frequency range covering the wifi spectrum in the 6 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_6_GHZ = {MHz_t{5945}, MHz_t{7125}};

/**
 * The different Resource Unit (RU) types.
 */
enum class RuType : uint8_t
{
    RU_26_TONE = 0,
    RU_52_TONE,
    RU_106_TONE,
    RU_242_TONE,
    RU_484_TONE,
    RU_996_TONE,
    RU_2x996_TONE,
    RU_TYPE_MAX
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param ruType the RU type
 * @returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const RuType& ruType)
{
    switch (ruType)
    {
    case RuType::RU_26_TONE:
        os << "26-tones";
        break;
    case RuType::RU_52_TONE:
        os << "52-tones";
        break;
    case RuType::RU_106_TONE:
        os << "106-tones";
        break;
    case RuType::RU_242_TONE:
        os << "242-tones";
        break;
    case RuType::RU_484_TONE:
        os << "484-tones";
        break;
    case RuType::RU_996_TONE:
        os << "996-tones";
        break;
    case RuType::RU_2x996_TONE:
        os << "2x996-tones";
        break;
    default:
        NS_FATAL_ERROR("Unknown RU type");
    }
    return os;
}

/// (lowest index, highest index) pair defining a subcarrier range
using SubcarrierRange = std::pair<int16_t, int16_t>;

/// a vector of subcarrier ranges defining a subcarrier group
using SubcarrierGroup = std::vector<SubcarrierRange>;

/// (bandwidth, number of tones) pair
using BwTonesPair = std::pair<MHz_t, RuType>;

/// map (bandwidth, number of tones) pairs to the group of subcarrier ranges
using SubcarrierGroups = std::map<BwTonesPair, std::vector<SubcarrierGroup>>;

} // namespace ns3

#endif /* WIFI_TYPES_H */
