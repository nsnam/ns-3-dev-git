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

#include "ns3/ctrl-headers.h"
#include "ns3/mac48-address.h"
#include "ns3/mgt-headers.h"
#include "ns3/object.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-phy-operating-channel.h"

#include <map>
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
     * Set the ID of main PHY (position in the vector of PHYs held by WifiNetDevice). This
     * method cannot be called during or after initialization.
     *
     * \param mainPhyId the ID of the main PHY
     */
    void SetMainPhyId(uint8_t mainPhyId);

    /**
     * \return the ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
     */
    uint8_t GetMainPhyId() const;

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
     * Set the member variable indicating whether the state of the CAM should be reset when
     * the main PHY switches channel and operates on the link associated with the CAM.
     *
     * \param enable whether the CAM state should be reset
     */
    void SetCamStateReset(bool enable);

    /**
     * \return the value of the member variable indicating whether the state of the CAM should be
     * reset when the main PHY switches channel and operates on the link associated with the CAM.
     */
    bool GetCamStateReset() const;

    /**
     * Notify the reception of a management frame addressed to us.
     *
     * \param mpdu the received MPDU
     * \param linkId the ID of the link over which the MPDU was received
     */
    void NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the reception of an initial Control frame on the given link.
     *
     * \param linkId the ID of the link on which the initial Control frame was received
     */
    void NotifyIcfReceived(uint8_t linkId);

    /**
     * Notify the start of an UL TXOP on the given link
     *
     * \param linkId the ID of the given link
     */
    void NotifyUlTxopStart(uint8_t linkId);

    /**
     * Notify the end of a TXOP on the given link.
     *
     * \param linkId the ID of the given link
     */
    void NotifyTxopEnd(uint8_t linkId);

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
    virtual uint8_t GetLinkToSendEmlOmn() = 0;

    /**
     * A previous EML Operating Mode Notification frame was dropped. Ask the subclass whether
     * the frame needs to be re-sent on the given link (if any).
     *
     * \param mpdu the dropped MPDU that includes the EML Operating Mode Notification frame
     * \return the ID of the link over which to re-send the frame, if needed
     */
    virtual std::optional<uint8_t> ResendNotification(Ptr<const WifiMpdu> mpdu) = 0;

    /**
     * \param linkId the ID of the given link
     * \return the operating channel the main PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForMainPhy(uint8_t linkId) const;

    /**
     * \param linkId the ID of the given link
     * \return the operating channel an aux PHY must switch to in order to operate
     *         on the given link
     */
    const WifiPhyOperatingChannel& GetChannelForAuxPhy(uint8_t linkId) const;

    /**
     * \return the EML Operating Mode Notification to send
     */
    MgtEmlOmn GetEmlOmn();

    Time m_emlsrPaddingDelay;    //!< EMLSR Padding delay
    Time m_emlsrTransitionDelay; //!< EMLSR Transition delay
    uint8_t m_mainPhyId; //!< ID of main PHY (position in the vector of PHYs held by WifiNetDevice)
    uint16_t m_auxPhyMaxWidth; //!< max channel width (MHz) supported by aux PHYs

  private:
    /**
     * Compute the operating channels that the main PHY and the aux PHY(s) must switch to in order
     * to operate on each of the setup links. The operating channels may be different due to
     * limited channel width capabilities of the aux PHY(s). This method shall be called upon
     * completion of ML setup.
     */
    void ComputeOperatingChannels();

    /**
     * Send an EML Operating Mode Notification frame.
     */
    void SendEmlOmn();

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
     * Switch channel on the Main PHY so that it operates on the given link.
     *
     * \param linkId the ID of the link on which the main PHY has to operate
     * \param noSwitchDelay whether switching delay should be zero
     */
    void SwitchMainPhy(uint8_t linkId, bool noSwitchDelay);

    /**
     * Adjust the operating channel of all the aux PHYs to meet the constraint on the maximum
     * channel width supported by aux PHYs.
     */
    void ApplyMaxChannelWidthOnAuxPhys();

    /**
     * Notify subclass that EMLSR mode changed.
     */
    virtual void NotifyEmlsrModeChanged() = 0;

    /**
     * Notify subclass that the main PHY is switching channel to operate on another link.
     *
     * \param currLinkId the ID of the link on which the main PHY is operating
     * \param nextLinkId the ID of the link on which the main PHY will be operating
     */
    virtual void NotifyMainPhySwitch(uint8_t currLinkId, uint8_t nextLinkId) = 0;

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
    bool m_resetCamState; //!< whether to reset the state of CAM when main PHY switches channel
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_mainPhyChannels; //!< link ID-indexed map of operating channels for the main PHY
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_auxPhyChannels; //!< link ID-indexed map of operating channels for the aux PHYs
};

} // namespace ns3

#endif /* EMLSR_MANAGER_H */
