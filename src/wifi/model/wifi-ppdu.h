/*
 * Copyright (c) 2019 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#ifndef WIFI_PPDU_H
#define WIFI_PPDU_H

#include "wifi-psdu.h"
#include "wifi-tx-vector.h"

#include "ns3/nstime.h"

#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::WifiPpdu class
 * and ns3::WifiConstPsduMap.
 */

namespace ns3
{

class Packet;
class WifiPhyOperatingChannel;

/**
 * Map of const PSDUs indexed by STA-ID
 */
typedef std::unordered_map<uint16_t /* STA-ID */, Ptr<const WifiPsdu> /* PSDU */> WifiConstPsduMap;

/**
 * @ingroup wifi
 *
 * WifiPpdu stores a preamble, a modulation class, PHY headers and a PSDU.
 * This class should be subclassed for each amendment.
 */
class WifiPpdu : public SimpleRefCount<WifiPpdu>
{
  public:
    /**
     * Create a PPDU storing a PSDU.
     *
     * @param psdu the PHY payload (PSDU)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param uid the unique ID of this PPDU
     */
    WifiPpdu(Ptr<const WifiPsdu> psdu,
             const WifiTxVector& txVector,
             const WifiPhyOperatingChannel& channel,
             uint64_t uid = UINT64_MAX);
    /**
     * Create a PPDU storing a map of PSDUs.
     *
     * @param psdus the PHY payloads (PSDUs)
     * @param txVector the TXVECTOR that was used for this PPDU
     * @param channel the operating channel of the PHY used to transmit this PPDU
     * @param uid the unique ID of this PPDU
     */
    WifiPpdu(const WifiConstPsduMap& psdus,
             const WifiTxVector& txVector,
             const WifiPhyOperatingChannel& channel,
             uint64_t uid);
    /**
     * Destructor for WifiPpdu.
     */
    virtual ~WifiPpdu() = default;

    /**
     * Get the TXVECTOR used to send the PPDU.
     *
     * @return the TXVECTOR of the PPDU.
     */
    const WifiTxVector& GetTxVector() const;

    /**
     * Reset the TXVECTOR.
     */
    void ResetTxVector() const;

    /**
     * Update the TXVECTOR based on some information known at the receiver.
     *
     * @param updatedTxVector the updated TXVECTOR.
     */
    void UpdateTxVector(const WifiTxVector& updatedTxVector) const;

    /**
     * Get the payload of the PPDU.
     *
     * @return the PSDU
     */
    Ptr<const WifiPsdu> GetPsdu() const;

    /**
     * @return c\ true if the PPDU's transmission was aborted due to transmitter switch off
     */
    bool IsTruncatedTx() const;

    /**
     * Indicate that the PPDU's transmission was aborted due to transmitter switch off.
     */
    void SetTruncatedTx();

    /**
     * Get the total transmission duration of the PPDU.
     *
     * @return the transmission duration of the PPDU
     */
    virtual Time GetTxDuration() const;

    /**
     * Get the channel width over which the PPDU will effectively be
     * transmitted.
     *
     * @return the effective channel width used for the tranmsission
     */
    virtual MHz_u GetTxChannelWidth() const;

    /**
     * @return the center frequency per segment used for the transmission of this PPDU
     */
    std::vector<MHz_u> GetTxCenterFreqs() const;

    /**
     * Check whether the given PPDU overlaps a given channel.
     *
     * @param minFreq the minimum frequency of the channel
     * @param maxFreq the maximum frequency of the channel
     * @return true if this PPDU overlaps the channel, false otherwise
     */
    bool DoesOverlapChannel(MHz_u minFreq, MHz_u maxFreq) const;

    /**
     * Get the modulation used for the PPDU.
     * @return the modulation used for the PPDU
     */
    WifiModulationClass GetModulation() const;

    /**
     * Get the UID of the PPDU.
     * @return the UID of the PPDU
     */
    uint64_t GetUid() const;

    /**
     * Get the preamble of the PPDU.
     * @return the preamble of the PPDU
     */
    WifiPreamble GetPreamble() const;

    /**
     * @brief Print the PPDU contents.
     * @param os output stream in which the data should be printed.
     */
    void Print(std::ostream& os) const;
    /**
     * @brief Copy this instance.
     * @return a Ptr to a copy of this instance.
     */
    virtual Ptr<WifiPpdu> Copy() const;

    /**
     * Return the PPDU type (\see WifiPpduType)
     * @return the PPDU type
     */
    virtual WifiPpduType GetType() const;

    /**
     * Get the ID of the STA that transmitted the PPDU for UL MU,
     * SU_STA_ID otherwise.
     * @return the ID of the STA that transmitted the PPDU for UL MU, SU_STA_ID otherwise
     */
    virtual uint16_t GetStaId() const;

  protected:
    /**
     * @brief Print the payload of the PPDU.
     * @return information on the payload part of the PPDU
     */
    virtual std::string PrintPayload() const;

    WifiPreamble m_preamble;            //!< the PHY preamble
    WifiModulationClass m_modulation;   //!< the modulation used for the transmission of this PPDU
    WifiConstPsduMap m_psdus;           //!< the PSDUs contained in this PPDU
    std::vector<MHz_u> m_txCenterFreqs; //!< the center frequency per segment used for the
                                        //!< transmission of this PPDU
    uint64_t m_uid;                     //!< the unique ID of this PPDU
    mutable std::optional<WifiTxVector>
        m_txVector; //!< the TXVECTOR at TX PHY or the reconstructed TXVECTOR at RX PHY (or
                    //!< std::nullopt if TXVECTOR has not been reconstructed yet)
    const WifiPhyOperatingChannel& m_operatingChannel; //!< the operating channel of the PHY

  private:
    /**
     * Get the TXVECTOR used to send the PPDU.
     *
     * @return the TXVECTOR of the PPDU.
     */
    virtual WifiTxVector DoGetTxVector() const;

    bool m_truncatedTx;     //!< flag indicating whether the frame's transmission was aborted due to
                            //!< transmitter switch off
    uint8_t m_txPowerLevel; //!< the transmission power level (used only for TX and initializing the
                            //!< returned WifiTxVector)
    uint8_t m_txAntennas;   //!< the number of antennas used to transmit this PPDU

    MHz_u m_txChannelWidth; /**< The channel width used for the transmission of this
                                         PPDU. This has to be stored since channel width can not
                                         always be obtained from the PHY headers, especially for
                                         non-HT PPDU, since we do not sense the spectrum to
                                         determine the occupied channel width for simplicity. */
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param ppdu the const pointer to the PPDU
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const Ptr<const WifiPpdu>& ppdu);

} // namespace ns3

#endif /* WIFI_PPDU_H */
