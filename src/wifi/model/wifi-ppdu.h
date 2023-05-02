/*
 * Copyright (c) 2019 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 */

#ifndef WIFI_PPDU_H
#define WIFI_PPDU_H

#include "wifi-tx-vector.h"

#include "ns3/nstime.h"

#include <list>
#include <optional>
#include <unordered_map>

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::WifiPpdu class
 * and ns3::WifiConstPsduMap.
 */

namespace ns3
{

class Packet;
class WifiPsdu;
class WifiPhyOperatingChannel;

/**
 * Map of const PSDUs indexed by STA-ID
 */
typedef std::unordered_map<uint16_t /* STA-ID */, Ptr<const WifiPsdu> /* PSDU */> WifiConstPsduMap;

/**
 * \ingroup wifi
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
     * \param psdu the PHY payload (PSDU)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param channel the operating channel of the PHY used to transmit this PPDU
     * \param uid the unique ID of this PPDU
     */
    WifiPpdu(Ptr<const WifiPsdu> psdu,
             const WifiTxVector& txVector,
             const WifiPhyOperatingChannel& channel,
             uint64_t uid = UINT64_MAX);
    /**
     * Create a PPDU storing a map of PSDUs.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param channel the operating channel of the PHY used to transmit this PPDU
     * \param uid the unique ID of this PPDU
     */
    WifiPpdu(const WifiConstPsduMap& psdus,
             const WifiTxVector& txVector,
             const WifiPhyOperatingChannel& channel,
             uint64_t uid);
    /**
     * Destructor for WifiPpdu.
     */
    virtual ~WifiPpdu();

    /**
     * Get the TXVECTOR used to send the PPDU.
     *
     * \return the TXVECTOR of the PPDU.
     */
    const WifiTxVector& GetTxVector() const;

    /**
     * Reset the TXVECTOR.
     */
    void ResetTxVector() const;

    /**
     * Update the TXVECTOR based on some information known at the receiver.
     *
     * \param updatedTxVector the updated TXVECTOR.
     */
    void UpdateTxVector(const WifiTxVector& updatedTxVector) const;

    /**
     * Get the payload of the PPDU.
     *
     * \return the PSDU
     */
    Ptr<const WifiPsdu> GetPsdu() const;

    /**
     * \return c\ true if the PPDU's transmission was aborted due to transmitter switch off
     */
    bool IsTruncatedTx() const;

    /**
     * Indicate that the PPDU's transmission was aborted due to transmitter switch off.
     */
    void SetTruncatedTx();

    /**
     * Get the total transmission duration of the PPDU.
     *
     * \return the transmission duration of the PPDU
     */
    virtual Time GetTxDuration() const;

    /**
     * Get the channel width over which the PPDU will effectively be
     * transmitted.
     *
     * \return the effective channel width (in MHz) used for the tranmsission
     */
    virtual uint16_t GetTransmissionChannelWidth() const;

    /**
     * \return the center frequency (MHz) used for the transmission of this PPDU
     */
    uint16_t GetTxCenterFreq() const;

    /**
     * Check whether the given PPDU overlaps a given channel.
     *
     * \param minFreq the minimum frequency (MHz) of the channel
     * \param maxFreq the maximum frequency (MHz) of the channel
     * \return true if this PPDU overlaps the channel, false otherwise
     */
    bool DoesOverlapChannel(uint16_t minFreq, uint16_t maxFreq) const;

    /**
     * Get the modulation used for the PPDU.
     * \return the modulation used for the PPDU
     */
    WifiModulationClass GetModulation() const;

    /**
     * Get the UID of the PPDU.
     * \return the UID of the PPDU
     */
    uint64_t GetUid() const;

    /**
     * Get the preamble of the PPDU.
     * \return the preamble of the PPDU
     */
    WifiPreamble GetPreamble() const;

    /**
     * \brief Print the PPDU contents.
     * \param os output stream in which the data should be printed.
     */
    void Print(std::ostream& os) const;
    /**
     * \brief Copy this instance.
     * \return a Ptr to a copy of this instance.
     */
    virtual Ptr<WifiPpdu> Copy() const;

    /**
     * Return the PPDU type (\see WifiPpduType)
     * \return the PPDU type
     */
    virtual WifiPpduType GetType() const;

    /**
     * Get the ID of the STA that transmitted the PPDU for UL MU,
     * SU_STA_ID otherwise.
     * \return the ID of the STA that transmitted the PPDU for UL MU, SU_STA_ID otherwise
     */
    virtual uint16_t GetStaId() const;

  protected:
    /**
     * \brief Print the payload of the PPDU.
     * \return information on the payload part of the PPDU
     */
    virtual std::string PrintPayload() const;

    WifiPreamble m_preamble;          //!< the PHY preamble
    WifiModulationClass m_modulation; //!< the modulation used for the transmission of this PPDU
    WifiConstPsduMap m_psdus;         //!< the PSDUs contained in this PPDU
    uint16_t m_txCenterFreq; //!< the center frequency (MHz) used for the transmission of this PPDU
    uint64_t m_uid;          //!< the unique ID of this PPDU
    mutable std::optional<WifiTxVector>
        m_txVector; //!< the TXVECTOR at TX PHY or the reconstructed TXVECTOR at RX PHY (or
                    //!< std::nullopt if TXVECTOR has not been reconstructed yet)
    const WifiPhyOperatingChannel& m_operatingChannel; //!< the operating channel of the PHY

  private:
    /**
     * Get the TXVECTOR used to send the PPDU.
     *
     * \return the TXVECTOR of the PPDU.
     */
    virtual WifiTxVector DoGetTxVector() const;

    bool m_truncatedTx;     //!< flag indicating whether the frame's transmission was aborted due to
                            //!< transmitter switch off
    uint8_t m_txPowerLevel; //!< the transmission power level (used only for TX and initializing the
                            //!< returned WifiTxVector)
    uint8_t m_txAntennas;   //!< the number of antennas used to transmit this PPDU
};                          // class WifiPpdu

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param ppdu the const pointer to the PPDU
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const Ptr<const WifiPpdu>& ppdu);

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param psdus the PSDUs
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const WifiConstPsduMap& psdus);

} // namespace ns3

#endif /* WIFI_PPDU_H */
