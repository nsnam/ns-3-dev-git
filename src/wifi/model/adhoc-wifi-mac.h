/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef ADHOC_WIFI_MAC_H
#define ADHOC_WIFI_MAC_H

#include "wifi-mac.h"

namespace ns3
{

struct AllSupportedRates;
class DsssParameterSet;
class CapabilityInformation;
class RandomVariableStream;
class ErpInformation;
class EdcaParameterSet;
class HtOperation;
class VhtOperation;
class HeOperation;
class EhtOperation;

/**
 * @ingroup wifi
 *
 * @brief Wifi MAC high model for an ad-hoc Wifi MAC
 *
 * As per IEEE 802.11-2024, sec. 11.1.3.5 (Beacon generation in an IBSS), if beacon generation is
 * enabled, TBTTs occur every BeaconInterval. At each TBTT a random delay in [0, 2 * aCWmin *
 * aSlotTime] is applied before transmitting a Beacon. If a Beacon from the same IBSS is received
 * before that delay expires, the remaining delay and any pending Beacon are cancelled.
 *
 * Note that The first TBTT may be further delayed by BeaconJitter.
 */
class AdhocWifiMac : public WifiMac
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    AdhocWifiMac();
    ~AdhocWifiMac() override;

    void SetLinkUpCallback(Callback<void> linkUp) override;
    bool CanForwardPacketsTo(Mac48Address to) const override;
    Ptr<Txop> GetTxopFor(AcIndex ac) const override;
    int64_t AssignStreams(int64_t stream) override;

  private:
    void DoInitialize() override;
    void DoDispose() override;
    void Receive(Ptr<const WifiMpdu> mpdu, uint8_t linkId) override;
    void DoCompleteConfig() override;
    void Enqueue(Ptr<WifiMpdu> mpdu, Mac48Address to, Mac48Address from) override;

    /**
     * Enable or disable beacon generation.
     *
     * @param enable enable or disable beacon generation
     */
    void SetBeaconGeneration(bool enable);

    /**
     * @param interval the interval between two beacon transmissions.
     */
    void SetBeaconInterval(Time interval);

    /**
     * @return the interval between two beacon transmissions.
     */
    Time GetBeaconInterval() const;

    /**
     * Called at each target beacon transmission time (TBTT).
     * Schedules the next TBTT and a Beacon transmission after the IBSS random delay.
     */
    void TbttTimeout();

    /**
     * Cancel the remaining IBSS random delay and flush any Beacon waiting
     * in the beacon queue.
     */
    void CancelPendingBeacon();

    /**
     * Forward a beacon packet for transmission.
     */
    void SendOneBeacon();

    /**
     * Process the Beacon frame received on the given link.
     *
     * @param mpdu the MPDU containing the Beacon frame
     * @param linkId the ID of the given link
     */
    void ReceiveBeacon(Ptr<const WifiMpdu> mpdu, linkId_t linkId);

    /**
     * Process the Probe Request frame received on the given link.
     *
     * @param mpdu the MPDU containing the Probe Request frame
     * @param linkId the ID of the given link
     */
    void ReceiveProbeRequest(Ptr<const WifiMpdu> mpdu, linkId_t linkId);

    /**
     * Fill in the same capabilities as this device for a given new peer.
     *
     * @param address the MAC address of the peer
     */
    void SetAllCapabilities(Mac48Address address);

    /**
     * Return an instance of SupportedRates that contains all rates that we support
     * (including HT rates).
     *
     * @return all rates that we support for the given link
     */
    AllSupportedRates GetSupportedRates() const;

    /**
     * Return the DSSS Parameter Set that we support
     *
     * @return the DSSS Parameter Set that we support
     */
    DsssParameterSet GetDsssParameterSet() const;

    /**
     * Return the Capability information of the IBSS.
     *
     * @return the Capability information that we support
     */
    CapabilityInformation GetCapabilities() const;

    /**
     * Return the ERP information of the IBSS.
     *
     * @return the ERP information that we support
     */
    ErpInformation GetErpInformation() const;

    /**
     * Return the EDCA Parameter Set of the IBSS.
     *
     * @return the EDCA Parameter Set that we support
     */
    EdcaParameterSet GetEdcaParameterSet() const;

    /**
     * Return the HT operation of the IBSS.
     *
     * @return the HT operation that we support
     */
    HtOperation GetHtOperation() const;

    /**
     * Return the VHT operation of the IBSS.
     *
     * @return the VHT operation that we support
     */
    VhtOperation GetVhtOperation() const;

    /**
     * Return the HE operation of the IBSS.
     *
     * @return the HE operation that we support
     */
    HeOperation GetHeOperation() const;

    /**
     * Return the EHT operation of the IBSS.
     *
     * @return the EHT operation that we support
     */
    EhtOperation GetEhtOperation() const;

    Ptr<Txop> m_beaconTxop;        //!< Dedicated Txop for beacons
    bool m_enableBeaconGeneration; //!< Flag whether beacons are being generated
    Time m_beaconInterval;         //!< the beacon interval
    AcIndex m_beaconAc;            //!< the access category to use for beacons
    Ptr<RandomVariableStream>
        m_beaconJitter; //!< Uniform [0, 1] RV for the initial TBTT offset and the per-TBTT delay
    bool m_enableBeaconJitter; //!< Flag whether the first TBTT is delayed by a random fraction of
                               //!< BeaconInterval

    EventId m_tbttEvent;   //!< Event for the next TBTT
    EventId m_beaconEvent; //!< Event to generate one beacon after the IBSS random delay
};

} // namespace ns3

#endif /* ADHOC_WIFI_MAC_H */
