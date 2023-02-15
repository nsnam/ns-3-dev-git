/*
 * Copyright (c) 2005,2006,2007 INRIA
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Rediet <getachew.redieteab@orange.com>
 */

#ifndef WIFI_PHY_COMMON_H
#define WIFI_PHY_COMMON_H

#include "wifi-standards.h"

#include "ns3/fatal-error.h"
#include "ns3/ptr.h"
#include "ns3/wifi-spectrum-value-helper.h"

#include <ostream>

/**
 * \file
 * \ingroup wifi
 * Declaration of the following enums:
 * - ns3::WifiPreamble
 * - ns3::WifiModulationClass
 * - ns3::WifiPpduField
 * - ns3::WifiPpduType
 * - ns3::WifiPhyRxfailureReason
 */

namespace ns3
{

class WifiNetDevice;
class WifiMode;
class Time;

/**
 * typedef for a pair of start and stop frequencies in Hz to represent a band
 */
using WifiSpectrumBandFrequencies = std::pair<uint64_t, uint64_t>;

/// WifiSpectrumBandInfo structure containing info about a spectrum band
struct WifiSpectrumBandInfo
{
    WifiSpectrumBandIndices indices;         //!< the start and stop indices of the band
    WifiSpectrumBandFrequencies frequencies; //!< the start and stop frequencies of the band
};

/**
 * \ingroup wifi
 * Compare two bands.
 *
 * \param lhs the band on the left of operator<
 * \param rhs the band on the right of operator<
 * \return true if the start/stop frequencies of left are lower than the start/stop frequencies of
 * right, false otherwise
 */
inline bool
operator<(const WifiSpectrumBandInfo& lhs, const WifiSpectrumBandInfo& rhs)
{
    return lhs.frequencies < rhs.frequencies;
}

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param band the band
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiSpectrumBandInfo& band)
{
    os << "indices: [" << band.indices.first << "-" << band.indices.second << "], frequencies: ["
       << band.frequencies.first << "Hz-" << band.frequencies.second << "Hz]";
    return os;
}

/**
 * These constants define the various convolutional coding rates
 * used for the OFDM transmission modes in the IEEE 802.11
 * standard. DSSS (for example) rates which do not have an explicit
 * coding stage in their generation should have this parameter set to
 * WIFI_CODE_RATE_UNDEFINED.
 */
enum WifiCodeRate : uint16_t
{
    WIFI_CODE_RATE_UNDEFINED, //!< undefined coding rate
    WIFI_CODE_RATE_1_2,       //!< 1/2 coding rate
    WIFI_CODE_RATE_2_3,       //!< 2/3 coding rate
    WIFI_CODE_RATE_3_4,       //!< 3/4 coding rate
    WIFI_CODE_RATE_5_6,       //!< 5/6 coding rate
    WIFI_CODE_RATE_5_8,       //!< 5/8 coding rate
    WIFI_CODE_RATE_13_16,     //!< 13/16 coding rate
    WIFI_CODE_RATE_1_4,       //!< 1/4 coding rate
    WIFI_CODE_RATE_13_28,     //!< 13/28 coding rate
    WIFI_CODE_RATE_13_21,     //!< 13/21 coding rate
    WIFI_CODE_RATE_52_63,     //!< 52/63 coding rate
    WIFI_CODE_RATE_13_14,     //!< 13/14 coding rate
    WIFI_CODE_RATE_7_8,       //!< 7/8 coding rate
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param codeRate the code rate
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiCodeRate& codeRate)
{
    switch (codeRate)
    {
    case WIFI_CODE_RATE_UNDEFINED:
        return (os << "Code rate undefined");
    case WIFI_CODE_RATE_1_2:
        return (os << "Code rate 1/2");
    case WIFI_CODE_RATE_2_3:
        return (os << "Code rate 2/3");
    case WIFI_CODE_RATE_3_4:
        return (os << "Code rate 3/4");
    case WIFI_CODE_RATE_5_6:
        return (os << "Code rate 5/6");
    case WIFI_CODE_RATE_5_8:
        return (os << "Code rate 5/8");
    case WIFI_CODE_RATE_13_16:
        return (os << "Code rate 13/16");
    case WIFI_CODE_RATE_1_4:
        return (os << "Code rate 1/4");
    case WIFI_CODE_RATE_13_28:
        return (os << "Code rate 13/28");
    case WIFI_CODE_RATE_13_21:
        return (os << "Code rate 13/21");
    case WIFI_CODE_RATE_52_63:
        return (os << "Code rate 52/63");
    case WIFI_CODE_RATE_13_14:
        return (os << "Code rate 13/14");
    case WIFI_CODE_RATE_7_8:
        return (os << "Code rate 7/8");
    default:
        NS_FATAL_ERROR("Unknown code rate");
        return (os << "Unknown");
    }
}

