/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#ifndef EMLSR_MANAGER_H
#define EMLSR_MANAGER_H

#include "ns3/mac48-address.h"
#include "ns3/mgt-headers.h"
#include "ns3/object.h"
#include "ns3/sta-wifi-mac.h"

#include <optional>
#include <set>

namespace ns3
{

class EhtFrameExchangeManager;
class WifiMpdu;

/**
 * \ingroup wifi
 *
 * EmlsrManager is an abstract base class defining the API that EHT non-AP MLDs
 * with EMLSR activated can use to handle the operations on the EMLSR links
 */
class EmlsrManager : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    EmlsrManager();
    ~EmlsrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an EHT non-AP MLD.
     *
     * \param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Set the Transition Timeout advertised by the associated AP with EMLSR activated.
     *
     * \param timeout the advertised Transition Timeout
     */
    void SetTransitionTimeout(Time timeout);

    /**
     * \return the Transition Timeout, if advertised by the associated AP
     */
    std::optional<Time> GetTransitionTimeout() const;

    /**
     * Take actions to enable EMLSR mode on the given set of links, if non-empty, or
     * disable EMLSR mode, otherwise.
     *
     * \param linkIds the IDs of the links on which EMLSR mode should be enabled
     *                (empty to disable EMLSR mode)
     */
    void SetEmlsrLinks(const std::set<uint8_t>& linkIds);

    /**
     * \return the set of links on which EMLSR mode is enabled
     */
    const std::set<uint8_t>& GetEmlsrLinks() const;

    /**
     * Notify the reception of a management frame addressed to us.
     *
     * \param mpdu the received MPDU
     * \param linkId the ID of the link over which the MPDU was received
     */
    void NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

  protected:
    void DoDispose() override;

    /**
     * \return the MAC of the non-AP MLD managed by this EMLSR Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * \param linkId the ID of the given link
     * \return the EHT FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<EhtFrameExchangeManager> GetEhtFem(uint8_t linkId) const;

    /**
     * \return the ID of the link on which the EML Operating Mode Notification frame has to be sent
     */
    virtual uint8_t GetLinkToSendEmlNotification() = 0;

    /**
     * A previous EML Operating Mode Notification frame was dropped. Ask the subclass whether
     * the frame needs to be re-sent on the given link (if any).
     *
     * \param mpdu the dropped MPDU that includes the EML Operating Mode Notification frame
     * \return the ID of the link over which to re-send the frame, if needed
     */
    virtual std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) = 0;

    Time m_emlsrPaddingDelay;    //!< EMLSR Padding delay
    Time m_emlsrTransitionDelay; //!< EMLSR Transition delay

  private:
    /**
     * Send an EML Operating Mode Notification frame.
     */
    void SendEmlOperatingModeNotification();

    /**
     * Notify the subclass of the reception of a management frame addressed to us.
     *
     * \param mpdu the received MPDU
     * \param linkId the ID of the link over which the MPDU was received
     */
    virtual void DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId) = 0;

    /**
     * Notify the acknowledgment of the given MPDU.
     *
     * \param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);

    /**
     * Notify that the given MPDU has been discarded for the given reason.
     *
     * \param reason the reason why the MPDU was dropped
     * \param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /**
     * This method is called to make an EMLSR mode change effective after the transition
     * delay has elapsed or a notification response has been received from the AP.
     */
    void ChangeEmlsrMode();

    /**
     * Notify subclass that EMLSR mode changed.
     */
    virtual void NotifyEmlsrModeChanged() = 0;

    Ptr<StaWifiMac> m_staMac;                     //!< the MAC of the managed non-AP MLD
    std::optional<Time> m_emlsrTransitionTimeout; /**< Transition timeout advertised by APs with
                                                       EMLSR activated */
    std::set<uint8_t> m_emlsrLinks; //!< ID of the EMLSR links (empty if EMLSR mode is disabled)
    std::optional<std::set<uint8_t>> m_nextEmlsrLinks; /**< ID of the links that will become the
                                                            EMLSR links when the pending
                                                            notification frame is acknowledged */
    Time m_lastAdvPaddingDelay;                        //!< last advertised padding delay
    Time m_lastAdvTransitionDelay;                     //!< last advertised transition delay
    EventId m_transitionTimeoutEvent; /**< Timer started after the successful transmission of an
                                           EML Operating Mode Notification frame */
};

} // namespace ns3

#endif /* EMLSR_MANAGER_H */
