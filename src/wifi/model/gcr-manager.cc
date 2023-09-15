/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "gcr-manager.h"

#include "ap-wifi-mac.h"
#include "wifi-mpdu.h"

#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GcrManager");

NS_OBJECT_ENSURE_REGISTERED(GcrManager);

TypeId
GcrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GcrManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute("RetransmissionPolicy",
                          "The retransmission policy to use for group addresses.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          EnumValue(GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY),
                          MakeEnumAccessor<GroupAddressRetransmissionPolicy>(
                              &GcrManager::m_retransmissionPolicy),
                          MakeEnumChecker(GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY,
                                          "NO_RETRY",
                                          GroupAddressRetransmissionPolicy::GCR_UNSOLICITED_RETRY,
                                          "GCR_UR",
                                          GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK,
                                          "GCR_BA"))
            .AddAttribute(
                "GcrProtectionMode",
                "Protection mode used for groupcast frames when needed: "
                "Rts-Cts or Cts-To-Self",
                EnumValue(GroupcastProtectionMode::RTS_CTS),
                MakeEnumAccessor<GroupcastProtectionMode>(&GcrManager::m_gcrProtectionMode),
                MakeEnumChecker(GroupcastProtectionMode::RTS_CTS,
                                "Rts-Cts",
                                GroupcastProtectionMode::CTS_TO_SELF,
                                "Cts-To-Self"))
            .AddAttribute("UnsolicitedRetryLimit",
                          "The maximum number of transmission attempts of a frame delivered using "
                          "the GCR unsolicited retry retransmission policy.",
                          UintegerValue(7),
                          MakeUintegerAccessor(&GcrManager::m_gcrUnsolicitedRetryLimit),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("GcrConcealmentAddress",
                          "The GCR concealment address.",
                          Mac48AddressValue(Mac48Address("01:0F:AC:47:43:52")),
                          MakeMac48AddressAccessor(&GcrManager::SetGcrConcealmentAddress,
                                                   &GcrManager::GetGcrConcealmentAddress),
                          MakeMac48AddressChecker());
    return tid;
}

GcrManager::GcrManager()
    : m_unsolicitedRetryCounter{0}
{
    NS_LOG_FUNCTION(this);
}

GcrManager::~GcrManager()
{
    NS_LOG_FUNCTION(this);
}

void
GcrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_apMac = nullptr;
    Object::DoDispose();
}

void
GcrManager::SetWifiMac(Ptr<ApWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ASSERT(mac);
    m_apMac = mac;

    NS_ABORT_MSG_IF(m_apMac->GetTypeOfStation() != AP || !m_apMac->GetQosSupported(),
                    "GcrManager can only be installed on QoS APs");
}

GroupAddressRetransmissionPolicy
GcrManager::GetRetransmissionPolicy() const
{
    return m_retransmissionPolicy;
}

GroupAddressRetransmissionPolicy
GcrManager::GetRetransmissionPolicyFor(const WifiMacHeader& header) const
{
    NS_ASSERT_MSG(header.IsQosData() && IsGroupcast(header.GetAddr1()),
                  "GCR service is only for QoS groupcast data frames");

    // 11.21.16.3.4 GCR operation:
    // A STA providing GCR service may switch between the DMS,
    // GCR block ack, or GCR unsolicited retry retransmission policies

    // This is a retry, use configured retransmission policy
    if (header.IsRetry())
    {
        return m_retransmissionPolicy;
    }

    // This is not a retry but all STAs are GCR-capable, use configured retransmission policy
    if (m_nonGcrStas.empty())
    {
        return m_retransmissionPolicy;
    }

    // not a retry and GCR-incapable STA(s) present, transmit using No-Ack/No-Retry
    return GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY;
}

void
GcrManager::SetGcrConcealmentAddress(const Mac48Address& address)
{
    NS_LOG_FUNCTION(this << address);
    NS_ASSERT_MSG(address.IsGroup(), "The concealment address should be a group address");
    m_gcrConcealmentAddress = address;
}

const Mac48Address&
GcrManager::GetGcrConcealmentAddress() const
{
    return m_gcrConcealmentAddress;
}