/**
 * \ingroup wifi
 * The type of preamble to be used by an IEEE 802.11 transmission
 */
enum WifiPreamble
{
    WIFI_PREAMBLE_LONG,
    WIFI_PREAMBLE_SHORT,
    WIFI_PREAMBLE_HT_MF,
    WIFI_PREAMBLE_VHT_SU,
    WIFI_PREAMBLE_VHT_MU,
    WIFI_PREAMBLE_DMG_CTRL,
    WIFI_PREAMBLE_DMG_SC,
    WIFI_PREAMBLE_DMG_OFDM,
    WIFI_PREAMBLE_HE_SU,
    WIFI_PREAMBLE_HE_ER_SU,
    WIFI_PREAMBLE_HE_MU,
    WIFI_PREAMBLE_HE_TB,
    WIFI_PREAMBLE_EHT_MU,
    WIFI_PREAMBLE_EHT_TB
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param preamble the preamble
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiPreamble& preamble)
{
    switch (preamble)
    {
    case WIFI_PREAMBLE_LONG:
        return (os << "LONG");
    case WIFI_PREAMBLE_SHORT:
        return (os << "SHORT");
    case WIFI_PREAMBLE_HT_MF:
        return (os << "HT_MF");
    case WIFI_PREAMBLE_VHT_SU:
        return (os << "VHT_SU");
    case WIFI_PREAMBLE_VHT_MU:
        return (os << "VHT_MU");
    case WIFI_PREAMBLE_DMG_CTRL:
        return (os << "DMG_CTRL");
    case WIFI_PREAMBLE_DMG_SC:
        return (os << "DMG_SC");
    case WIFI_PREAMBLE_DMG_OFDM:
        return (os << "DMG_OFDM");
    case WIFI_PREAMBLE_HE_SU:
        return (os << "HE_SU");
    case WIFI_PREAMBLE_HE_ER_SU:
        return (os << "HE_ER_SU");
    case WIFI_PREAMBLE_HE_MU:
        return (os << "HE_MU");
    case WIFI_PREAMBLE_HE_TB:
        return (os << "HE_TB");
    case WIFI_PREAMBLE_EHT_MU:
        return (os << "EHT_MU");
    case WIFI_PREAMBLE_EHT_TB:
        return (os << "EHT_TB");
    default:
        NS_FATAL_ERROR("Invalid preamble");
        return (os << "INVALID");
    }
}

/**
 * \ingroup wifi
 * This enumeration defines the modulation classes per
 * (Table 10-6 "Modulation classes"; IEEE 802.11-2016, with
 * updated in 802.11ax/D6.0 as Table 10-9).
 */
