/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Ghada Badawy <gbadawy@gmail.com>
 */

#ifndef WIFI_TX_VECTOR_H
#define WIFI_TX_VECTOR_H

#include "wifi-mode.h"
#include "wifi-phy-band.h"
#include "wifi-phy-common.h"

#include "ns3/he-ru.h"
#include "ns3/nstime.h"

#include <list>
#include <optional>
#include <set>
#include <vector>

namespace ns3
{

/// STA_ID for a RU that is intended for no user (Section 26.11.1 802.11ax-2021)
static constexpr uint16_t NO_USER_STA_ID = 2046;

/// HE MU specific user transmission parameters.
struct HeMuUserInfo
{
    HeRu::RuSpec ru; ///< RU specification
    uint8_t mcs;     ///< MCS index
    uint8_t nss;     ///< number of spatial streams

    /**
     * Compare this user info to the given user info.
     *
     * @param other the given user info
     * @return true if this user info compares equal to the given user info, false otherwise
     */
    bool operator==(const HeMuUserInfo& other) const;
    /**
     * Compare this user info to the given user info.
     *
     * @param other the given user info
     * @return true if this user info differs from the given user info, false otherwise
     */
    bool operator!=(const HeMuUserInfo& other) const;
};

/// 9 bits RU_ALLOCATION per 20 MHz
using RuAllocation = std::vector<uint16_t>;

/**
 * @ingroup wifi
 * Enum for the different values for CENTER_26_TONE_RU
 */
enum Center26ToneRuIndication : uint8_t
{
    CENTER_26_TONE_RU_UNALLOCATED = 0,
    CENTER_26_TONE_RU_LOW_80_MHZ_ALLOCATED,          /* also used if BW == 80 MHz */
    CENTER_26_TONE_RU_HIGH_80_MHZ_ALLOCATED,         /* unused if BW < 160 MHz */
    CENTER_26_TONE_RU_LOW_AND_HIGH_80_MHZ_ALLOCATED, /* unused if BW < 160 MHz */
    CENTER_26_TONE_RU_INDICATION_MAX                 /* last value */
};

/**
 * This class mimics the TXVECTOR which is to be
 * passed to the PHY in order to define the parameters which are to be
 * used for a transmission. See IEEE 802.11-2016 16.2.5 "Transmit PHY",
 * and also 8.3.4.1 "PHY SAP peer-to-peer service primitive
 * parameters".
 *
 * If this class is constructed with the constructor that takes no
 * arguments, then the client must explicitly set the mode and
 * transmit power level parameters before using them.  Default
 * member initializers are provided for the other parameters, to
 * conform to a non-MIMO/long guard configuration, although these
 * may also be explicitly set after object construction.
 *
 * When used in a infrastructure context, WifiTxVector values should be
 * drawn from WifiRemoteStationManager parameters since rate adaptation
 * is responsible for picking the mode, number of streams, etc., but in
 * the case in which there is no such manager (e.g. mesh), the client
 * still needs to initialize at least the mode and transmit power level
 * appropriately.
 *
 * @note the above reference is valid for the DSSS PHY only (clause
 * 16). TXVECTOR is defined also for the other PHYs, however they
 * don't include the TXPWRLVL explicitly in the TXVECTOR. This is
 * somewhat strange, since all PHYs actually have a
 * PMD_TXPWRLVL.request primitive. We decide to include the power
 * level in WifiTxVector for all PHYs, since it serves better our
 * purposes, and furthermore it seems close to the way real devices
 * work (e.g., madwifi).
 */
class WifiTxVector
{
  public:
    /// map of HE MU specific user info parameters indexed by STA-ID
    typedef std::map<uint16_t /* staId */, HeMuUserInfo /* HE MU specific user info */>
        HeMuUserInfoMap;

