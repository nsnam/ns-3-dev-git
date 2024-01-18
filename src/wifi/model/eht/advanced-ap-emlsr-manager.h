/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
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

#ifndef ADVANCED_AP_EMLSR_MANAGER_H
#define ADVANCED_AP_EMLSR_MANAGER_H

#include "default-ap-emlsr-manager.h"

#include "ns3/nstime.h"

#include <set>

namespace ns3
{

class ApWifiMac;
class WifiMacHeader;
class WifiPsdu;
class WifiTxVector;

/**
 * \ingroup wifi
 *
 * AdvancedApEmlsrManager is an advanced AP EMLSR manager.
 */
class AdvancedApEmlsrManager : public DefaultApEmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    AdvancedApEmlsrManager();
    ~AdvancedApEmlsrManager() override;

    void NotifyPsduRxOk(uint8_t linkId, Ptr<const WifiPsdu> psdu) override;
    void NotifyPsduRxError(uint8_t linkId, Ptr<const WifiPsdu> psdu) override;
    Time GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                     const WifiTxVector& txVector,
                                     WifiPhyBand band) override;
    bool UpdateCwAfterFailedIcf() override;

  protected:
    void DoDispose() override;
    void DoSetWifiMac(Ptr<ApWifiMac> mac) override;

    /**
     * Store information about the MAC header of the MPDU being received on the given link.
     *
     * \param linkId the ID of the given link
     * \param macHdr the MAC header of the MPDU being received
     * \param txVector the TXVECTOR used to transmit the PSDU
     * \param psduDuration the remaining duration of the PSDU
     */
    void ReceivedMacHdr(uint8_t linkId,
                        const WifiMacHeader& macHdr,
                        const WifiTxVector& txVector,
                        Time psduDuration);

  private:
    std::set<uint8_t>
        m_blockedLinksOnMacHdrRx;  //!< links that have been blocked upon receiving a MAC header
    bool m_useNotifiedMacHdr;      //!< whether to use the information about the MAC header of
                                   //!< the MPDU being received (if notified by the PHY)
    bool m_earlySwitchToListening; //!< Whether the AP MLD assumes that an EMLSR client is able to
                                   //!< detect at the end of the MAC header that a PSDU is not
                                   //!< addressed to it and immediately starts switching to
                                   //!< listening mode
    bool m_waitTransDelayOnPsduRxError; //!< Whether the AP MLD waits for a response timeout after a
                                        //!< PSDU reception error before starting the transition
                                        //!< delay
    bool m_updateCwAfterFailedIcf;      //!< Whether the AP MLD shall double the CW upon CTS timeout
                                   //!< after an MU-RTS in case all the clients solicited by the
                                   //!< MU-RTS are EMLSR clients that have sent a frame to the AP
};

} // namespace ns3

#endif /* ADVANCED_AP_EMLSR_MANAGER_H */