enum WifiModulationClass
{
    /** Modulation class unknown or unspecified. A WifiMode with this
    WifiModulationClass has not been properly initialized. */
    WIFI_MOD_CLASS_UNKNOWN = 0,
    WIFI_MOD_CLASS_DSSS,      //!< DSSS (Clause 15)
    WIFI_MOD_CLASS_HR_DSSS,   //!< HR/DSSS (Clause 16)
    WIFI_MOD_CLASS_ERP_OFDM,  //!< ERP-OFDM (18.4)
    WIFI_MOD_CLASS_OFDM,      //!< OFDM (Clause 17)
    WIFI_MOD_CLASS_HT,        //!< HT (Clause 19)
    WIFI_MOD_CLASS_VHT,       //!< VHT (Clause 22)
    WIFI_MOD_CLASS_DMG_CTRL,  //!< DMG (Clause 21)
    WIFI_MOD_CLASS_DMG_OFDM,  //!< DMG (Clause 21)
    WIFI_MOD_CLASS_DMG_SC,    //!< DMG (Clause 21)
    WIFI_MOD_CLASS_DMG_LP_SC, //!< DMG (Clause 21)
    WIFI_MOD_CLASS_HE,        //!< HE (Clause 27)
    WIFI_MOD_CLASS_EHT        //!< EHT (Clause 36)
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param modulation the WifiModulationClass
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiModulationClass& modulation)
{
    switch (modulation)
    {
    case WIFI_MOD_CLASS_DSSS:
        return (os << "DSSS");
    case WIFI_MOD_CLASS_HR_DSSS:
        return (os << "HR/DSSS");
    case WIFI_MOD_CLASS_ERP_OFDM:
        return (os << "ERP-OFDM");
    case WIFI_MOD_CLASS_OFDM:
        return (os << "OFDM");
    case WIFI_MOD_CLASS_HT:
        return (os << "HT");
    case WIFI_MOD_CLASS_VHT:
        return (os << "VHT");
    case WIFI_MOD_CLASS_DMG_CTRL:
        return (os << "DMG_CTRL");
    case WIFI_MOD_CLASS_DMG_OFDM:
        return (os << "DMG_OFDM");
    case WIFI_MOD_CLASS_DMG_SC:
        return (os << "DMG_SC");
    case WIFI_MOD_CLASS_DMG_LP_SC:
        return (os << "DMG_LP_SC");
    case WIFI_MOD_CLASS_HE:
        return (os << "HE");
    case WIFI_MOD_CLASS_EHT:
        return (os << "EHT");
    default:
        NS_FATAL_ERROR("Unknown modulation");
        return (os << "unknown");
    }
}

/**
 * \ingroup wifi
 * The type of PPDU field (grouped for convenience)
 */
enum WifiPpduField
{
    /**
     * SYNC + SFD fields for DSSS or ERP,
     * shortSYNC + shortSFD fields for HR/DSSS or ERP,
     * HT-GF-STF + HT-GF-LTF1 fields for HT-GF,
     * L-STF + L-LTF fields otherwise.
     */
    WIFI_PPDU_FIELD_PREAMBLE = 0,
    /**
     * PHY header field for DSSS or ERP,
     * short PHY header field for HR/DSSS or ERP,
     * field not present for HT-GF,
     * L-SIG field or L-SIG + RL-SIG fields otherwise.
     */
    WIFI_PPDU_FIELD_NON_HT_HEADER,
    WIFI_PPDU_FIELD_HT_SIG,   //!< HT-SIG field
    WIFI_PPDU_FIELD_TRAINING, //!< STF + LTF fields (excluding those in preamble for HT-GF)
    WIFI_PPDU_FIELD_SIG_A,    //!< SIG-A field
    WIFI_PPDU_FIELD_SIG_B,    //!< SIG-B field
    WIFI_PPDU_FIELD_U_SIG,    //!< U-SIG field
    WIFI_PPDU_FIELD_EHT_SIG,  //!< EHT-SIG field
    WIFI_PPDU_FIELD_DATA      //!< data field
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param field the PPDU field
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiPpduField& field)
{
    switch (field)
    {
    case WIFI_PPDU_FIELD_PREAMBLE:
        return (os << "preamble");
    case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return (os << "non-HT header");
    case WIFI_PPDU_FIELD_HT_SIG:
        return (os << "HT-SIG");
    case WIFI_PPDU_FIELD_TRAINING:
        return (os << "training");
    case WIFI_PPDU_FIELD_SIG_A:
        return (os << "SIG-A");
    case WIFI_PPDU_FIELD_SIG_B:
        return (os << "SIG-B");
    case WIFI_PPDU_FIELD_U_SIG:
        return (os << "U-SIG");
    case WIFI_PPDU_FIELD_EHT_SIG:
        return (os << "EHT-SIG");
    case WIFI_PPDU_FIELD_DATA:
        return (os << "data");
    default:
        NS_FATAL_ERROR("Unknown field");
        return (os << "unknown");
    }
}

/**
 * \ingroup wifi
 * The type of PPDU (SU, DL MU, or UL MU)
 */
