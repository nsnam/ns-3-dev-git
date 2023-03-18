/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef EHT_FRAME_EXCHANGE_MANAGER_H
#define EHT_FRAME_EXCHANGE_MANAGER_H

#include "ns3/he-frame-exchange-manager.h"
#include "ns3/mgt-headers.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * EhtFrameExchangeManager handles the frame exchange sequences
 * for EHT stations.
 */
class EhtFrameExchangeManager : public HeFrameExchangeManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    EhtFrameExchangeManager();
    ~EhtFrameExchangeManager() override;

    void SetLinkId(uint8_t linkId) override;
    Ptr<WifiMpdu> CreateAliasIfNeeded(Ptr<WifiMpdu> mpdu) const override;

    /**
     * Send an EML Operating Mode Notification frame to the given station.
     *
     * \param dest the MAC address of the receiver
     * \param frame the EML Operating Mode Notification frame to send
     */
    void SendEmlOperatingModeNotification(const Mac48Address& dest,
                                          const MgtEmlOperatingModeNotification& frame);

    /**
     * Get the RSSI (in dBm) of the most recent packet received from the station having
     * the given address. If there is no such information for the given station and the
     * station is affiliated with an MLD, return the RSSI (in dBm) of the most recent
     * packet received from another station of the same MLD.
     *
     * \param address of the remote station
     * \return the RSSI (in dBm) of the most recent packet received from the remote station
     */
    std::optional<double> GetMostRecentRssi(const Mac48Address& address) const override;

    /**
     * \param psdu the given PSDU
     * \param aid the AID of an EMLSR client
     * \param address the link MAC address of an EMLSR client
     * \return whether the EMLSR client having the given AID and MAC address shall switch back to
     *         the listening operation when receiving the given PSDU
     */
    bool GetEmlsrSwitchToListening(Ptr<const WifiPsdu> psdu,
                                   uint16_t aid,
                                   const Mac48Address& address) const;

  protected:
    void ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector) override;
    void ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector) override;
    void SendMuRts(const WifiTxParameters& txParams) override;
    void NotifyChannelReleased(Ptr<Txop> txop) override;

    /**
     * This method is intended to be called when an AP MLD detects that an EMLSR client previously
     * involved in the current TXOP will start waiting for the transition delay interval (to switch
     * back to listening operation) after the given delay.
     * This method blocks the transmissions on all the EMLSR links of the given EMLSR client until
     * the transition delay advertised by the EMLSR client expires.
     *
     * \param address the link MAC address of the given EMLSR client
     * \param delay the given delay
     */
    void EmlsrSwitchToListening(const Mac48Address& address, const Time& delay);
};

} // namespace ns3

#endif /* EHT_FRAME_EXCHANGE_MANAGER_H */