    WifiTxVector();
    /**
     * Create a TXVECTOR with the given parameters.
     *
     * @param mode WifiMode
     * @param powerLevel transmission power level
     * @param preamble preamble type
     * @param guardInterval the guard interval duration in nanoseconds
     * @param nTx the number of TX antennas
     * @param nss the number of spatial STBC streams (NSS)
     * @param ness the number of extension spatial streams (NESS)
     * @param channelWidth the channel width
     * @param aggregation enable or disable MPDU aggregation
     * @param stbc enable or disable STBC
     * @param ldpc enable or disable LDPC (BCC is used otherwise)
     * @param bssColor the BSS color
     * @param length the LENGTH field of the L-SIG
     * @param triggerResponding the Trigger Responding parameter
     */
    WifiTxVector(WifiMode mode,
                 uint8_t powerLevel,
                 WifiPreamble preamble,
                 Time guardInterval,
                 uint8_t nTx,
                 uint8_t nss,
                 uint8_t ness,
                 MHz_u channelWidth,
                 bool aggregation,
                 bool stbc = false,
                 bool ldpc = false,
                 uint8_t bssColor = 0,
                 uint16_t length = 0,
                 bool triggerResponding = false);
    /**
     * Copy constructor
     * @param txVector the TXVECTOR to copy
     */
    WifiTxVector(const WifiTxVector& txVector);

    /**
     * @returns whether mode has been initialized
     */
    bool GetModeInitialized() const;
    /**
     * If this TX vector is associated with an SU PPDU, return the selected
     * payload transmission mode. If this TX vector is associated with an
     * MU PPDU, return the transmission mode (MCS) selected for the transmission
     * to the station identified by the given STA-ID.
     *
     * @param staId the station ID for MU
     * @returns the selected payload transmission mode
     */
    WifiMode GetMode(uint16_t staId = SU_STA_ID) const;
    /**
     * Sets the selected payload transmission mode
     *
     * @param mode the payload WifiMode
     */
    void SetMode(WifiMode mode);
    /**
     * Sets the selected payload transmission mode for a given STA ID (for MU only)
     *
     * @param mode
     * @param staId the station ID for MU
     */
    void SetMode(WifiMode mode, uint16_t staId);

    /**
     * Get the modulation class specified by this TXVECTOR.
     *
     * @return the Modulation Class specified by this TXVECTOR
     */
    WifiModulationClass GetModulationClass() const;