enum WifiPpduType
{
    WIFI_PPDU_TYPE_SU = 0,
    WIFI_PPDU_TYPE_DL_MU,
    WIFI_PPDU_TYPE_UL_MU
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param type the PPDU type
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiPpduType& type)
{
    switch (type)
    {
    case WIFI_PPDU_TYPE_SU:
        return (os << "SU");
    case WIFI_PPDU_TYPE_DL_MU:
        return (os << "DL MU");
    case WIFI_PPDU_TYPE_UL_MU:
        return (os << "UL MU");
    default:
        NS_FATAL_ERROR("Unknown type");
        return (os << "unknown");
    }
}

/**
 * \ingroup wifi
 * Enumeration of the possible reception failure reasons.
 */
enum WifiPhyRxfailureReason
{
    UNKNOWN = 0,
    UNSUPPORTED_SETTINGS,
    CHANNEL_SWITCHING,
    RXING,
    TXING,
    SLEEPING,
    POWERED_OFF,
    TRUNCATED_TX,
    BUSY_DECODING_PREAMBLE,
    PREAMBLE_DETECT_FAILURE,
    RECEPTION_ABORTED_BY_TX,
    L_SIG_FAILURE,
    HT_SIG_FAILURE,
    SIG_A_FAILURE,
    SIG_B_FAILURE,
    U_SIG_FAILURE,
    EHT_SIG_FAILURE,
    PREAMBLE_DETECTION_PACKET_SWITCH,
    FRAME_CAPTURE_PACKET_SWITCH,
    OBSS_PD_CCA_RESET,
    PPDU_TOO_LATE,
    FILTERED,
    DMG_HEADER_FAILURE,
    DMG_ALLOCATION_ENDED
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param reason the failure reason
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, const WifiPhyRxfailureReason& reason)
{
    switch (reason)
    {
    case UNSUPPORTED_SETTINGS:
        return (os << "UNSUPPORTED_SETTINGS");
    case CHANNEL_SWITCHING:
        return (os << "CHANNEL_SWITCHING");
    case RXING:
        return (os << "RXING");
    case TXING:
        return (os << "TXING");
    case SLEEPING:
        return (os << "SLEEPING");
    case POWERED_OFF:
        return (os << "OFF");
    case TRUNCATED_TX:
        return (os << "TRUNCATED_TX");
    case BUSY_DECODING_PREAMBLE:
        return (os << "BUSY_DECODING_PREAMBLE");
    case PREAMBLE_DETECT_FAILURE:
        return (os << "PREAMBLE_DETECT_FAILURE");
    case RECEPTION_ABORTED_BY_TX:
        return (os << "RECEPTION_ABORTED_BY_TX");
    case L_SIG_FAILURE:
        return (os << "L_SIG_FAILURE");
    case HT_SIG_FAILURE:
        return (os << "HT_SIG_FAILURE");
    case SIG_A_FAILURE:
        return (os << "SIG_A_FAILURE");
    case SIG_B_FAILURE:
        return (os << "SIG_B_FAILURE");
    case U_SIG_FAILURE:
        return (os << "U_SIG_FAILURE");
    case EHT_SIG_FAILURE:
        return (os << "EHT_SIG_FAILURE");
    case PREAMBLE_DETECTION_PACKET_SWITCH:
        return (os << "PREAMBLE_DETECTION_PACKET_SWITCH");
    case FRAME_CAPTURE_PACKET_SWITCH:
        return (os << "FRAME_CAPTURE_PACKET_SWITCH");
    case OBSS_PD_CCA_RESET:
        return (os << "OBSS_PD_CCA_RESET");
    case PPDU_TOO_LATE:
        return (os << "PPDU_TOO_LATE");
    case FILTERED:
        return (os << "FILTERED");
    case DMG_HEADER_FAILURE:
        return (os << "DMG_HEADER_FAILURE");
    case DMG_ALLOCATION_ENDED:
        return (os << "DMG_ALLOCATION_ENDED");
    case UNKNOWN:
    default:
        NS_FATAL_ERROR("Unknown reason");
        return (os << "UNKNOWN");
    }
}

/**
 * \ingroup wifi
 * Enumeration of the possible channel-list parameter elements
 * defined in Table 8-5 of IEEE 802.11-2016.
 */
