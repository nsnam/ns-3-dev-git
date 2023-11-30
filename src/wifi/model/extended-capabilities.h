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
 * @brief The Extended Capabilities Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise the Extended Capabilities Information Element
 */
class ExtendedCapabilities : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;

    uint8_t m_20_40_bssCoexistenceManagementSupport : 1 {
        0};                                            ///< 20/40 BSS Coexistence Management Support
    uint8_t m_extendedChannelSwitching : 1 {0};        ///< Extended Channel Switching
    uint8_t m_psmpCapability : 1 {0};                  ///< PSMP Capability
    uint8_t m_spsmpSupport : 1 {0};                    ///< S-PSMP Support
    uint8_t m_event : 1 {0};                           ///< Event
    uint8_t m_diagnostics : 1 {0};                     ///< Diagnostics
    uint8_t m_multicastDiagnostics : 1 {0};            ///< Multicast Diagnostics
    uint8_t m_locationTracking : 1 {0};                ///< Location Tracking
    uint8_t m_fms : 1 {0};                             ///< FMS
    uint8_t m_proxyArpService : 1 {0};                 ///< Proxy ARP Service
    uint8_t m_collocatedInterferenceReporting : 1 {0}; ///< Collocated Interference Reporting
    uint8_t m_civicLocation : 1 {0};                   ///< Civic Location
    uint8_t m_geospatialLocation : 1 {0};              ///< Geospatial Location
    uint8_t m_tfs : 1 {0};                             ///< TFS
    uint8_t m_wnmSleepMode : 1 {0};                    ///< WNM Sleep Mode
    uint8_t m_timBroadcast : 1 {0};                    ///< TIM Broadcast
    uint8_t m_bssTransition : 1 {0};                   ///< BSS Transition
    uint8_t m_qosTrafficCapability : 1 {0};            ///< QoS Traffic Capability
    uint8_t m_acStationCount : 1 {0};                  ///< AC Station Count
    uint8_t m_multipleBssid : 1 {0};                   ///< Multiple BSSID
    uint8_t m_timingMeasurement : 1 {0};               ///< Timing Measurement
    uint8_t m_channelUsage : 1 {0};                    ///< Channel Usage
    uint8_t m_ssidList : 1 {0};                        ///< SSID List
    uint8_t m_dms : 1 {0};                             ///< DMS
    uint8_t m_utcTsfOffset : 1 {0};                    ///< UTC TSF Offset
    uint8_t m_tpuBufferStaSupport : 1 {0};             ///< TPU Buffer STA Support
    uint8_t m_tdlsPeerPsmSupport : 1 {0};              ///< TDLS Peer PSM Support
    uint8_t m_tdlsChannelSwitching : 1 {0};            ///< TDLS Channel Switching
    uint8_t m_interworking : 1 {0};                    ///< Interworking
    uint8_t m_qosMap : 1 {0};                          ///< QoS Map
    uint8_t m_ebr : 1 {0};                             ///< EBR
    uint8_t m_sspnInterface : 1 {0};                   ///< SSPN Interface
    uint8_t m_msgcfCapability : 1 {0};                 ///< MSGCF Capability
    uint8_t m_tdlsSupport : 1 {0};                     ///< TDLS Support
    uint8_t m_tdlsProhibited : 1 {0};                  ///< TDLS Prohibited
    uint8_t m_tdlsChannelSwitchingProhibited : 1 {0};  ///< TDLS Channel Switching Prohibited
    uint8_t m_rejectUnadmittedFrame : 1 {0};           ///< Reject Unadmitted Frame
    uint8_t m_serviceIntervalGranularity : 3 {0};      ///< Service Interval Granularity
    uint8_t m_identifierLocation : 1 {0};              ///< Identifier Location
    uint8_t m_uapsdCoexistence : 1 {0};                ///< U-APSD Coexistence
    uint8_t m_wnmNotification : 1 {0};                 ///< WNM Notification
    uint8_t m_qabCapability : 1 {0};                   ///< QAB Capability
    uint8_t m_utf8Ssid : 1 {0};                        ///< UTF-8 SSID
    uint8_t m_qmfActivated : 1 {0};                    ///< QMFActivated
    uint8_t m_qmfReconfigurationActivated : 1 {0};     ///< QMFReconfigurationActivated
    uint8_t m_robustAvStreaming : 1 {0};               ///< Robust AV Streaming
    uint8_t m_advancedGcr : 1 {0};                     ///< Advanced GCR
    uint8_t m_meshGcr : 1 {0};                         ///< Mesh GCR
    uint8_t m_scs : 1 {0};                             ///< SCS
    uint8_t m_qloadReport : 1 {0};                     ///< QLoad Report
    uint8_t m_alternateEdca : 1 {0};                   ///< Alternate EDCA
    uint8_t m_unprotectedTxopNegotiation : 1 {0};      ///< Unprotected TXOP Negotiation
    uint8_t m_protectedTxopNegotiation : 1 {0};        ///< Protected TXOP Negotiation
    uint8_t m_protectedQloadReport : 1 {0};            ///< Protected QLoad Report
    uint8_t m_tdlsWiderBandwidth : 1 {0};              ///< TDLS Wider Bandwidth
    uint8_t m_operatingModeNotification : 1 {0};       ///< Operating Mode Notification
    uint8_t m_maxNumberOfMsdusInAmsdu : 2 {0};         ///< Max Number Of MSDUs In A-MSDU
};

} // namespace ns3

#endif /* EXTENDED_CAPABILITIES_H */