    /**
     * @returns the transmission power level
     */
    uint8_t GetTxPowerLevel() const;
    /**
     * Sets the selected transmission power level
     *
     * @param powerlevel the transmission power level
     */
    void SetTxPowerLevel(uint8_t powerlevel);
    /**
     * @returns the preamble type
     */
    WifiPreamble GetPreambleType() const;
    /**
     * Sets the preamble type
     *
     * @param preamble the preamble type
     */
    void SetPreambleType(WifiPreamble preamble);
    /**
     * @returns the channel width
     */
    MHz_u GetChannelWidth() const;
    /**
     * Sets the selected channelWidth
     *
     * @param channelWidth the channel width
     */
    void SetChannelWidth(MHz_u channelWidth);
    /**
     * @returns the guard interval duration (in nanoseconds)
     */
    Time GetGuardInterval() const;
    /**
     * Sets the guard interval duration (in nanoseconds)
     *
     * @param guardInterval the guard interval duration (in nanoseconds)
     */
    void SetGuardInterval(Time guardInterval);
    /**
     * @returns the number of TX antennas
     */
    uint8_t GetNTx() const;
    /**
     * Sets the number of TX antennas
     *
     * @param nTx the number of TX antennas
     */
    void SetNTx(uint8_t nTx);
    /**
     * If this TX vector is associated with an SU PPDU, return the number of
     * spatial streams. If this TX vector is associated with an MU PPDU,
     * return the number of spatial streams for the transmission to the station
     * identified by the given STA-ID.
     *
     * @param staId the station ID for MU
     * @returns the number of spatial streams
     */
    uint8_t GetNss(uint16_t staId = SU_STA_ID) const;
    /**
     * @returns the maximum number of Nss over all RUs of an HE MU (used for OFDMA)
     */
    uint8_t GetNssMax() const;
    /**
     * @returns the total number of Nss for a given RU of an HE MU (used for full bandwidth MU-MIMO)
     */
    uint8_t GetNssTotal() const;
    /**
     * Sets the number of Nss
     *
     * @param nss the number of spatial streams
     */
    void SetNss(uint8_t nss);
    /**
     * Sets the number of Nss for MU
     *
     * @param nss the number of spatial streams
     * @param staId the station ID for MU
     */
    void SetNss(uint8_t nss, uint16_t staId);
    /**
     * @returns the number of extended spatial streams
     */
    uint8_t GetNess() const;
    /**
     * Sets the Ness number
     *
     * @param ness the number of extended spatial streams
     */
    void SetNess(uint8_t ness);
    /**
     * Checks whether the PSDU contains A-MPDU.
     * @returns true if this PSDU has A-MPDU aggregation,
     *          false otherwise
     */
    bool IsAggregation() const;
    /**
     * Sets if PSDU contains A-MPDU.
     *
     * @param aggregation whether the PSDU contains A-MPDU or not
     */
    void SetAggregation(bool aggregation);
    /**
     * Check if STBC is used or not
     *
     * @returns true if STBC is used,
     *           false otherwise
     */
    bool IsStbc() const;
    /**
     * Sets if STBC is being used
     *
     * @param stbc enable or disable STBC
     */
    void SetStbc(bool stbc);
    /**
     * Check if LDPC FEC coding is used or not
     *
     * @returns true if LDPC is used,
     *          false if BCC is used
     */
    bool IsLdpc() const;
    /**
     * Sets if LDPC FEC coding is being used
     *
     * @param ldpc enable or disable LDPC
     */
    void SetLdpc(bool ldpc);
    /**
     * Checks whether this TXVECTOR corresponds to a non-HT duplicate.
     * @returns true if this TXVECTOR corresponds to a non-HT duplicate,
     *          false otherwise.
     */
    bool IsNonHtDuplicate() const;
    /**
     * Set the BSS color
     * @param color the BSS color
     */
    void SetBssColor(uint8_t color);
    /**
     * Get the BSS color
     * @return the BSS color
     */
    uint8_t GetBssColor() const;
    /**
     * Set the LENGTH field of the L-SIG
     * @param length the LENGTH field of the L-SIG
     */
    void SetLength(uint16_t length);
    /**
     * Get the LENGTH field of the L-SIG
     * @return the LENGTH field of the L-SIG
     */
    uint16_t GetLength() const;
    /**
     * Return true if the Trigger Responding parameter is set to true, false otherwise.
     * @return true if the Trigger Responding parameter is set to true, false otherwise
     */
    bool IsTriggerResponding() const;
    /**
     * Set the Trigger Responding parameter to the given value
     * @param triggerResponding the value for the Trigger Responding parameter
     */
    void SetTriggerResponding(bool triggerResponding);
    /**
     * The standard disallows certain combinations of WifiMode, number of
     * spatial streams, and channel widths.  This method can be used to
     * check whether this WifiTxVector contains an invalid combination.
     * If a PHY band is specified, it is checked that the PHY band is appropriate for
     * the modulation class of the TXVECTOR, in case the latter is OFDM or ERP-OFDM.
     *
     * @param band the PHY band
     * @return true if the WifiTxVector parameters are allowed by the standard
     */
    bool IsValid(WifiPhyBand band = WIFI_PHY_BAND_UNSPECIFIED) const;
    /**
     * @return true if this TX vector is used for a multi-user (OFDMA and/or MU-MIMO) transmission
     */
    bool IsMu() const;
    /**
     * @return true if this TX vector is used for a downlink multi-user (OFDMA and/or MU-MIMO)
     * transmission
     */
    bool IsDlMu() const;
    /**
     * @return true if this TX vector is used for an uplink multi-user (OFDMA and/or MU-MIMO)
     * transmission
     */
    bool IsUlMu() const;
    /**
     * Return true if this TX vector is used for a downlink multi-user transmission using OFDMA.
     *
     * @return true if this TX vector is used for a downlink multi-user transmission using OFDMA
     */
    bool IsDlOfdma() const;
    /**
     * Return true if this TX vector is used for a downlink multi-user transmission using MU-MIMO.
     *
     * @return true if this TX vector is used for a downlink multi-user transmission using MU-MIMO
     */
    bool IsDlMuMimo() const;
    /**
     * Check if STA ID is allocated
     * @param staId STA ID
     * @return true if allocated, false otherwise
     */
    bool IsAllocated(uint16_t staId) const;
    /**
     * Get the RU specification for the STA-ID.
     * This is applicable only for MU.
     *
     * @param staId the station ID
     * @return the RU specification for the STA-ID
     */
    HeRu::RuSpec GetRu(uint16_t staId) const;
    /**
     * Set the RU specification for the STA-ID.
     * This is applicable only for MU.
     *
     * @param ru the RU specification
     * @param staId the station ID
     */
    void SetRu(HeRu::RuSpec ru, uint16_t staId);
    /**
     * Get the HE MU user-specific transmission information for the given STA-ID.
     * This is applicable only for HE MU.
     *
     * @param staId the station ID
     * @return the HE MU user-specific transmission information for the given STA-ID
     */
    HeMuUserInfo GetHeMuUserInfo(uint16_t staId) const;
    /**
     * Set the HE MU user-specific transmission information for the given STA-ID.
     * This is applicable only for HE MU.
     *
     * @param staId the station ID
     * @param userInfo the HE MU user-specific transmission information
     */
    void SetHeMuUserInfo(uint16_t staId, HeMuUserInfo userInfo);
    /**
     * Get a const reference to the map HE MU user-specific transmission information indexed by
     * STA-ID. This is applicable only for HE MU.
     *
     * @return a const reference to the map of HE MU user-specific information indexed by STA-ID
     */
    const HeMuUserInfoMap& GetHeMuUserInfoMap() const;
    /**
     * Get a reference to the map HE MU user-specific transmission information indexed by STA-ID.
     * This is applicable only for HE MU.
     *
     * @return a reference to the map of HE MU user-specific information indexed by STA-ID
     */
    HeMuUserInfoMap& GetHeMuUserInfoMap();