bool
GcrManager::UseConcealment(const WifiMacHeader& header) const
{
    NS_ASSERT_MSG(header.IsQosData() && IsGroupcast(header.GetAddr1()),
                  "GCR service is only for QoS groupcast data frames");
    NS_ASSERT_MSG(m_retransmissionPolicy != GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY,
                  "GCR service is not enabled");
    NS_ASSERT_MSG(!m_staMembers.empty(), "GCR service should not be used");

    // Only GCR capable STAs, hence concealment is always used
    if (m_nonGcrStas.empty())
    {
        return true;
    }
    // If A-MSDU is used, that means previous transmission was already concealed
    if (header.IsQosAmsdu())
    {
        return true;
    }
    // Otherwise, use concealment except for the first transmission (hence when it is a retry)
    return header.IsRetry();
}

bool
GcrManager::KeepGroupcastQueued(Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << *mpdu);
    NS_ASSERT_MSG(mpdu->GetHeader().IsQosData() && IsGroupcast(mpdu->GetHeader().GetAddr1()),
                  "GCR service is only for QoS groupcast data frames");
    NS_ASSERT_MSG(m_retransmissionPolicy != GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY,
                  "GCR service is not enabled");
    NS_ASSERT_MSG(!m_staMembers.empty(), "GCR service should not be used");
    if (m_retransmissionPolicy == GroupAddressRetransmissionPolicy::GCR_BLOCK_ACK)
    {
        return !mpdu->GetHeader().IsRetry() && !m_nonGcrStas.empty();
    }
    if (!m_mpdu || !mpdu->GetHeader().IsRetry())
    {
        m_unsolicitedRetryCounter = 0;
        m_mpdu = mpdu;
        NS_LOG_DEBUG("First groupcast transmission using No-Ack/No-Retry");
    }
    else
    {
        m_unsolicitedRetryCounter++;
        NS_LOG_DEBUG("GCR solicited retry counter increased to " << +m_unsolicitedRetryCounter);
    }
    if (m_unsolicitedRetryCounter == m_gcrUnsolicitedRetryLimit)
    {
        NS_LOG_DEBUG("Last groupcast transmission retry done");
        m_mpdu = nullptr;
        m_unsolicitedRetryCounter = 0;
        return false;
    }
    return true;
}

void
GcrManager::NotifyStaAssociated(const Mac48Address& staAddress, bool gcrCapable)
{
    if (m_retransmissionPolicy == GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY)
    {
        // GCR is not used and we do not support run-time change of the retransmission policy
        return;
    }
    NS_LOG_FUNCTION(this << staAddress << gcrCapable);
    if (gcrCapable)
    {
        NS_ASSERT(m_staMembers.count(staAddress) == 0);
        m_staMembers.insert(staAddress);
    }
    else
    {
        NS_ASSERT(m_nonGcrStas.count(staAddress) == 0);
        m_nonGcrStas.insert(staAddress);
    }
}

void
GcrManager::NotifyStaDeassociated(const Mac48Address& staAddress)
{
    if (m_retransmissionPolicy == GroupAddressRetransmissionPolicy::NO_ACK_NO_RETRY)
    {
        // GCR is not used and we do not support run-time change of the retransmission policy
        return;
    }
    NS_LOG_FUNCTION(this << staAddress);
    m_nonGcrStas.erase(staAddress);
    m_staMembers.erase(staAddress);
}

const GcrManager::GcrMembers&
GcrManager::GetMemberStasForGroupAddress(const Mac48Address& /* groupAddress */) const
{
    // TODO: we currently assume all STAs belong to all group addresses
    // as long as group membership action frame is not implemented
    return m_staMembers;
}

void
GcrManager::NotifyGroupMembershipChanged(const Mac48Address& staAddress,
                                         const std::set<Mac48Address>& groupAddressList)
{
    NS_LOG_FUNCTION(this << staAddress << groupAddressList.size());
    // TODO: group membership is not implemented yet, current implementation assumes GCR STAs are
    // members of all groups
}

} // namespace ns3
