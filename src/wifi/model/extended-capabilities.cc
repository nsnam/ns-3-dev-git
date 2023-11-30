/*
 * Copyright (c) 2017
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "extended-capabilities.h"

namespace ns3
{

WifiInformationElementId
ExtendedCapabilities::ElementId() const
{
    return IE_EXTENDED_CAPABILITIES;
}

void
ExtendedCapabilities::Print(std::ostream& os) const
{
    os << "Extended Capabilities=";
    // TODO: print useful capabilities
}

uint16_t
ExtendedCapabilities::GetInformationFieldSize() const
{
    return 8;
}

void
ExtendedCapabilities::SerializeInformationField(Buffer::Iterator start) const
{
    uint8_t byte = 0;
    byte |= m_20_40_bssCoexistenceManagementSupport & 0x01;
    byte |= (m_extendedChannelSwitching & 0x01) << 2;
    byte |= (m_psmpCapability & 0x01) << 4;
    byte |= (m_spsmpSupport & 0x01) << 6;
    byte |= (m_event & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_diagnostics & 0x01;
    byte |= (m_multicastDiagnostics & 0x01) << 1;
    byte |= (m_locationTracking & 0x01) << 2;
    byte |= (m_fms & 0x01) << 3;
    byte |= (m_proxyArpService & 0x01) << 4;
    byte |= (m_collocatedInterferenceReporting & 0x01) << 5;
    byte |= (m_civicLocation & 0x01) << 6;
    byte |= (m_geospatialLocation & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_tfs & 0x01;
    byte |= (m_wnmSleepMode & 0x01) << 1;
    byte |= (m_timBroadcast & 0x01) << 2;
    byte |= (m_bssTransition & 0x01) << 3;
    byte |= (m_qosTrafficCapability & 0x01) << 4;
    byte |= (m_acStationCount & 0x01) << 5;
    byte |= (m_multipleBssid & 0x01) << 6;
    byte |= (m_timingMeasurement & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_channelUsage & 0x01;
    byte |= (m_ssidList & 0x01) << 1;
    byte |= (m_dms & 0x01) << 2;
    byte |= (m_utcTsfOffset & 0x01) << 3;
    byte |= (m_tpuBufferStaSupport & 0x01) << 4;
    byte |= (m_tdlsPeerPsmSupport & 0x01) << 5;
    byte |= (m_tdlsChannelSwitching & 0x01) << 6;
    byte |= (m_interworking & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_qosMap & 0x01;
    byte |= (m_ebr & 0x01) << 1;
    byte |= (m_sspnInterface & 0x01) << 2;
    byte |= (m_msgcfCapability & 0x01) << 4;
    byte |= (m_tdlsSupport & 0x01) << 5;
    byte |= (m_tdlsProhibited & 0x01) << 6;
    byte |= (m_tdlsChannelSwitchingProhibited & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_rejectUnadmittedFrame & 0x01;
    byte |= (m_serviceIntervalGranularity & 0x07) << 1;
    byte |= (m_identifierLocation & 0x01) << 4;
    byte |= (m_uapsdCoexistence & 0x01) << 5;
    byte |= (m_wnmNotification & 0x01) << 6;
    byte |= (m_qabCapability & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_utf8Ssid & 0x01;
    byte |= (m_qmfActivated & 0x01) << 1;
    byte |= (m_qmfReconfigurationActivated & 0x01) << 2;
    byte |= (m_robustAvStreaming & 0x01) << 3;
    byte |= (m_advancedGcr & 0x01) << 4;
    byte |= (m_meshGcr & 0x01) << 5;
    byte |= (m_scs & 0x01) << 6;
    byte |= (m_qloadReport & 0x01) << 7;
    start.WriteU8(byte);

    byte = 0;
    byte |= m_alternateEdca & 0x01;
    byte |= (m_unprotectedTxopNegotiation & 0x01) << 1;
    byte |= (m_protectedTxopNegotiation & 0x01) << 2;
    byte |= (m_protectedQloadReport & 0x01) << 3;
    byte |= (m_tdlsWiderBandwidth & 0x01) << 4;
    byte |= (m_operatingModeNotification & 0x01) << 5;
    byte |= (m_maxNumberOfMsdusInAmsdu & 0x03) << 6;
    start.WriteU8(byte);
}

uint16_t
ExtendedCapabilities::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;

    auto byte = i.ReadU8();
    m_20_40_bssCoexistenceManagementSupport = byte & 0x01;
    m_extendedChannelSwitching = (byte >> 2) & 0x01;
    m_psmpCapability = (byte >> 4) & 0x01;
    m_spsmpSupport = (byte >> 6) & 0x01;
    m_event = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_diagnostics = byte & 0x01;
    m_multicastDiagnostics = (byte >> 1) & 0x01;
    m_locationTracking = (byte >> 2) & 0x01;
    m_fms = (byte >> 3) & 0x01;
    m_proxyArpService = (byte >> 4) & 0x01;
    m_collocatedInterferenceReporting = (byte >> 5) & 0x01;
    m_civicLocation = (byte >> 6) & 0x01;
    m_geospatialLocation = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_tfs = byte & 0x01;
    m_wnmSleepMode = (byte >> 1) & 0x01;
    m_timBroadcast = (byte >> 2) & 0x01;
    m_bssTransition = (byte >> 3) & 0x01;
    m_qosTrafficCapability = (byte >> 4) & 0x01;
    m_acStationCount = (byte >> 5) & 0x01;
    m_multipleBssid = (byte >> 6) & 0x01;
    m_timingMeasurement = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_channelUsage = byte & 0x01;
    m_ssidList = (byte >> 1) & 0x01;
    m_dms = (byte >> 2) & 0x01;
    m_utcTsfOffset = (byte >> 3) & 0x01;
    m_tpuBufferStaSupport = (byte >> 4) & 0x01;
    m_tdlsPeerPsmSupport = (byte >> 5) & 0x01;
    m_tdlsChannelSwitching = (byte >> 6) & 0x01;
    m_interworking = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_qosMap = byte & 0x01;
    m_ebr = (byte >> 1) & 0x01;
    m_sspnInterface = (byte >> 2) & 0x01;
    m_msgcfCapability = (byte >> 4) & 0x01;
    m_tdlsSupport = (byte >> 5) & 0x01;
    m_tdlsProhibited = (byte >> 6) & 0x01;
    m_tdlsChannelSwitchingProhibited = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_rejectUnadmittedFrame = byte & 0x01;
    m_serviceIntervalGranularity = (byte >> 1) & 0x07;
    m_identifierLocation = (byte >> 4) & 0x01;
    m_uapsdCoexistence = (byte >> 5) & 0x01;
    m_wnmNotification = (byte >> 6) & 0x01;
    m_qabCapability = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_utf8Ssid = byte & 0x01;
    m_qmfActivated = (byte >> 1) & 0x01;
    m_qmfReconfigurationActivated = (byte >> 2) & 0x01;
    m_robustAvStreaming = (byte >> 3) & 0x01;
    m_advancedGcr = (byte >> 4) & 0x01;
    m_meshGcr = (byte >> 5) & 0x01;
    m_scs = (byte >> 6) & 0x01;
    m_qloadReport = (byte >> 7) & 0x01;

    byte = i.ReadU8();
    m_alternateEdca = byte & 0x01;
    m_unprotectedTxopNegotiation = (byte >> 1) & 0x01;
    m_protectedTxopNegotiation = (byte >> 2) & 0x01;
    m_protectedQloadReport = (byte >> 3) & 0x01;
    m_tdlsWiderBandwidth = (byte >> 4) & 0x01;
    m_operatingModeNotification = (byte >> 5) & 0x01;
    m_maxNumberOfMsdusInAmsdu = (byte >> 6) & 0x03;

    return length;
}

} // namespace ns3