    /// map of specific user info parameters ordered per increasing frequency RUs
    using UserInfoMapOrderedByRus = std::map<HeRu::RuSpec, std::set<uint16_t>, HeRu::RuSpecCompare>;

    /**
     * Get the map of specific user info parameters ordered per increasing frequency RUs.
     *
     * @param p20Index the index of the primary20 channel
     * @return the map of specific user info parameters ordered per increasing frequency RUs
     */
    UserInfoMapOrderedByRus GetUserInfoMapOrderedByRus(uint8_t p20Index) const;

    /**
     * Indicate whether the Common field is present in the HE-SIG-B field.
     *
     * @return true if the Common field is present in the HE-SIG-B, false otherwise
     */
    bool IsSigBCompression() const;

    /**
     * Set the 20 MHz subchannels that are punctured.
     *
     * @param inactiveSubchannels the bitmap indexed by the 20 MHz subchannels in ascending order,
     *        where each bit indicates whether the corresponding 20 MHz subchannel is punctured or
     * not within the transmission bandwidth
     */
    void SetInactiveSubchannels(const std::vector<bool>& inactiveSubchannels);
    /**
     * Get the 20 MHz subchannels that are punctured.
     *
     * @return the bitmap indexed by the 20 MHz subchannels in ascending order,
     *         where each bit indicates whether the corresponding 20 MHz subchannel is punctured or
     * not within the transmission bandwidth
     */
    const std::vector<bool>& GetInactiveSubchannels() const;

    /**
     * Set the MCS used for SIG-B
     * @param mode MCS used for SIG-B
     */
    void SetSigBMode(const WifiMode& mode);

    /**
     * Get MCS used for SIG-B
     * @return MCS for SIG-B
     */
    WifiMode GetSigBMode() const;

    /**
     * Set RU_ALLOCATION field
     * @param ruAlloc 8 bit RU_ALLOCATION per 20 MHz
     * @param p20Index the index of the primary20 channel
     */
    void SetRuAllocation(const RuAllocation& ruAlloc, uint8_t p20Index);

    /**
     * Get RU_ALLOCATION field
     * @return 8 bit RU_ALLOCATION per 20 MHz
     * @param p20Index the index of the primary20 channel
     */
    const RuAllocation& GetRuAllocation(uint8_t p20Index) const;

    /**
     * Set CENTER_26_TONE_RU field
     * @param center26ToneRuIndication the CENTER_26_TONE_RU field
     */
    void SetCenter26ToneRuIndication(Center26ToneRuIndication center26ToneRuIndication);

