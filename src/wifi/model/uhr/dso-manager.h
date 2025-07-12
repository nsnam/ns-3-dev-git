/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef DSO_MANAGER_H
#define DSO_MANAGER_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/wifi-ru.h"

#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace ns3
{

class UhrFrameExchangeManager;
class StaWifiMac;
class WifiMpdu;
class WifiPsdu;
class WifiPhyOperatingChannel;
class MgtAssocResponseHeader;
class WifiTxVector;

/**
 * @ingroup wifi
 * Enumeration of the possible DSO TXOP events to report.
 */
enum class DsoTxopEvent
{
    RX_ICF,  // Received Initial Control Frame to indicate the start of a DSO TXOP
    TX_ICR,  // Transmitted response to the ICF that initiated the DSO TXOP
    TIMEOUT, // DSO TXOP terminated because no PHY-RXSTART indication has been received within the
             // DSO expected timeout value
    DURATION_DETECT_END, // DSO TXOP terminated because the duration/ID field of the most recently
                         // received frame has been detected to be zero and EarlyTxopEndDetect
                         // attribute is enabled
    RX_CF_END,           // DSO TXOP terminated because a CF-END frame has been received
    RX_OBSS,             // DSO TXOP terminated because an OBSS frame has been received
    RX_OTHER, // DSO TXOP terminated because a frame that is not an OBSS frame and is not intended
              // to this DSO STA has been received
    FAILED_RESPONSE, // DSO TXOP terminated because no response to the most recently received frame
                     // from the AP that requires an immediate response after a SIFS has been
                     // transmitted (not supported yet)
};

/**
 * @ingroup wifi
 *
 * DsoManager is an abstract base class defining the API that UHR non-AP STA STAs with DSO activated
 * can use to handle the DSO operations.
 */
class DsoManager : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DsoManager();
    ~DsoManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an UHR non-AP STA.
     *
     * @param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Get whether the STA is currently operating on the DSO subband on the given link.
     *
     * @param linkId the ID of the link to check
     * @return true if the STA is on the DSO subband, false otherwise
     */
    bool IsOnDsoSubband(uint8_t linkId) const;

    /**
     * Notify the reception of a management frame addressed to us.
     *
     * @param mpdu the received MPDU
     * @param linkId the ID of the link over which the MPDU was received
     */
    void NotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Notify the reception of an initial Control frame on the given link.
     *
     * @param linkId the ID of the link on which the initial Control frame was received
     * @param ru the RU the non-AP STA should transition to or std::nullopt if no RU is assigned to
     * this STA
     */
    void NotifyIcfReceived(uint8_t linkId, std::optional<WifiRu::RuSpec> ru);

    /**
     * Notify the transmission of the response to an initial Control frame on the given link.
     *
     * @param linkId the ID of the link on which the ICR was transmitted
     */
    void NotifyIcrTransmitted(uint8_t linkId);

    /**
     * Notify the end of a TXOP on the given link.
     *
     * @param linkId the ID of the given link
     * @param psdu the received PSDU that triggered the termination of the TXOP, if any
     * @param txVector TXVECTOR of the received PSDU, if any
     */
    void NotifyTxopEnd(
        uint8_t linkId,
        Ptr<const WifiPsdu> psdu = nullptr,
        std::optional<std::reference_wrapper<const WifiTxVector>> txVector = std::nullopt);

    /**
     * Get the delay to switch to the DSO subband.
     * This is implementation-specific and has to be provided by the subclass.
     * @return the delay to switch to the DSO subband
     */
    virtual Time GetSwitchingDelayToDsoSubband() const = 0;

  protected:
    void DoDispose() override;

    /**
     * @return the MAC of the non-AP MLD managed by this DSO Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * @param linkId the ID of the given link
     * @return the UHR FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<UhrFrameExchangeManager> GetUhrFem(uint8_t linkId) const;

    /**
     * @param linkId the ID of the given link
     * @return the operating channel the PHY must switch to in order to operate
     *         on the primary subband
     */
    const WifiPhyOperatingChannel& GetPrimarySubband(uint8_t linkId) const;

    /**
     * @param linkId the ID of the given link
     * @return the operating channel(s) the PHY must switch to in order to operate
     *         on the DSO subband(s)
     */
    const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>& GetDsoSubbands(
        uint8_t linkId) const;

  private:
    /**
     * Compute the primary subband and the DSO subband(s) for a given link.
     * The primary subband is always the operating channel on that link.
     * For an UHR non-AP STA with DSO activated for which its PHY operating on that link supports at
     * least 80 MHz, DSO subband(s) are computed unless that PHY supports a channel width equal or
     * larger than the one of the AP. DSO subband(s) are calculated as follows:
     * - if the PHY supports 160 MHz and the AP operates on a 320 MHz channel, the DSO subband is
     * the secondary 160 MHz channel of the AP for that link;
     * - if the PHY supports 80 MHz and the AP operates on a 160 MHz channel, the DSO subband is the
     * secondary 80 MHz channel of the AP for that link;
     * - if the PHY supports 80 MHz and the AP operates on a 320 MHz channel, the DSO subbands are
     * all the 80 MHz channels of the AP for that link, except the primary 80 MHz channel.
     *
     * @param linkId the ID of the given link
     * @param assocResp the Association Response frame header for that link
     */
    void ComputeSubbands(uint8_t linkId, const MgtAssocResponseHeader& assocResp);

    /**
     * Perform a channel switch.
     *
     * @param linkId the ID of the given link
     * @param channel the operating channel to switch to
     * @param delay the switching delay
     */
    void SwitchPhyChannel(uint8_t linkId,
                          const WifiPhyOperatingChannel& channel,
                          const Time& delay);

    Ptr<StaWifiMac> m_staMac; //!< the MAC of the managed non-AP MLD

    std::map<uint8_t, WifiPhyOperatingChannel>
        m_primarySubbands; //!< link ID-indexed map of primary subbands
    std::map<uint8_t, std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>>
        m_dsoSubbands; //!< link ID-indexed map of DSO subbands

    Time m_dsoSwitchBackDelay; //!< DSO Switch Back delay

    /**
     * TracedCallback signature for DSO TXOP event trace source.
     *
     * @param event the event that occurred
     * @param linkId the ID of the link on which the event occurred
     */
    typedef void (*DsoTxopEventTracedCallback)(DsoTxopEvent event, uint8_t linkId);

    TracedCallback<DsoTxopEvent, uint8_t> m_dsoTxopEventTrace; //!< DSO TXOP event trace source
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the output stream
 * @param event the DSO TXOP event
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, DsoTxopEvent event);

} // namespace ns3

#endif /* DSO_MANAGER_H */
