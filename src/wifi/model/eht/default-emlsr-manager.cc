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

#include "default-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/boolean.h"
#include "ns3/channel-access-manager.h"
#include "ns3/log.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultEmlsrManager);

TypeId
DefaultEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DefaultEmlsrManager")
            .SetParent<EmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<DefaultEmlsrManager>()
            .AddAttribute("SwitchAuxPhy",
                          "Whether Aux PHY should switch channel to operate on the link on which "
                          "the Main PHY was operating before moving to the link of the Aux PHY. "
                          "Note that, if the Aux PHY does not switch channel, the main PHY will "
                          "switch back to its previous link once the TXOP terminates (otherwise, "
                          "no PHY will be listening on that EMLSR link).",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DefaultEmlsrManager::m_switchAuxPhy),
                          MakeBooleanChecker());
    return tid;
}

DefaultEmlsrManager::DefaultEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultEmlsrManager::~DefaultEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DefaultEmlsrManager::DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);
}

uint8_t
DefaultEmlsrManager::GetLinkToSendEmlOmn()
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

std::optional<uint8_t>
DefaultEmlsrManager::ResendNotification(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

void
DefaultEmlsrManager::NotifyEmlsrModeChanged()
{
    NS_LOG_FUNCTION(this);
}

void
DefaultEmlsrManager::NotifyMainPhySwitch(uint8_t currLinkId, uint8_t nextLinkId)
{
    NS_LOG_FUNCTION(this << currLinkId << nextLinkId);

    if (m_switchAuxPhy)
    {
        // switch channel on Aux PHY so that it operates on the link on which the main PHY was
        // operating
        SwitchAuxPhy(nextLinkId, currLinkId);
        return;
    }

    if (currLinkId != GetMainPhyId())
    {
        // the main PHY is leaving a non-primary link, hence an aux PHY needs to be reconnected
        NS_ASSERT_MSG(
            m_auxPhyToReconnect,
            "There should be an aux PHY to reconnect when the main PHY leaves a non-primary link");

        // the Aux PHY is not actually switching (hence no switching delay)
        GetStaMac()->NotifySwitchingEmlsrLink(m_auxPhyToReconnect, currLinkId, Seconds(0));
        SetCcaEdThresholdOnLinkSwitch(m_auxPhyToReconnect, currLinkId);
        m_auxPhyToReconnect = nullptr;
    }

    if (nextLinkId != GetMainPhyId())
    {
        // the main PHY is moving to a non-primary link and the aux PHY does not switch link
        m_auxPhyToReconnect = GetStaMac()->GetWifiPhy(nextLinkId);
    }
}

void
DefaultEmlsrManager::DoNotifyIcfReceived(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyUlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyTxopEnd(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // switch main PHY to the previous link, if needed
    if (!m_switchAuxPhy && m_auxPhyToReconnect)
    {
        auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

        // the main PHY may be switching at the end of a TXOP when, e.g., the main PHY starts
        // switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
        // is not received and the UL TXOP ends before the main PHY channel switch is completed.
        // In such cases, wait until the main PHY channel switch is completed before requesting
        // a new channel switch.
        // Backoff shall not be reset on the link left by the main PHY because a TXOP ended and
        // a new backoff value must be generated.
        if (!mainPhy->IsStateSwitching())
        {
            SwitchMainPhy(GetMainPhyId(), false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
        }
        else
        {
            Simulator::Schedule(mainPhy->GetDelayUntilIdle(), [=, this]() {
                // request the main PHY to switch back to the primary link only if in the meantime
                // no TXOP started on another link (which will require the main PHY to switch link)
                if (!GetEhtFem(linkId)->UsingOtherEmlsrLink())
                {
                    SwitchMainPhy(GetMainPhyId(), false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
                }
            });
        }
        return;
    }
}

} // namespace ns3
