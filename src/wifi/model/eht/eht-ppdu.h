/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
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

#ifndef EHT_PPDU_H
#define EHT_PPDU_H

#include "ns3/he-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::EhtPpdu class.
 */

namespace ns3
{

/**
 * \brief EHT PPDU (11be)
 * \ingroup wifi
 *
 * EhtPpdu is currently identical to HePpdu
 */
class EhtPpdu : public HePpdu
{
  public:
    /**
     * Create an EHT PPDU, storing a map of PSDUs.
     *
     * This PPDU can either be UL or DL.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param channel the operating channel of the PHY used to transmit this PPDU
     * \param ppduDuration the transmission duration of this PPDU
     * \param uid the unique ID of this PPDU or of the triggering PPDU if this is an EHT TB PPDU
     * \param flag the flag indicating the type of Tx PSD to build
     */
    EhtPpdu(const WifiConstPsduMap& psdus,
            const WifiTxVector& txVector,
            const WifiPhyOperatingChannel& channel,
            Time ppduDuration,
            uint64_t uid,
            TxPsdFlag flag);

    WifiPpduType GetType() const override;
    Ptr<WifiPpdu> Copy() const override;

    /**
     * Get the number of RUs per EHT-SIG-B content channel.
     * This function will be used once EHT PHY headers are implemented.
     *
     * \param channelWidth the channel width occupied by the PPDU (in MHz)
     * \param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * \param ruAllocation 8 bit RU_ALLOCATION per 20 MHz

     * \return a pair containing the number of RUs in each EHT-SIG-B content channel (resp. 1 and 2)
     */
    static std::pair<std::size_t, std::size_t> GetNumRusPerEhtSigBContentChannel(
        uint16_t channelWidth,
        uint8_t ehtPpduType,
        const std::vector<uint8_t>& ruAllocation);

    /**
     * Get variable length EHT-SIG field size
     * \param channelWidth the channel width occupied by the PPDU (in MHz)
     * \param ruAllocation 8 bit RU_ALLOCATION per 20 MHz
     * \param ehtPpduType the EHT_PPDU_TYPE used by the PPDU
     * \return field size in bytes
     */
    static uint32_t GetEhtSigFieldSize(uint16_t channelWidth,
                                       const std::vector<uint8_t>& ruAllocation,
                                       uint8_t ehtPpduType);

  private:
    bool IsDlMu() const override;
    bool IsUlMu() const override;
    void SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const override;

    uint8_t m_ehtSuMcs{0};      //!< EHT-MCS for EHT SU transmissions
    uint8_t m_ehtSuNStreams{1}; //!< Number of streams for EHT SU transmissions
    uint8_t m_ehtPpduType{1};   /**< EHT_PPDU_TYPE per Table 36-1 IEEE 802.11be D2.3.
                                     To be removed once EHT PHY headers are supported. */
};                              // class EhtPpdu

} // namespace ns3

#endif /* EHT_PPDU_H */
