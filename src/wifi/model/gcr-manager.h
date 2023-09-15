/*
 * Copyright (c) 2023 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef GCR_MANAGER_H
#define GCR_MANAGER_H

#include "qos-utils.h"

#include "ns3/mac48-address.h"
#include "ns3/object.h"

#include <set>
#include <unordered_set>

namespace ns3
{

class ApWifiMac;
class WifiMpdu;

/**
 * @brief The possible values for group address retransmission policy
 */
enum class GroupAddressRetransmissionPolicy : uint8_t
{
    NO_ACK_NO_RETRY = 0,
    GCR_UNSOLICITED_RETRY,
    GCR_BLOCK_ACK
};

/// Groupcast protection mode enumeration
enum GroupcastProtectionMode : uint8_t
{
    RTS_CTS,
    CTS_TO_SELF
};

/**
 * @ingroup wifi
 *
 * GcrManager is a base class defining the API to handle 802.11aa GCR.
 */
class GcrManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    GcrManager();
    ~GcrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of a QoS AP.
     *
     * @param mac the wifi MAC
     */
    void SetWifiMac(Ptr<ApWifiMac> mac);

    /**
     * Get the configured retransmission policy.
     *
     * @return the configured retransmission policy
     */
    GroupAddressRetransmissionPolicy GetRetransmissionPolicy() const;

    /**
     * Get the retransmission policy to use to transmit a given group addressed packet.
     *
     * @param header the header of the groupcast frame
     * @return the retransmission policy to use
     */
    GroupAddressRetransmissionPolicy GetRetransmissionPolicyFor(const WifiMacHeader& header) const;

    /**
     * Set the GCR concealment address.
     *
     * @param address the GCR concealment address
     */
    void SetGcrConcealmentAddress(const Mac48Address& address);

    /**
     * Get the GCR concealment address.
     *
     * @return the GCR concealment address
     */
    const Mac48Address& GetGcrConcealmentAddress() const;

    /**
     * Indicate whether a group addressed packet should be transmitted to the GCR concealment
     * address.
     *
     * @param header the header of the groupcast packet
     * @return whether GCR concealment should be used
     */
    bool UseConcealment(const WifiMacHeader& header) const;

    /**
     * This function indicates whether a groupcast MPDU should be kept for next retransmission.
     *
     * @param mpdu the groupcast MPDU
     * @return whether a groupcast MPDU should be kept for next retransmission
     */
    bool KeepGroupcastQueued(Ptr<WifiMpdu> mpdu);

    /**
     * This function notifies a STA is associated.
     *
     * @param staAddress the MAC address of the STA
     * @param isGcrCapable whether GCR is supported by the STA
     */
    void NotifyStaAssociated(const Mac48Address& staAddress, bool isGcrCapable);

    /**
     * This function deletes a STA as a member of any group addresses.
     *
     * @param staAddress the MAC address of the STA
     */
    void NotifyStaDeassociated(const Mac48Address& staAddress);

    /**
     * This function adds a STA as a member of zero or more group addresses.
     *
     * @param staAddress the MAC address of the STA
     * @param groupAddressList zero or more MAC addresses to indicate the set of group addressed MAC
     * addresses for which the STA receives frames
     */
    void NotifyGroupMembershipChanged(const Mac48Address& staAddress,
                                      const std::set<Mac48Address>& groupAddressList);

    /// MAC addresses of member STAs of a GCR group
    using GcrMembers = std::unordered_set<Mac48Address, WifiAddressHash>;

    /**
     * Get the list of MAC addresses of member STAs for a given group address.
     * @param groupAddress the group address
     * @return list of MAC addresses of member STAs for the group address
     */
    const GcrMembers& GetMemberStasForGroupAddress(const Mac48Address& groupAddress) const;

    /**
     * Get the MAC address of the individually addressed recipient to use for a groupcast frame
     * or for a protection frame sent prior to groupcast frames for a given group address.
     * @param groupAddress the group address
     * @return the MAC address of the individually addressed recipient to use.
     */
    virtual Mac48Address GetIndividuallyAddressedRecipient(
        const Mac48Address& groupAddress) const = 0;

  protected:
    void DoDispose() override;

    /// List of associated STAs that are not GCR capable
    using NonGcrStas = GcrMembers;

    Ptr<ApWifiMac> m_apMac;                                  //!< the MAC of the AP
    GroupAddressRetransmissionPolicy m_retransmissionPolicy; //!< retransmission policy
    uint8_t m_gcrUnsolicitedRetryLimit;                      //!< GCR Unsolicited Retry Limit
    uint8_t m_unsolicitedRetryCounter;                       //!< the unsolicited retry counter
    Ptr<WifiMpdu> m_mpdu;                                    //!< current MPDU being retransmitted
    NonGcrStas m_nonGcrStas;                                 //!< the list of non-GCR capable STAs
    GcrMembers m_staMembers; //!< the list of STA members (assume currently each member is part of
                             //!< all group)
    GroupcastProtectionMode m_gcrProtectionMode; //!< Protection mode for groupcast frames
    Mac48Address m_gcrConcealmentAddress;        //!< GCR concealment address
};

} // namespace ns3

#endif /* GCR_MANAGER_H */
