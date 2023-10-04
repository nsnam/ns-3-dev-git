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

    if (!m_switchAuxPhy)
    {
        // record that the main PHY will have to switch back to its current link
        m_linkIdForMainPhyAfterTxop = currLinkId;
        m_auxPhyToReconnect = GetStaMac()->GetWifiPhy(nextLinkId);
        return;
    }

    // switch channel on Aux PHY so that it operates on the link on which the main PHY was operating
    SwitchAuxPhy(nextLinkId, currLinkId);
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
    if (m_linkIdForMainPhyAfterTxop && linkId != m_linkIdForMainPhyAfterTxop)
    {
        auto auxPhy = m_auxPhyToReconnect;

        // lambda to switch the main PHY back to its previous link and reconnect the aux PHY to
        // its original link
        auto restorePhys = [=, this]() {
            SwitchMainPhy(*m_linkIdForMainPhyAfterTxop, false);
            // the Aux PHY is not actually switching (hence no switching delay)
            GetStaMac()->NotifySwitchingEmlsrLink(auxPhy, linkId, Seconds(0));
            SetCcaEdThresholdOnLinkSwitch(auxPhy, linkId);
            m_linkIdForMainPhyAfterTxop.reset();
        };

        auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

        // the main PHY may be switching at the end of a TXOP when, e.g., the main PHY starts
        // switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
        // is not received and the UL TXOP ends before the main PHY channel switch is completed.
        // In such cases, wait until the main PHY channel switch is completed before requesting
        // a new channel switch.
        if (!mainPhy->IsStateSwitching())
        {
            restorePhys();
        }
        else
        {
            Simulator::Schedule(mainPhy->GetDelayUntilIdle(), restorePhys);
        }
        return;
    }
    m_linkIdForMainPhyAfterTxop.reset();
}

} // namespace ns3