    /**
     * Get CENTER_26_TONE_RU field
     * This field is present if format is HE_MU and
     * when channel width is set to 80 MHz or larger.
     * @return the CENTER_26_TONE_RU field if present
     */
    std::optional<Center26ToneRuIndication> GetCenter26ToneRuIndication() const;

    /**
     * Set the EHT_PPDU_TYPE parameter
     * @param type the EHT_PPDU_TYPE parameter
     */
    void SetEhtPpduType(uint8_t type);
    /**
     * Get the EHT_PPDU_TYPE parameter
     * @return the EHT_PPDU_TYPE parameter
     */
    uint8_t GetEhtPpduType() const;

  private:
    /**
     * Derive the RU_ALLOCATION field from the TXVECTOR
     * for which its RU_ALLOCATION field has not been set yet,
     * based on the content of per-user information.
     * This is valid only for allocations of RUs of the same size per 20 MHz subchannel.
     *
     * @param p20Index the index of the primary20 channel
     * @return 8 bit RU_ALLOCATION per 20 MHz
     */
    RuAllocation DeriveRuAllocation(uint8_t p20Index) const;

    /**
     * Derive the CENTER_26_TONE_RU field from the TXVECTOR
     * for which its CENTER_26_TONE_RU has not been set yet,
     * based on the content of per-user information.
     *
     * @return the CENTER_26_TONE_RU field
     */
    Center26ToneRuIndication DeriveCenter26ToneRuIndication() const;

    /**
     * Get the number of STAs in a given RU.
     *
     * @param ru the RU specification
     * @return the number of STAs in the RU
     */
    uint8_t GetNumStasInRu(const HeRu::RuSpec& ru) const;

    WifiMode m_mode;          /**< The DATARATE parameter in Table 15-4.
                              It is the value that will be passed
                              to PMD_RATE.request */
    uint8_t m_txPowerLevel;   /**< The TXPWR_LEVEL parameter in Table 15-4.
                              It is the value that will be passed
                              to PMD_TXPWRLVL.request */
    WifiPreamble m_preamble;  /**< preamble */
    MHz_u m_channelWidth;     /**< channel width */
    Time m_guardInterval;     /**< guard interval duration */
    uint8_t m_nTx;            /**< number of TX antennas */
    uint8_t m_nss;            /**< number of spatial streams */
    uint8_t m_ness;           /**< number of spatial streams in beamforming */
    bool m_aggregation;       /**< Flag whether the PSDU contains A-MPDU. */
    bool m_stbc;              /**< STBC used or not */
    bool m_ldpc;              /**< LDPC FEC coding if true, BCC otherwise*/
    uint8_t m_bssColor;       /**< BSS color */
    uint16_t m_length;        /**< LENGTH field of the L-SIG */
    bool m_triggerResponding; /**< The Trigger Responding parameter */

    bool m_modeInitialized; /**< Internal initialization flag */

    // MU information
    HeMuUserInfoMap m_muUserInfos; /**< HE MU specific per-user information
                                        indexed by station ID (STA-ID) corresponding
                                        to the 11 LSBs of the AID of the recipient STA
                                        This list shall be used only for HE MU */
    std::vector<bool>
        m_inactiveSubchannels; /**< Bitmap of inactive subchannels used for preamble puncturing */

    WifiMode m_sigBMcs; /**< MCS_SIG_B per Table 27-1 IEEE 802.11ax-2021 */

    mutable RuAllocation m_ruAllocation; /**< RU allocations that are going to be carried
                                              in SIG-B common field per Table 27-1 IEEE */

    mutable std::optional<Center26ToneRuIndication>
        m_center26ToneRuIndication; /**< CENTER_26_TONE_RU field when format is HE_MU and
                                         when channel width is set to 80 MHz or larger  (Table 27-1
                                       802.11ax-2021)*/

    uint8_t m_ehtPpduType; /**< EHT_PPDU_TYPE per Table 36-1 IEEE 802.11be D2.3 */
};

/**
 * Serialize WifiTxVector to the given ostream.
 *
 * @param os the output stream
 * @param v the WifiTxVector to stringify
 *
 * @return output stream
 */
std::ostream& operator<<(std::ostream& os, const WifiTxVector& v);

} // namespace ns3

#endif /* WIFI_TX_VECTOR_H */