enum WifiChannelListType : uint8_t
{
    WIFI_CHANLIST_PRIMARY = 0,
    WIFI_CHANLIST_SECONDARY,
    WIFI_CHANLIST_SECONDARY40,
    WIFI_CHANLIST_SECONDARY80
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param type the wifi channel list type
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiChannelListType type)
{
    switch (type)
    {
    case WIFI_CHANLIST_PRIMARY:
        return (os << "PRIMARY");
    case WIFI_CHANLIST_SECONDARY:
        return (os << "SECONDARY");
    case WIFI_CHANLIST_SECONDARY40:
        return (os << "SECONDARY40");
    case WIFI_CHANLIST_SECONDARY80:
        return (os << "SECONDARY80");
    default:
        NS_FATAL_ERROR("Unknown wifi channel type");
        return (os << "UNKNOWN");
    }
}

/**
 * Convert the guard interval to nanoseconds based on the WifiMode.
 *
 * \param mode the WifiMode
 * \param device pointer to the WifiNetDevice object
 *
 * \return the guard interval duration in nanoseconds
 */
uint16_t ConvertGuardIntervalToNanoSeconds(WifiMode mode, const Ptr<WifiNetDevice> device);

/**
 * Convert the guard interval to nanoseconds based on the WifiMode.
 *
 * \param mode the WifiMode
 * \param htShortGuardInterval whether HT/VHT short guard interval is enabled
 * \param heGuardInterval the HE guard interval duration
 *
 * \return the guard interval duration in nanoseconds
 */
uint16_t ConvertGuardIntervalToNanoSeconds(WifiMode mode,
                                           bool htShortGuardInterval,
                                           Time heGuardInterval);

/**
 * Return the preamble to be used for the transmission.
 *
 * \param modulation the modulation selected for the transmission
 * \param useShortPreamble whether short preamble should be used
 *
 * \return the preamble to be used for the transmission
 */
WifiPreamble GetPreambleForTransmission(WifiModulationClass modulation, bool useShortPreamble);

/**
 * Return the modulation class corresponding to the given preamble type.
 * Only preamble types used by HT/VHT/HE/EHT can be passed to this function.
 *
 * \param preamble the given preamble type (must be one defined by HT standard or later)
 * \return the modulation class corresponding to the given preamble type
 */
WifiModulationClass GetModulationClassForPreamble(WifiPreamble preamble);

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
bool IsAllowedControlAnswerModulationClass(WifiModulationClass modClassReq,
                                           WifiModulationClass modClassAnswer);

/**
 * Get the maximum PPDU duration (see Section 10.14 of 802.11-2016) for
 * the PHY layers defining the aPPDUMaxTime characteristic (HT, VHT and HE).
 * Return zero otherwise.
 *
 * \param preamble the preamble type
 *
 * \return the maximum PPDU duration, if defined, and zero otherwise
 */
Time GetPpduMaxTime(WifiPreamble preamble);

/**
 * Return true if a preamble corresponds to a multi-user transmission.
 *
 * \param preamble the preamble
 * \return true if the provided preamble corresponds to a multi-user transmission
 */
bool IsMu(WifiPreamble preamble);

/**
 * Return true if a preamble corresponds to a downlink multi-user transmission.
 *
 * \param preamble the preamble
 * \return true if the provided preamble corresponds to a downlink multi-user transmission
 */
bool IsDlMu(WifiPreamble preamble);

/**
 * Return true if a preamble corresponds to a uplink multi-user transmission.
 *
 * \param preamble the preamble
 * \return true if the provided preamble corresponds to a uplink multi-user transmission
 */
bool IsUlMu(WifiPreamble preamble);

/**
 * Return the modulation class corresponding to a given standard.
 *
 * \param standard the standard
 * \return the modulation class corresponding to the standard
 */
WifiModulationClass GetModulationClassForStandard(WifiStandard standard);

/**
 * Get the maximum channel width in MHz allowed for the given modulation class.
 *
 * \param modulation the modulation class
 * \return the maximum channel width in MHz allowed for the given modulation class
 */
uint16_t GetMaximumChannelWidth(WifiModulationClass modulation);

/**
 * Return true if a preamble corresponds to an EHT transmission.
 *
 * \param preamble the preamble
 * \return true if the provided preamble corresponds to an EHT transmission
 */
bool IsEht(WifiPreamble preamble);

} // namespace ns3

#endif /* WIFI_PHY_COMMON_H */
