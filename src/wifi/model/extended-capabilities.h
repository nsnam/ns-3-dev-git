/*
 * Copyright (c) 2017
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EXTENDED_CAPABILITIES_H
#define EXTENDED_CAPABILITIES_H

#include "wifi-information-element.h"

namespace ns3
{

/**
 * \brief The Extended Capabilities Information Element
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the Extended Capabilities Information Element
 */
class ExtendedCapabilities : public WifiInformationElement
{
  public:
    ExtendedCapabilities();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the first byte in the Extended Capabilities information element.
     *
     * \param ctrl the first byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte1(uint8_t ctrl);
    /**
     * Set the second byte in the Extended Capabilities information element.
     *
     * \param ctrl the second byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte2(uint8_t ctrl);
    /**
     * Set the third byte in the Extended Capabilities information element.
     *
     * \param ctrl the third byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte3(uint8_t ctrl);
    /**
     * Set the fourth byte in the Extended Capabilities information element.
     *
     * \param ctrl the fourth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte4(uint8_t ctrl);
    /**
     * Set the fifth byte in the Extended Capabilities information element.
     *
     * \param ctrl the fifth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte5(uint8_t ctrl);
    /**
     * Set the sixth byte in the Extended Capabilities information element.
     *
     * \param ctrl the sixth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte6(uint8_t ctrl);
    /**
     * Set the seventh byte in the Extended Capabilities information element.
     *
     * \param ctrl the seventh byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte7(uint8_t ctrl);
    /**
     * Set the eighth byte in the Extended Capabilities information element.
     *
     * \param ctrl the eighth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte8(uint8_t ctrl);

    /**
     * Return the first byte in the Extended Capabilities information element.
     *
     * \return the first byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte1() const;
    /**
     * Return the second byte in the Extended Capabilities information element.
     *
     * \return the second byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte2() const;
    /**
     * Return the third byte in the Extended Capabilities information element.
     *
     * \return the third byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte3() const;
    /**
     * Return the fourth byte in the Extended Capabilities information element.
     *
     * \return the fourth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte4() const;
    /**
     * Return the fifth byte in the Extended Capabilities information element.
     *
     * \return the fifth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte5() const;
    /**
     * Return the sixth byte in the Extended Capabilities information element.
     *
     * \return the sixth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte6() const;
    /**
     * Return the seventh byte in the Extended Capabilities information element.
     *
     * \return the seventh byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte7() const;
    /**
     * Return the eighth byte in the Extended Capabilities information element.
     *
     * \return the eighth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte8() const;

  private:
    // fields if HT supported
    uint8_t m_20_40_bssCoexistenceManagementSupport; ///< 20/40 BSS Coexistence Management Support
    uint8_t m_extendedChannelSwitching;              ///< Extended Channel Switching
    uint8_t m_psmpCapability;                        ///< PSMP Capability
    uint8_t m_spsmpSupport;                          ///< S-PSMP Support

    uint8_t m_event;                           ///< Event
    uint8_t m_diagnostics;                     ///< Diagnostics
    uint8_t m_multicastDiagnostics;            ///< Multicast Diagnostics
    uint8_t m_locationTracking;                ///< Location Tracking
    uint8_t m_fms;                             ///< FMS
    uint8_t m_proxyArpService;                 ///< Proxy ARP Service
    uint8_t m_collocatedInterferenceReporting; ///< Collocated Interference Reporting
    uint8_t m_civicLocation;                   ///< Civic Location
    uint8_t m_geospatialLocation;              ///< Geospatial Location

    uint8_t m_tfs;                  ///< TFS
    uint8_t m_wnmSleepMode;         ///< WNM Sleep Mode
    uint8_t m_timBroadcast;         ///< TIM Broadcast
    uint8_t m_bssTransition;        ///< BSS Transition
    uint8_t m_qosTrafficCapability; ///< QoS Traffic Capability
    uint8_t m_acStationCount;       ///< AC Station Count
    uint8_t m_multipleBssid;        ///< Multiple BSSID
    uint8_t m_timingMeasurement;    ///< Timing Measurement

    uint8_t m_channelUsage;         ///< Channel Usage
    uint8_t m_ssidList;             ///< SSID List
    uint8_t m_dms;                  ///< DMS
    uint8_t m_utcTsfOffset;         ///< UTC TSF Offset
    uint8_t m_tpuBufferStaSupport;  ///< TPU Buffer STA Support
    uint8_t m_tdlsPeerPsmSupport;   ///< TDLS Peer PSM Support
    uint8_t m_tdlsChannelSwitching; ///< TDLS Channel Switching
    uint8_t m_interworking;         ///< Interworking

    uint8_t m_qosMap;                         ///< QoS Map
    uint8_t m_ebr;                            ///< EBR
    uint8_t m_sspnInterface;                  ///< SSPN Interface
    uint8_t m_msgcfCapability;                ///< MSGCF Capability
    uint8_t m_tdlsSupport;                    ///< TDLS Support
    uint8_t m_tdlsProhibited;                 ///< TDLS Prohibited
    uint8_t m_tdlsChannelSwitchingProhibited; ///< TDLS Channel Switching Prohibited

    uint8_t m_rejectUnadmittedFrame;      ///< Reject Unadmitted Frame
    uint8_t m_serviceIntervalGranularity; ///< Service Interval Granularity
    uint8_t m_identifierLocation;         ///< Identifier Location
    uint8_t m_uapsdCoexistence;           ///< U-APSD Coexistence
    uint8_t m_wnmNotification;            ///< WNM Notification
    uint8_t m_qabCapability;              ///< QAB Capability

    uint8_t m_utf8Ssid;                    ///< UTF-8 SSID
    uint8_t m_qmfActivated;                ///< QMFActivated
    uint8_t m_qmfReconfigurationActivated; ///< QMFReconfigurationActivated
    uint8_t m_robustAvStreaming;           ///< Robust AV Streaming
    uint8_t m_advancedGcr;                 ///< Advanced GCR
    uint8_t m_meshGcr;                     ///< Mesh GCR
    uint8_t m_scs;                         ///< SCS
    uint8_t m_qloadReport;                 ///< QLoad Report

    uint8_t m_alternateEdca;              ///< Alternate EDCA
    uint8_t m_unprotectedTxopNegotiation; ///< Unprotected TXOP Negotiation
    uint8_t m_protectedTxopNegotiation;   ///< Protected TXOP Negotiation
    uint8_t m_protectedQloadReport;       ///< Protected QLoad Report
    uint8_t m_tdlsWiderBandwidth;         ///< TDLS Wider Bandwidth
    uint8_t m_operatingModeNotification;  ///< Operating Mode Notification
    uint8_t m_maxNumberOfMsdusInAmsdu;    ///< Max Number Of MSDUs In A-MSDU
};

} // namespace ns3

#endif /* EXTENDED_CAPABILITIES_H */
