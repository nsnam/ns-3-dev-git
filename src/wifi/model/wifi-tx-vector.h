/*
 * Copyright (c) 2010 CTTC
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Ghada Badawy <gbadawy@gmail.com>
 */

#ifndef WIFI_TX_VECTOR_H
#define WIFI_TX_VECTOR_H

#include "wifi-mode.h"
#include "wifi-phy-common.h"

#include "ns3/he-ru.h"

#include <list>
#include <vector>

namespace ns3
{

/// HE MU specific user transmission parameters.
struct HeMuUserInfo
{
    HeRu::RuSpec ru; ///< RU specification
    uint8_t mcs;     ///< MCS index
    uint8_t nss;     ///< number of spatial streams

    /**
     * Compare this user info to the given user info.
     *
     * \param other the given user info
     * \return true if this user info compares equal to the given user info, false otherwise
     */
    bool operator==(const HeMuUserInfo& other) const;
    /**
     * Compare this user info to the given user info.
     *
     * \param other the given user info
     * \return true if this user info differs from the given user info, false otherwise
     */
    bool operator!=(const HeMuUserInfo& other) const;
};

/// A vector of subcarrier group
using SubcarrierGroups = std::vector<HeRu::SubcarrierGroup>;

/// Maximum number of HE-SIG-B content channels
constexpr size_t WIFI_MAX_NUM_HE_SIGB_CONTENT_CHANNELS = 2;

/// HE SIG-B Content Channels STA ID Allocation
using ContentChannelAllocation = std::vector<std::vector<uint16_t>>;

/// 8 bit RU_ALLOCATION per 20 MHz
using RuAllocation = std::vector<uint8_t>;

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
 * \note the above reference is valid for the DSSS PHY only (clause
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
    ~WifiTxVector();
    /**
     * Create a TXVECTOR with the given parameters.
     *
     * \param mode WifiMode
     * \param powerLevel transmission power level
     * \param preamble preamble type
     * \param guardInterval the guard interval duration in nanoseconds
     * \param nTx the number of TX antennas
     * \param nss the number of spatial STBC streams (NSS)
     * \param ness the number of extension spatial streams (NESS)
     * \param channelWidth the channel width in MHz
     * \param aggregation enable or disable MPDU aggregation
     * \param stbc enable or disable STBC
     * \param ldpc enable or disable LDPC (BCC is used otherwise)
     * \param bssColor the BSS color
     * \param length the LENGTH field of the L-SIG
     * \param triggerResponding the Trigger Responding parameter
     */
    WifiTxVector(WifiMode mode,
                 uint8_t powerLevel,
                 WifiPreamble preamble,
                 uint16_t guardInterval,
                 uint8_t nTx,
                 uint8_t nss,
                 uint8_t ness,
                 uint16_t channelWidth,
                 bool aggregation,
                 bool stbc = false,
                 bool ldpc = false,
                 uint8_t bssColor = 0,
                 uint16_t length = 0,
                 bool triggerResponding = false);
    /**
     * Copy constructor
     * \param txVector the TXVECTOR to copy
     */
    WifiTxVector(const WifiTxVector& txVector);

    /**
     * \returns whether mode has been initialized
     */
    bool GetModeInitialized() const;
    /**
     * If this TX vector is associated with an SU PPDU, return the selected
     * payload transmission mode. If this TX vector is associated with an
     * MU PPDU, return the transmission mode (MCS) selected for the transmission
     * to the station identified by the given STA-ID.
     *
     * \param staId the station ID for MU
     * \returns the selected payload transmission mode
     */
    WifiMode GetMode(uint16_t staId = SU_STA_ID) const;
    /**
     * Sets the selected payload transmission mode
     *
     * \param mode the payload WifiMode
     */
    void SetMode(WifiMode mode);
    /**
     * Sets the selected payload transmission mode for a given STA ID (for MU only)
     *
     * \param mode
     * \param staId the station ID for MU
     */
    void SetMode(WifiMode mode, uint16_t staId);

    /**
     * Get the modulation class specified by this TXVECTOR.
     *
     * \return the Modulation Class specified by this TXVECTOR
     */
    WifiModulationClass GetModulationClass() const;

    /**
     * \returns the transmission power level
     */
    uint8_t GetTxPowerLevel() const;
    /**
     * Sets the selected transmission power level
     *
     * \param powerlevel the transmission power level
     */
    void SetTxPowerLevel(uint8_t powerlevel);
    /**
     * \returns the preamble type
     */
    WifiPreamble GetPreambleType() const;
    /**
     * Sets the preamble type
     *
     * \param preamble the preamble type
     */
    void SetPreambleType(WifiPreamble preamble);
    /**
     * \returns the channel width (in MHz)
     */
    uint16_t GetChannelWidth() const;
    /**
     * Sets the selected channelWidth (in MHz)
     *
     * \param channelWidth the channel width (in MHz)
     */
    void SetChannelWidth(uint16_t channelWidth);
    /**
     * \returns the guard interval duration (in nanoseconds)
     */
    uint16_t GetGuardInterval() const;
    /**
     * Sets the guard interval duration (in nanoseconds)
     *
     * \param guardInterval the guard interval duration (in nanoseconds)
     */
    void SetGuardInterval(uint16_t guardInterval);
    /**
     * \returns the number of TX antennas
     */
    uint8_t GetNTx() const;
    /**
     * Sets the number of TX antennas
     *
     * \param nTx the number of TX antennas
     */
    void SetNTx(uint8_t nTx);
    /**
     * If this TX vector is associated with an SU PPDU, return the number of
     * spatial streams. If this TX vector is associated with an MU PPDU,
     * return the number of spatial streams for the transmission to the station
     * identified by the given STA-ID.
     *
     * \param staId the station ID for MU
     * \returns the number of spatial streams
     */
    uint8_t GetNss(uint16_t staId = SU_STA_ID) const;
    /**
     * \returns the maximum number of Nss (namely if MU)
     */
    uint8_t GetNssMax() const;
    /**
     * Sets the number of Nss
     *
     * \param nss the number of spatial streams
     */
    void SetNss(uint8_t nss);
    /**
     * Sets the number of Nss for MU
     *
     * \param nss the number of spatial streams
     * \param staId the station ID for MU
     */
    void SetNss(uint8_t nss, uint16_t staId);
    /**
     * \returns the number of extended spatial streams
     */
    uint8_t GetNess() const;
    /**
     * Sets the Ness number
     *
     * \param ness the number of extended spatial streams
     */
    void SetNess(uint8_t ness);
    /**
     * Checks whether the PSDU contains A-MPDU.
     * \returns true if this PSDU has A-MPDU aggregation,
     *          false otherwise
     */
    bool IsAggregation() const;
    /**
     * Sets if PSDU contains A-MPDU.
     *
     * \param aggregation whether the PSDU contains A-MPDU or not
     */
    void SetAggregation(bool aggregation);
    /**
     * Check if STBC is used or not
     *
     * \returns true if STBC is used,
     *           false otherwise
     */
    bool IsStbc() const;
    /**
     * Sets if STBC is being used
     *
     * \param stbc enable or disable STBC
     */
    void SetStbc(bool stbc);
    /**
     * Check if LDPC FEC coding is used or not
     *
     * \returns true if LDPC is used,
     *          false if BCC is used
     */
    bool IsLdpc() const;
    /**
     * Sets if LDPC FEC coding is being used
     *
     * \param ldpc enable or disable LDPC
     */
    void SetLdpc(bool ldpc);
    /**
     * Checks whether this TXVECTOR corresponds to a non-HT duplicate.
     * \returns true if this TXVECTOR corresponds to a non-HT duplicate,
     *          false otherwise.
     */
    bool IsNonHtDuplicate() const;
    /**
     * Set the BSS color
     * \param color the BSS color
     */
    void SetBssColor(uint8_t color);
    /**
     * Get the BSS color
     * \return the BSS color
     */
    uint8_t GetBssColor() const;
    /**
     * Set the LENGTH field of the L-SIG
     * \param length the LENGTH field of the L-SIG
     */
    void SetLength(uint16_t length);
    /**
     * Get the LENGTH field of the L-SIG
     * \return the LENGTH field of the L-SIG
     */
    uint16_t GetLength() const;
    /**
     * Return true if the Trigger Responding parameter is set to true, false otherwise.
     * \return true if the Trigger Responding parameter is set to true, false otherwise
     */
    bool IsTriggerResponding() const;
    /**
     * Set the Trigger Responding parameter to the given value
     * \param triggerResponding the value for the Trigger Responding parameter
     */
    void SetTriggerResponding(bool triggerResponding);
    /**
     * The standard disallows certain combinations of WifiMode, number of
     * spatial streams, and channel widths.  This method can be used to
     * check whether this WifiTxVector contains an invalid combination.
     *
     * \return true if the WifiTxVector parameters are allowed by the standard
     */
    bool IsValid() const;
    /**
     * \return true if this TX vector is used for a multi-user (OFDMA and/or MU-MIMO) transmission
     */
    bool IsMu() const;
    /**
     * \return true if this TX vector is used for a downlink multi-user (OFDMA and/or MU-MIMO)
     * transmission
     */
    bool IsDlMu() const;
    /**
     * \return true if this TX vector is used for an uplink multi-user (OFDMA and/or MU-MIMO)
     * transmission
     */
    bool IsUlMu() const;
    /**
     * Get the RU specification for the STA-ID.
     * This is applicable only for MU.
     *
     * \param staId the station ID
     * \return the RU specification for the STA-ID
     */
    HeRu::RuSpec GetRu(uint16_t staId) const;
    /**
     * Set the RU specification for the STA-ID.
     * This is applicable only for MU.
     *
     * \param ru the RU specification
     * \param staId the station ID
     */
    void SetRu(HeRu::RuSpec ru, uint16_t staId);
    /**
     * Get the HE MU user-specific transmission information for the given STA-ID.
     * This is applicable only for HE MU.
     *
     * \param staId the station ID
     * \return the HE MU user-specific transmission information for the given STA-ID
     */
    HeMuUserInfo GetHeMuUserInfo(uint16_t staId) const;
    /**
     * Set the HE MU user-specific transmission information for the given STA-ID.
     * This is applicable only for HE MU.
     *
     * \param staId the station ID
     * \param userInfo the HE MU user-specific transmission information
     */
    void SetHeMuUserInfo(uint16_t staId, HeMuUserInfo userInfo);
    /**
     * Get a const reference to the map HE MU user-specific transmission information indexed by
     * STA-ID. This is applicable only for HE MU.
     *
     * \return a const reference to the map of HE MU user-specific information indexed by STA-ID
     */
    const HeMuUserInfoMap& GetHeMuUserInfoMap() const;
    /**
     * Get a reference to the map HE MU user-specific transmission information indexed by STA-ID.
     * This is applicable only for HE MU.
     *
     * \return a reference to the map of HE MU user-specific information indexed by STA-ID
     */
    HeMuUserInfoMap& GetHeMuUserInfoMap();
    /**
     * Get the number of RUs per HE-SIG-B content channel.
     * This is applicable only for MU. MU-MIMO (i.e. multiple stations
     * per RU) is not supported yet.
     * See section 27.3.10.8.3 of IEEE 802.11ax draft 4.0.
     *
     * \return a pair containing the number of RUs in each HE-SIG-B content channel (resp. 1 and 2)
     */
    std::pair<std::size_t, std::size_t> GetNumRusPerHeSigBContentChannel() const;

    /**
     * Set the 20 MHz subchannels that are punctured.
     *
     * \param inactiveSubchannels the bitmap indexed by the 20 MHz subchannels in ascending order,
     *        where each bit indicates whether the corresponding 20 MHz subchannel is punctured or
     * not within the transmission bandwidth
     */
    void SetInactiveSubchannels(const std::vector<bool>& inactiveSubchannels);
    /**
     * Get the 20 MHz subchannels that are punctured.
     *
     * \return the bitmap indexed by the 20 MHz subchannels in ascending order,
     *         where each bit indicates whether the corresponding 20 MHz subchannel is punctured or
     * not within the transmission bandwidth
     */
    const std::vector<bool>& GetInactiveSubchannels() const;

    /**
     * Set the MCS used for SIG-B
     * \param mode MCS used for SIG-B
     */
    void SetSigBMode(const WifiMode& mode);

    /**
     * Get MCS used for SIG-B
     * \return MCS for SIG-B
     */
    WifiMode GetSigBMode() const;

    /**
     * Set RU Allocation of SIG-B common field
     * \param ruAlloc 8 bit RU_ALLOCATION per 20 MHz
     */
    void SetRuAllocation(const RuAllocation& ruAlloc);

    /**
     * Get RU Allocation of SIG-B
     * \return 8 bit RU_ALLOCATION per 20 MHz
     */
    const RuAllocation& GetRuAllocation() const;

    /**
     * Get the HE SIG-B content channel STA ID allocation
     * IEEE 802.11ax-2021 27.3.11.8.2 HE-SIG-B content channels
     * \return content channel allocation
     */
    ContentChannelAllocation GetContentChannelAllocation() const;

    /**
     * Set the EHT_PPDU_TYPE parameter
     * \param type the EHT_PPDU_TYPE parameter
     */
    void SetEhtPpduType(uint8_t type);
    /**
     * Get the EHT_PPDU_TYPE parameter
     * \return the EHT_PPDU_TYPE parameter
     */
    uint8_t GetEhtPpduType() const;

  private:
    /**
     * Derive the RU allocation from the TXVECTOR for which its RU allocation has not been set yet.
     * This is valid only for allocations of RUs of the same size.
     *
     * \return the RU allocation
     */
    RuAllocation DeriveRuAllocation() const;

    WifiMode m_mode;          /**< The DATARATE parameter in Table 15-4.
                              It is the value that will be passed
                              to PMD_RATE.request */
    uint8_t m_txPowerLevel;   /**< The TXPWR_LEVEL parameter in Table 15-4.
                              It is the value that will be passed
                              to PMD_TXPWRLVL.request */
    WifiPreamble m_preamble;  /**< preamble */
    uint16_t m_channelWidth;  /**< channel width in MHz */
    uint16_t m_guardInterval; /**< guard interval duration in nanoseconds */
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
    uint8_t m_ehtPpduType;               /**< EHT_PPDU_TYPE per Table 36-1 IEEE 802.11be D2.3 */
};

/**
 * Serialize WifiTxVector to the given ostream.
 *
 * \param os the output stream
 * \param v the WifiTxVector to stringify
 *
 * \return output stream
 */
std::ostream& operator<<(std::ostream& os, const WifiTxVector& v);

} // namespace ns3

#endif /* WIFI_TX_VECTOR_H */
