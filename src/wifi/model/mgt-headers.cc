/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "mgt-headers.h"

#include "ns3/address-utils.h"
#include "ns3/simulator.h"

namespace ns3
{

/***********************************************************
 *          Probe Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeRequestHeader);

MgtProbeRequestHeader::~MgtProbeRequestHeader()
{
}

void
MgtProbeRequestHeader::SetSsid(const Ssid& ssid)
{
    m_ssid = ssid;
}

void
MgtProbeRequestHeader::SetSsid(Ssid&& ssid)
{
    m_ssid = std::move(ssid);
}

const Ssid&
MgtProbeRequestHeader::GetSsid() const
{
    return m_ssid;
}

void
MgtProbeRequestHeader::SetSupportedRates(const SupportedRates& rates)
{
    m_rates = rates;
}

void
MgtProbeRequestHeader::SetSupportedRates(SupportedRates&& rates)
{
    m_rates = std::move(rates);
}

void
MgtProbeRequestHeader::SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities)
{
    m_extendedCapability = extendedCapabilities;
}

void
MgtProbeRequestHeader::SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities)
{
    m_extendedCapability = std::move(extendedCapabilities);
}

const std::optional<ExtendedCapabilities>&
MgtProbeRequestHeader::GetExtendedCapabilities() const
{
    return m_extendedCapability;
}

void
MgtProbeRequestHeader::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapability = htCapabilities;
}

void
MgtProbeRequestHeader::SetHtCapabilities(HtCapabilities&& htCapabilities)
{
    m_htCapability = std::move(htCapabilities);
}

const std::optional<HtCapabilities>&
MgtProbeRequestHeader::GetHtCapabilities() const
{
    return m_htCapability;
}

void
MgtProbeRequestHeader::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapability = vhtCapabilities;
}

void
MgtProbeRequestHeader::SetVhtCapabilities(VhtCapabilities&& vhtCapabilities)
{
    m_vhtCapability = std::move(vhtCapabilities);
}

const std::optional<VhtCapabilities>&
MgtProbeRequestHeader::GetVhtCapabilities() const
{
    return m_vhtCapability;
}

void
MgtProbeRequestHeader::SetHeCapabilities(const HeCapabilities& heCapabilities)
{
    m_heCapability = heCapabilities;
}

void
MgtProbeRequestHeader::SetHeCapabilities(HeCapabilities&& heCapabilities)
{
    m_heCapability = std::move(heCapabilities);
}

const std::optional<HeCapabilities>&
MgtProbeRequestHeader::GetHeCapabilities() const
{
    return m_heCapability;
}

void
MgtProbeRequestHeader::SetEhtCapabilities(const EhtCapabilities& ehtCapabilities)
{
    m_ehtCapability = ehtCapabilities;
}

void
MgtProbeRequestHeader::SetEhtCapabilities(EhtCapabilities&& ehtCapabilities)
{
    m_ehtCapability = std::move(ehtCapabilities);
}

const std::optional<EhtCapabilities>&
MgtProbeRequestHeader::GetEhtCapabilities() const
{
    return m_ehtCapability;
}

const SupportedRates&
MgtProbeRequestHeader::GetSupportedRates() const
{
    return m_rates;
}

uint32_t
MgtProbeRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += m_ssid.GetSerializedSize();
    size += m_rates.GetSerializedSize();
    if (m_rates.GetNRates() > 8)
    {
        size += m_rates.extended->GetSerializedSize();
    }
    if (m_extendedCapability.has_value())
    {
        size += m_extendedCapability->GetSerializedSize();
    }
    if (m_htCapability.has_value())
    {
        size += m_htCapability->GetSerializedSize();
    }
    if (m_vhtCapability.has_value())
    {
        size += m_vhtCapability->GetSerializedSize();
    }
    if (m_heCapability.has_value())
    {
        size += m_heCapability->GetSerializedSize();
    }
    if (m_ehtCapability.has_value())
    {
        size += m_ehtCapability->GetSerializedSize();
    }
    return size;
}

TypeId
MgtProbeRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeRequestHeader>();
    return tid;
}

TypeId
MgtProbeRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtProbeRequestHeader::Print(std::ostream& os) const
{
    os << "ssid=" << m_ssid << ", "
       << "rates=" << m_rates << ", ";
    if (m_extendedCapability.has_value())
    {
        os << "Extended Capabilities=" << *m_extendedCapability << " , ";
    }
    if (m_htCapability.has_value())
    {
        os << "HT Capabilities=" << *m_htCapability << " , ";
    }
    if (m_vhtCapability.has_value())
    {
        os << "VHT Capabilities=" << *m_vhtCapability << " , ";
    }
    if (m_heCapability.has_value())
    {
        os << "HE Capabilities=" << *m_heCapability << " , ";
    }
    if (m_ehtCapability.has_value())
    {
        os << "EHT Capabilities=" << *m_ehtCapability;
    }
}

void
MgtProbeRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_ssid.Serialize(i);
    i = m_rates.Serialize(i);
    if (m_rates.GetNRates() > 8)
    {
        i = m_rates.extended->Serialize(i);
    }
    if (m_extendedCapability.has_value())
    {
        i = m_extendedCapability->Serialize(i);
    }
    if (m_htCapability.has_value())
    {
        i = m_htCapability->Serialize(i);
    }
    if (m_vhtCapability.has_value())
    {
        i = m_vhtCapability->Serialize(i);
    }
    if (m_heCapability.has_value())
    {
        i = m_heCapability->Serialize(i);
    }
    if (m_ehtCapability.has_value())
    {
        i = m_ehtCapability->Serialize(i);
    }
}

uint32_t
MgtProbeRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_ssid.Deserialize(i);
    i = m_rates.Deserialize(i);
    i = WifiInformationElement::DeserializeIfPresent(m_rates.extended, i, &m_rates);
    i = WifiInformationElement::DeserializeIfPresent(m_extendedCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heCapability, i);
    const bool is2_4Ghz = m_rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    i = WifiInformationElement::DeserializeIfPresent(m_ehtCapability, i, is2_4Ghz, m_heCapability);
    return i.GetDistanceFrom(start);
}

/***********************************************************
 *          Probe Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeResponseHeader);

MgtProbeResponseHeader::MgtProbeResponseHeader()
{
}

MgtProbeResponseHeader::~MgtProbeResponseHeader()
{
}

uint64_t
MgtProbeResponseHeader::GetTimestamp() const
{
    return m_timestamp;
}

const Ssid&
MgtProbeResponseHeader::GetSsid() const
{
    return m_ssid;
}

uint64_t
MgtProbeResponseHeader::GetBeaconIntervalUs() const
{
    return m_beaconInterval;
}

const SupportedRates&
MgtProbeResponseHeader::GetSupportedRates() const
{
    return m_rates;
}

void
MgtProbeResponseHeader::SetCapabilities(const CapabilityInformation& capabilities)
{
    m_capability = capabilities;
}

void
MgtProbeResponseHeader::SetCapabilities(CapabilityInformation&& capabilities)
{
    m_capability = std::move(capabilities);
}

const CapabilityInformation&
MgtProbeResponseHeader::GetCapabilities() const
{
    return m_capability;
}

void
MgtProbeResponseHeader::SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities)
{
    m_extendedCapability = extendedCapabilities;
}

void
MgtProbeResponseHeader::SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities)
{
    m_extendedCapability = std::move(extendedCapabilities);
}

const std::optional<ExtendedCapabilities>&
MgtProbeResponseHeader::GetExtendedCapabilities() const
{
    return m_extendedCapability;
}

void
MgtProbeResponseHeader::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapability = htCapabilities;
}

void
MgtProbeResponseHeader::SetHtCapabilities(HtCapabilities&& htCapabilities)
{
    m_htCapability = std::move(htCapabilities);
}

const std::optional<HtCapabilities>&
MgtProbeResponseHeader::GetHtCapabilities() const
{
    return m_htCapability;
}

void
MgtProbeResponseHeader::SetHtOperation(const HtOperation& htOperation)
{
    m_htOperation = htOperation;
}

void
MgtProbeResponseHeader::SetHtOperation(HtOperation&& htOperation)
{
    m_htOperation = std::move(htOperation);
}

const std::optional<HtOperation>&
MgtProbeResponseHeader::GetHtOperation() const
{
    return m_htOperation;
}

void
MgtProbeResponseHeader::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapability = vhtCapabilities;
}

void
MgtProbeResponseHeader::SetVhtCapabilities(VhtCapabilities&& vhtCapabilities)
{
    m_vhtCapability = std::move(vhtCapabilities);
}

const std::optional<VhtCapabilities>&
MgtProbeResponseHeader::GetVhtCapabilities() const
{
    return m_vhtCapability;
}

void
MgtProbeResponseHeader::SetVhtOperation(const VhtOperation& vhtOperation)
{
    m_vhtOperation = vhtOperation;
}

void
MgtProbeResponseHeader::SetVhtOperation(VhtOperation&& vhtOperation)
{
    m_vhtOperation = std::move(vhtOperation);
}

const std::optional<VhtOperation>&
MgtProbeResponseHeader::GetVhtOperation() const
{
    return m_vhtOperation;
}

void
MgtProbeResponseHeader::SetHeCapabilities(const HeCapabilities& heCapabilities)
{
    m_heCapability = heCapabilities;
}

void
MgtProbeResponseHeader::SetHeCapabilities(HeCapabilities&& heCapabilities)
{
    m_heCapability = std::move(heCapabilities);
}

const std::optional<HeCapabilities>&
MgtProbeResponseHeader::GetHeCapabilities() const
{
    return m_heCapability;
}

void
MgtProbeResponseHeader::SetHeOperation(const HeOperation& heOperation)
{
    m_heOperation = heOperation;
}

void
MgtProbeResponseHeader::SetHeOperation(HeOperation&& heOperation)
{
    m_heOperation = std::move(heOperation);
}

const std::optional<HeOperation>&
MgtProbeResponseHeader::GetHeOperation() const
{
    return m_heOperation;
}

void
MgtProbeResponseHeader::SetEhtCapabilities(const EhtCapabilities& ehtCapabilities)
{
    m_ehtCapability = ehtCapabilities;
}

void
MgtProbeResponseHeader::SetEhtCapabilities(EhtCapabilities&& ehtCapabilities)
{
    m_ehtCapability = std::move(ehtCapabilities);
}

const std::optional<EhtCapabilities>&
MgtProbeResponseHeader::GetEhtCapabilities() const
{
    return m_ehtCapability;
}

void
MgtProbeResponseHeader::SetEhtOperation(const EhtOperation& ehtOperation)
{
    m_ehtOperation = ehtOperation;
}

void
MgtProbeResponseHeader::SetEhtOperation(EhtOperation&& ehtOperation)
{
    m_ehtOperation = std::move(ehtOperation);
}

const std::optional<EhtOperation>&
MgtProbeResponseHeader::GetEhtOperation() const
{
    return m_ehtOperation;
}

void
MgtProbeResponseHeader::SetSsid(const Ssid& ssid)
{
    m_ssid = ssid;
}

void
MgtProbeResponseHeader::SetSsid(Ssid&& ssid)
{
    m_ssid = std::move(ssid);
}

void
MgtProbeResponseHeader::SetBeaconIntervalUs(uint64_t us)
{
    m_beaconInterval = us;
}

void
MgtProbeResponseHeader::SetSupportedRates(const SupportedRates& rates)
{
    m_rates = rates;
}

void
MgtProbeResponseHeader::SetSupportedRates(SupportedRates&& rates)
{
    m_rates = std::move(rates);
}

void
MgtProbeResponseHeader::SetDsssParameterSet(const DsssParameterSet& dsssParameterSet)
{
    m_dsssParameterSet = dsssParameterSet;
}

void
MgtProbeResponseHeader::SetDsssParameterSet(DsssParameterSet&& dsssParameterSet)
{
    m_dsssParameterSet = std::move(dsssParameterSet);
}

const std::optional<DsssParameterSet>&
MgtProbeResponseHeader::GetDsssParameterSet() const
{
    return m_dsssParameterSet;
}

void
MgtProbeResponseHeader::SetErpInformation(const ErpInformation& erpInformation)
{
    m_erpInformation = erpInformation;
}

void
MgtProbeResponseHeader::SetErpInformation(ErpInformation&& erpInformation)
{
    m_erpInformation = std::move(erpInformation);
}

const std::optional<ErpInformation>&
MgtProbeResponseHeader::GetErpInformation() const
{
    return m_erpInformation;
}

void
MgtProbeResponseHeader::SetEdcaParameterSet(const EdcaParameterSet& edcaParameters)
{
    m_edcaParameterSet = edcaParameters;
}

void
MgtProbeResponseHeader::SetEdcaParameterSet(EdcaParameterSet&& edcaParameters)
{
    m_edcaParameterSet = std::move(edcaParameters);
}

void
MgtProbeResponseHeader::SetMuEdcaParameterSet(const MuEdcaParameterSet& muEdcaParameters)
{
    m_muEdcaParameterSet = muEdcaParameters;
}

void
MgtProbeResponseHeader::SetMuEdcaParameterSet(MuEdcaParameterSet&& muEdcaParameters)
{
    m_muEdcaParameterSet = std::move(muEdcaParameters);
}

void
MgtProbeResponseHeader::SetReducedNeighborReport(const ReducedNeighborReport& reducedNeighborReport)
{
    m_reducedNeighborReport = reducedNeighborReport;
}

void
MgtProbeResponseHeader::SetReducedNeighborReport(ReducedNeighborReport&& reducedNeighborReport)
{
    m_reducedNeighborReport = std::move(reducedNeighborReport);
}

void
MgtProbeResponseHeader::SetMultiLinkElement(const MultiLinkElement& multiLinkElement)
{
    m_multiLinkElement = multiLinkElement;
}

void
MgtProbeResponseHeader::SetMultiLinkElement(MultiLinkElement&& multiLinkElement)
{
    m_multiLinkElement = std::move(multiLinkElement);
}

const std::optional<EdcaParameterSet>&
MgtProbeResponseHeader::GetEdcaParameterSet() const
{
    return m_edcaParameterSet;
}

const std::optional<MuEdcaParameterSet>&
MgtProbeResponseHeader::GetMuEdcaParameterSet() const
{
    return m_muEdcaParameterSet;
}

const std::optional<ReducedNeighborReport>&
MgtProbeResponseHeader::GetReducedNeighborReport() const
{
    return m_reducedNeighborReport;
}

const std::optional<MultiLinkElement>&
MgtProbeResponseHeader::GetMultiLinkElement() const
{
    return m_multiLinkElement;
}

TypeId
MgtProbeResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeResponseHeader>();
    return tid;
}

TypeId
MgtProbeResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtProbeResponseHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 8; // timestamp
    size += 2; // beacon interval
    size += m_capability.GetSerializedSize();
    size += m_ssid.GetSerializedSize();
    size += m_rates.GetSerializedSize();
    if (m_dsssParameterSet.has_value())
    {
        size += m_dsssParameterSet->GetSerializedSize();
    }
    if (m_erpInformation.has_value())
    {
        size += m_erpInformation->GetSerializedSize();
    }
    if (m_rates.GetNRates() > 8)
    {
        size += m_rates.extended->GetSerializedSize();
    }
    if (m_edcaParameterSet.has_value())
    {
        size += m_edcaParameterSet->GetSerializedSize();
    }
    if (m_extendedCapability.has_value())
    {
        size += m_extendedCapability->GetSerializedSize();
    }
    if (m_htCapability.has_value())
    {
        size += m_htCapability->GetSerializedSize();
    }
    if (m_htOperation.has_value())
    {
        size += m_htOperation->GetSerializedSize();
    }
    if (m_vhtCapability.has_value())
    {
        size += m_vhtCapability->GetSerializedSize();
    }
    if (m_vhtOperation.has_value())
    {
        size += m_vhtOperation->GetSerializedSize();
    }
    if (m_reducedNeighborReport.has_value())
    {
        size += m_reducedNeighborReport->GetSerializedSize();
    }
    if (m_heCapability.has_value())
    {
        size += m_heCapability->GetSerializedSize();
    }
    if (m_heOperation.has_value())
    {
        size += m_heOperation->GetSerializedSize();
    }
    if (m_muEdcaParameterSet.has_value())
    {
        size += m_muEdcaParameterSet->GetSerializedSize();
    }
    if (m_multiLinkElement.has_value())
    {
        size += m_multiLinkElement->GetSerializedSize();
    }
    if (m_ehtCapability.has_value())
    {
        size += m_ehtCapability->GetSerializedSize();
    }
    if (m_ehtOperation.has_value())
    {
        size += m_ehtOperation->GetSerializedSize();
    }
    return size;
}

void
MgtProbeResponseHeader::Print(std::ostream& os) const
{
    os << "ssid=" << m_ssid << ", "
       << "rates=" << m_rates << ", ";
    if (m_erpInformation.has_value())
    {
        os << "ERP information=" << *m_erpInformation << ", ";
    }
    if (m_extendedCapability.has_value())
    {
        os << "Extended Capabilities=" << *m_extendedCapability << " , ";
    }
    if (m_htCapability.has_value())
    {
        os << "HT Capabilities=" << *m_htCapability << " , ";
    }
    if (m_htOperation.has_value())
    {
        os << "HT Operation=" << *m_htOperation << " , ";
    }
    if (m_vhtCapability.has_value())
    {
        os << "VHT Capabilities=" << *m_vhtCapability << " , ";
    }
    if (m_vhtOperation.has_value())
    {
        os << "VHT Operation=" << *m_vhtOperation << " , ";
    }
    if (m_heCapability.has_value())
    {
        os << "HE Capabilities=" << *m_heCapability << " , ";
    }
    if (m_heOperation.has_value())
    {
        os << "HE Operation=" << *m_heOperation << " , ";
    }
    if (m_ehtCapability.has_value())
    {
        os << "EHT Capabilities=" << *m_ehtCapability;
    }
    if (m_ehtOperation.has_value())
    {
        os << "EHT Operation=" << *m_ehtOperation;
    }
}

void
MgtProbeResponseHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU64(Simulator::Now().GetMicroSeconds());
    i.WriteHtolsbU16(static_cast<uint16_t>(m_beaconInterval / 1024));
    i = m_capability.Serialize(i);
    i = m_ssid.Serialize(i);
    i = m_rates.Serialize(i);
    if (m_dsssParameterSet.has_value())
    {
        i = m_dsssParameterSet->Serialize(i);
    }
    if (m_erpInformation.has_value())
    {
        i = m_erpInformation->Serialize(i);
    }
    if (m_rates.GetNRates() > 8)
    {
        i = m_rates.extended->Serialize(i);
    }
    if (m_edcaParameterSet.has_value())
    {
        i = m_edcaParameterSet->Serialize(i);
    }
    if (m_extendedCapability.has_value())
    {
        i = m_extendedCapability->Serialize(i);
    }
    if (m_htCapability.has_value())
    {
        i = m_htCapability->Serialize(i);
    }
    if (m_htOperation.has_value())
    {
        i = m_htOperation->Serialize(i);
    }
    if (m_vhtCapability.has_value())
    {
        i = m_vhtCapability->Serialize(i);
    }
    if (m_vhtOperation.has_value())
    {
        i = m_vhtOperation->Serialize(i);
    }
    if (m_reducedNeighborReport.has_value())
    {
        i = m_reducedNeighborReport->Serialize(i);
    }
    if (m_heCapability.has_value())
    {
        i = m_heCapability->Serialize(i);
    }
    if (m_heOperation.has_value())
    {
        i = m_heOperation->Serialize(i);
    }
    if (m_muEdcaParameterSet.has_value())
    {
        i = m_muEdcaParameterSet->Serialize(i);
    }
    if (m_multiLinkElement.has_value())
    {
        i = m_multiLinkElement->Serialize(i);
    }
    if (m_ehtCapability.has_value())
    {
        i = m_ehtCapability->Serialize(i);
    }
    if (m_ehtOperation.has_value())
    {
        i = m_ehtOperation->Serialize(i);
    }
}

uint32_t
MgtProbeResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator tmp;
    Buffer::Iterator i = start;
    m_timestamp = i.ReadLsbtohU64();
    m_beaconInterval = i.ReadLsbtohU16();
    m_beaconInterval *= 1024;
    i = m_capability.Deserialize(i);
    i = m_ssid.Deserialize(i);
    i = m_rates.Deserialize(i);
    i = WifiInformationElement::DeserializeIfPresent(m_dsssParameterSet, i);
    i = WifiInformationElement::DeserializeIfPresent(m_erpInformation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_rates.extended, i, &m_rates);
    i = WifiInformationElement::DeserializeIfPresent(m_edcaParameterSet, i);
    i = WifiInformationElement::DeserializeIfPresent(m_extendedCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_reducedNeighborReport, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_muEdcaParameterSet, i);
    i = WifiInformationElement::DeserializeIfPresent(m_multiLinkElement, i, WIFI_MAC_MGT_BEACON);
    const bool is2_4Ghz = m_rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    i = WifiInformationElement::DeserializeIfPresent(m_ehtCapability, i, is2_4Ghz, m_heCapability);
    i = WifiInformationElement::DeserializeIfPresent(m_ehtOperation, i);

    return i.GetDistanceFrom(start);
}

/***********************************************************
 *          Beacons
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtBeaconHeader);

/* static */
TypeId
MgtBeaconHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtBeaconHeader")
                            .SetParent<MgtProbeResponseHeader>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtBeaconHeader>();
    return tid;
}

/***********************************************************
 *          Assoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocRequestHeader);

MgtAssocRequestHeader::MgtAssocRequestHeader()
    : m_listenInterval(0)
{
}

MgtAssocRequestHeader::~MgtAssocRequestHeader()
{
}

void
MgtAssocRequestHeader::SetSsid(const Ssid& ssid)
{
    m_ssid = ssid;
}

void
MgtAssocRequestHeader::SetSsid(Ssid&& ssid)
{
    m_ssid = std::move(ssid);
}

void
MgtAssocRequestHeader::SetSupportedRates(const SupportedRates& rates)
{
    m_rates = rates;
}

void
MgtAssocRequestHeader::SetSupportedRates(SupportedRates&& rates)
{
    m_rates = std::move(rates);
}

void
MgtAssocRequestHeader::SetListenInterval(uint16_t interval)
{
    m_listenInterval = interval;
}

void
MgtAssocRequestHeader::SetCapabilities(const CapabilityInformation& capabilities)
{
    m_capability = capabilities;
}

void
MgtAssocRequestHeader::SetCapabilities(CapabilityInformation&& capabilities)
{
    m_capability = std::move(capabilities);
}

const CapabilityInformation&
MgtAssocRequestHeader::GetCapabilities() const
{
    return m_capability;
}

void
MgtAssocRequestHeader::SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities)
{
    m_extendedCapability = extendedCapabilities;
}

void
MgtAssocRequestHeader::SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities)
{
    m_extendedCapability = std::move(extendedCapabilities);
}

const std::optional<ExtendedCapabilities>&
MgtAssocRequestHeader::GetExtendedCapabilities() const
{
    return m_extendedCapability;
}

void
MgtAssocRequestHeader::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapability = htCapabilities;
}

void
MgtAssocRequestHeader::SetHtCapabilities(HtCapabilities&& htCapabilities)
{
    m_htCapability = std::move(htCapabilities);
}

const std::optional<HtCapabilities>&
MgtAssocRequestHeader::GetHtCapabilities() const
{
    return m_htCapability;
}

void
MgtAssocRequestHeader::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapability = vhtCapabilities;
}

void
MgtAssocRequestHeader::SetVhtCapabilities(VhtCapabilities&& vhtCapabilities)
{
    m_vhtCapability = std::move(vhtCapabilities);
}

const std::optional<VhtCapabilities>&
MgtAssocRequestHeader::GetVhtCapabilities() const
{
    return m_vhtCapability;
}

void
MgtAssocRequestHeader::SetHeCapabilities(const HeCapabilities& heCapabilities)
{
    m_heCapability = heCapabilities;
}

void
MgtAssocRequestHeader::SetHeCapabilities(HeCapabilities&& heCapabilities)
{
    m_heCapability = std::move(heCapabilities);
}

const std::optional<HeCapabilities>&
MgtAssocRequestHeader::GetHeCapabilities() const
{
    return m_heCapability;
}

void
MgtAssocRequestHeader::SetEhtCapabilities(const EhtCapabilities& ehtCapabilities)
{
    m_ehtCapability = ehtCapabilities;
}

void
MgtAssocRequestHeader::SetEhtCapabilities(EhtCapabilities&& ehtCapabilities)
{
    m_ehtCapability = std::move(ehtCapabilities);
}

const std::optional<EhtCapabilities>&
MgtAssocRequestHeader::GetEhtCapabilities() const
{
    return m_ehtCapability;
}

void
MgtAssocRequestHeader::SetMultiLinkElement(const MultiLinkElement& multiLinkElement)
{
    m_multiLinkElement = multiLinkElement;
}

void
MgtAssocRequestHeader::SetMultiLinkElement(MultiLinkElement&& multiLinkElement)
{
    m_multiLinkElement = std::move(multiLinkElement);
}

const std::optional<MultiLinkElement>&
MgtAssocRequestHeader::GetMultiLinkElement() const
{
    return m_multiLinkElement;
}

const Ssid&
MgtAssocRequestHeader::GetSsid() const
{
    return m_ssid;
}

const SupportedRates&
MgtAssocRequestHeader::GetSupportedRates() const
{
    return m_rates;
}

uint16_t
MgtAssocRequestHeader::GetListenInterval() const
{
    return m_listenInterval;
}

TypeId
MgtAssocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocRequestHeader>();
    return tid;
}

TypeId
MgtAssocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtAssocRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2;
    size += m_ssid.GetSerializedSize();
    size += m_rates.GetSerializedSize();
    if (m_rates.GetNRates() > 8)
    {
        size += m_rates.extended->GetSerializedSize();
    }
    if (m_extendedCapability.has_value())
    {
        size += m_extendedCapability->GetSerializedSize();
    }
    if (m_htCapability.has_value())
    {
        size += m_htCapability->GetSerializedSize();
    }
    if (m_vhtCapability.has_value())
    {
        size += m_vhtCapability->GetSerializedSize();
    }
    if (m_heCapability.has_value())
    {
        size += m_heCapability->GetSerializedSize();
    }
    if (m_multiLinkElement.has_value())
    {
        size += m_multiLinkElement->GetSerializedSize();
    }
    if (m_ehtCapability.has_value())
    {
        size += m_ehtCapability->GetSerializedSize();
    }
    return size;
}

void
MgtAssocRequestHeader::Print(std::ostream& os) const
{
    os << "ssid=" << m_ssid << ", "
       << "rates=" << m_rates << ", ";
    if (m_extendedCapability.has_value())
    {
        os << "Extended Capabilities=" << *m_extendedCapability << " , ";
    }
    if (m_htCapability.has_value())
    {
        os << "HT Capabilities=" << *m_htCapability << " , ";
    }
    if (m_vhtCapability.has_value())
    {
        os << "VHT Capabilities=" << *m_vhtCapability << " , ";
    }
    if (m_heCapability.has_value())
    {
        os << "HE Capabilities=" << *m_heCapability << " , ";
    }
    if (m_ehtCapability.has_value())
    {
        os << "EHT Capabilities=" << *m_ehtCapability;
    }
}

void
MgtAssocRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    i = m_ssid.Serialize(i);
    i = m_rates.Serialize(i);
    if (m_rates.GetNRates() > 8)
    {
        i = m_rates.extended->Serialize(i);
    }
    if (m_extendedCapability.has_value())
    {
        i = m_extendedCapability->Serialize(i);
    }
    if (m_htCapability.has_value())
    {
        i = m_htCapability->Serialize(i);
    }
    if (m_vhtCapability.has_value())
    {
        i = m_vhtCapability->Serialize(i);
    }
    if (m_heCapability.has_value())
    {
        i = m_heCapability->Serialize(i);
    }
    if (m_multiLinkElement.has_value())
    {
        i = m_multiLinkElement->Serialize(i);
    }
    if (m_ehtCapability.has_value())
    {
        i = m_ehtCapability->Serialize(i);
    }
}

uint32_t
MgtAssocRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator tmp;
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    i = m_ssid.Deserialize(i);
    i = m_rates.Deserialize(i);
    i = WifiInformationElement::DeserializeIfPresent(m_rates.extended, i, &m_rates);
    i = WifiInformationElement::DeserializeIfPresent(m_extendedCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_multiLinkElement,
                                                     i,
                                                     WIFI_MAC_MGT_ASSOCIATION_REQUEST);
    const bool is2_4Ghz = m_rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    i = WifiInformationElement::DeserializeIfPresent(m_ehtCapability, i, is2_4Ghz, m_heCapability);
    return i.GetDistanceFrom(start);
}

/***********************************************************
 *          Ressoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtReassocRequestHeader);

MgtReassocRequestHeader::MgtReassocRequestHeader()
    : m_currentApAddr(Mac48Address())
{
}

MgtReassocRequestHeader::~MgtReassocRequestHeader()
{
}

void
MgtReassocRequestHeader::SetSsid(const Ssid& ssid)
{
    m_ssid = ssid;
}

void
MgtReassocRequestHeader::SetSsid(Ssid&& ssid)
{
    m_ssid = std::move(ssid);
}

void
MgtReassocRequestHeader::SetSupportedRates(const SupportedRates& rates)
{
    m_rates = rates;
}

void
MgtReassocRequestHeader::SetSupportedRates(SupportedRates&& rates)
{
    m_rates = std::move(rates);
}

void
MgtReassocRequestHeader::SetListenInterval(uint16_t interval)
{
    m_listenInterval = interval;
}

void
MgtReassocRequestHeader::SetCapabilities(const CapabilityInformation& capabilities)
{
    m_capability = capabilities;
}

void
MgtReassocRequestHeader::SetCapabilities(CapabilityInformation&& capabilities)
{
    m_capability = std::move(capabilities);
}

const CapabilityInformation&
MgtReassocRequestHeader::GetCapabilities() const
{
    return m_capability;
}

void
MgtReassocRequestHeader::SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities)
{
    m_extendedCapability = extendedCapabilities;
}

void
MgtReassocRequestHeader::SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities)
{
    m_extendedCapability = std::move(extendedCapabilities);
}

const std::optional<ExtendedCapabilities>&
MgtReassocRequestHeader::GetExtendedCapabilities() const
{
    return m_extendedCapability;
}

void
MgtReassocRequestHeader::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapability = htCapabilities;
}

void
MgtReassocRequestHeader::SetHtCapabilities(HtCapabilities&& htCapabilities)
{
    m_htCapability = std::move(htCapabilities);
}

const std::optional<HtCapabilities>&
MgtReassocRequestHeader::GetHtCapabilities() const
{
    return m_htCapability;
}

void
MgtReassocRequestHeader::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapability = vhtCapabilities;
}

void
MgtReassocRequestHeader::SetVhtCapabilities(VhtCapabilities&& vhtCapabilities)
{
    m_vhtCapability = std::move(vhtCapabilities);
}

const std::optional<VhtCapabilities>&
MgtReassocRequestHeader::GetVhtCapabilities() const
{
    return m_vhtCapability;
}

void
MgtReassocRequestHeader::SetHeCapabilities(const HeCapabilities& heCapabilities)
{
    m_heCapability = heCapabilities;
}

void
MgtReassocRequestHeader::SetHeCapabilities(HeCapabilities&& heCapabilities)
{
    m_heCapability = std::move(heCapabilities);
}

const std::optional<HeCapabilities>&
MgtReassocRequestHeader::GetHeCapabilities() const
{
    return m_heCapability;
}

void
MgtReassocRequestHeader::SetEhtCapabilities(const EhtCapabilities& ehtCapabilities)
{
    m_ehtCapability = ehtCapabilities;
}

void
MgtReassocRequestHeader::SetEhtCapabilities(EhtCapabilities&& ehtCapabilities)
{
    m_ehtCapability = std::move(ehtCapabilities);
}

const std::optional<EhtCapabilities>&
MgtReassocRequestHeader::GetEhtCapabilities() const
{
    return m_ehtCapability;
}

void
MgtReassocRequestHeader::SetMultiLinkElement(const MultiLinkElement& multiLinkElement)
{
    m_multiLinkElement = multiLinkElement;
}

void
MgtReassocRequestHeader::SetMultiLinkElement(MultiLinkElement&& multiLinkElement)
{
    m_multiLinkElement = std::move(multiLinkElement);
}

const std::optional<MultiLinkElement>&
MgtReassocRequestHeader::GetMultiLinkElement() const
{
    return m_multiLinkElement;
}

const Ssid&
MgtReassocRequestHeader::GetSsid() const
{
    return m_ssid;
}

const SupportedRates&
MgtReassocRequestHeader::GetSupportedRates() const
{
    return m_rates;
}

uint16_t
MgtReassocRequestHeader::GetListenInterval() const
{
    return m_listenInterval;
}

void
MgtReassocRequestHeader::SetCurrentApAddress(Mac48Address currentApAddr)
{
    m_currentApAddr = currentApAddr;
}

TypeId
MgtReassocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtReassocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtReassocRequestHeader>();
    return tid;
}

TypeId
MgtReassocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtReassocRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2; // listen interval
    size += 6; // current AP address
    size += m_ssid.GetSerializedSize();
    size += m_rates.GetSerializedSize();
    if (m_rates.GetNRates() > 8)
    {
        size += m_rates.extended->GetSerializedSize();
    }
    if (m_extendedCapability.has_value())
    {
        size += m_extendedCapability->GetSerializedSize();
    }
    if (m_htCapability.has_value())
    {
        size += m_htCapability->GetSerializedSize();
    }
    if (m_vhtCapability.has_value())
    {
        size += m_vhtCapability->GetSerializedSize();
    }
    if (m_heCapability.has_value())
    {
        size += m_heCapability->GetSerializedSize();
    }
    if (m_multiLinkElement.has_value())
    {
        size += m_multiLinkElement->GetSerializedSize();
    }
    if (m_ehtCapability.has_value())
    {
        size += m_ehtCapability->GetSerializedSize();
    }
    return size;
}

void
MgtReassocRequestHeader::Print(std::ostream& os) const
{
    os << "current AP address=" << m_currentApAddr << ", "
       << "ssid=" << m_ssid << ", "
       << "rates=" << m_rates << ", ";
    if (m_extendedCapability.has_value())
    {
        os << "Extended Capabilities=" << *m_extendedCapability << " , ";
    }
    if (m_htCapability.has_value())
    {
        os << "HT Capabilities=" << *m_htCapability << " , ";
    }
    if (m_vhtCapability.has_value())
    {
        os << "VHT Capabilities=" << *m_vhtCapability << " , ";
    }
    if (m_heCapability.has_value())
    {
        os << "HE Capabilities=" << *m_heCapability << " , ";
    }
    if (m_ehtCapability.has_value())
    {
        os << "EHT Capabilities=" << *m_ehtCapability;
    }
}

void
MgtReassocRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    WriteTo(i, m_currentApAddr);
    i = m_ssid.Serialize(i);
    i = m_rates.Serialize(i);
    if (m_rates.GetNRates() > 8)
    {
        i = m_rates.extended->Serialize(i);
    }
    if (m_extendedCapability.has_value())
    {
        i = m_extendedCapability->Serialize(i);
    }
    if (m_htCapability.has_value())
    {
        i = m_htCapability->Serialize(i);
    }
    if (m_vhtCapability.has_value())
    {
        i = m_vhtCapability->Serialize(i);
    }
    if (m_heCapability.has_value())
    {
        i = m_heCapability->Serialize(i);
    }
    if (m_multiLinkElement.has_value())
    {
        i = m_multiLinkElement->Serialize(i);
    }
    if (m_ehtCapability.has_value())
    {
        i = m_ehtCapability->Serialize(i);
    }
}

uint32_t
MgtReassocRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator tmp;
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    ReadFrom(i, m_currentApAddr);
    i = m_ssid.Deserialize(i);
    i = m_rates.Deserialize(i);
    i = WifiInformationElement::DeserializeIfPresent(m_rates.extended, i, &m_rates);
    i = WifiInformationElement::DeserializeIfPresent(m_extendedCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_multiLinkElement,
                                                     i,
                                                     WIFI_MAC_MGT_REASSOCIATION_REQUEST);
    const bool is2_4Ghz = m_rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    i = WifiInformationElement::DeserializeIfPresent(m_ehtCapability, i, is2_4Ghz, m_heCapability);
    return i.GetDistanceFrom(start);
}

/***********************************************************
 *          Assoc/Reassoc Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocResponseHeader);

MgtAssocResponseHeader::MgtAssocResponseHeader()
    : m_aid(0)
{
}

MgtAssocResponseHeader::~MgtAssocResponseHeader()
{
}

StatusCode
MgtAssocResponseHeader::GetStatusCode()
{
    return m_code;
}

const SupportedRates&
MgtAssocResponseHeader::GetSupportedRates() const
{
    return m_rates;
}

void
MgtAssocResponseHeader::SetStatusCode(StatusCode code)
{
    m_code = code;
}

void
MgtAssocResponseHeader::SetSupportedRates(const SupportedRates& rates)
{
    m_rates = rates;
}

void
MgtAssocResponseHeader::SetSupportedRates(SupportedRates&& rates)
{
    m_rates = std::move(rates);
}

void
MgtAssocResponseHeader::SetCapabilities(const CapabilityInformation& capabilities)
{
    m_capability = capabilities;
}

void
MgtAssocResponseHeader::SetCapabilities(CapabilityInformation&& capabilities)
{
    m_capability = std::move(capabilities);
}

const CapabilityInformation&
MgtAssocResponseHeader::GetCapabilities() const
{
    return m_capability;
}

void
MgtAssocResponseHeader::SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities)
{
    m_extendedCapability = extendedCapabilities;
}

void
MgtAssocResponseHeader::SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities)
{
    m_extendedCapability = std::move(extendedCapabilities);
}

const std::optional<ExtendedCapabilities>&
MgtAssocResponseHeader::GetExtendedCapabilities() const
{
    return m_extendedCapability;
}

void
MgtAssocResponseHeader::SetHtCapabilities(const HtCapabilities& htCapabilities)
{
    m_htCapability = htCapabilities;
}

void
MgtAssocResponseHeader::SetHtCapabilities(HtCapabilities&& htCapabilities)
{
    m_htCapability = std::move(htCapabilities);
}

const std::optional<HtCapabilities>&
MgtAssocResponseHeader::GetHtCapabilities() const
{
    return m_htCapability;
}

void
MgtAssocResponseHeader::SetHtOperation(const HtOperation& htOperation)
{
    m_htOperation = htOperation;
}

void
MgtAssocResponseHeader::SetHtOperation(HtOperation&& htOperation)
{
    m_htOperation = std::move(htOperation);
}

const std::optional<HtOperation>&
MgtAssocResponseHeader::GetHtOperation() const
{
    return m_htOperation;
}

void
MgtAssocResponseHeader::SetVhtCapabilities(const VhtCapabilities& vhtCapabilities)
{
    m_vhtCapability = vhtCapabilities;
}

void
MgtAssocResponseHeader::SetVhtCapabilities(VhtCapabilities&& vhtCapabilities)
{
    m_vhtCapability = std::move(vhtCapabilities);
}

const std::optional<VhtCapabilities>&
MgtAssocResponseHeader::GetVhtCapabilities() const
{
    return m_vhtCapability;
}

void
MgtAssocResponseHeader::SetVhtOperation(const VhtOperation& vhtOperation)
{
    m_vhtOperation = vhtOperation;
}

void
MgtAssocResponseHeader::SetVhtOperation(VhtOperation&& vhtOperation)
{
    m_vhtOperation = std::move(vhtOperation);
}

const std::optional<VhtOperation>&
MgtAssocResponseHeader::GetVhtOperation() const
{
    return m_vhtOperation;
}

void
MgtAssocResponseHeader::SetHeCapabilities(const HeCapabilities& heCapabilities)
{
    m_heCapability = heCapabilities;
}

void
MgtAssocResponseHeader::SetHeCapabilities(HeCapabilities&& heCapabilities)
{
    m_heCapability = std::move(heCapabilities);
}

const std::optional<HeCapabilities>&
MgtAssocResponseHeader::GetHeCapabilities() const
{
    return m_heCapability;
}

void
MgtAssocResponseHeader::SetHeOperation(const HeOperation& heOperation)
{
    m_heOperation = heOperation;
}

void
MgtAssocResponseHeader::SetHeOperation(HeOperation&& heOperation)
{
    m_heOperation = std::move(heOperation);
}

const std::optional<HeOperation>&
MgtAssocResponseHeader::GetHeOperation() const
{
    return m_heOperation;
}

void
MgtAssocResponseHeader::SetEhtCapabilities(const EhtCapabilities& ehtCapabilities)
{
    m_ehtCapability = ehtCapabilities;
}

void
MgtAssocResponseHeader::SetEhtCapabilities(EhtCapabilities&& ehtCapabilities)
{
    m_ehtCapability = std::move(ehtCapabilities);
}

const std::optional<EhtCapabilities>&
MgtAssocResponseHeader::GetEhtCapabilities() const
{
    return m_ehtCapability;
}

void
MgtAssocResponseHeader::SetEhtOperation(const EhtOperation& ehtOperation)
{
    m_ehtOperation = ehtOperation;
}

void
MgtAssocResponseHeader::SetEhtOperation(EhtOperation&& ehtOperation)
{
    m_ehtOperation = std::move(ehtOperation);
}

const std::optional<EhtOperation>&
MgtAssocResponseHeader::GetEhtOperation() const
{
    return m_ehtOperation;
}

void
MgtAssocResponseHeader::SetMultiLinkElement(const MultiLinkElement& multiLinkElement)
{
    m_multiLinkElement = multiLinkElement;
}

void
MgtAssocResponseHeader::SetMultiLinkElement(MultiLinkElement&& multiLinkElement)
{
    m_multiLinkElement = std::move(multiLinkElement);
}

const std::optional<MultiLinkElement>&
MgtAssocResponseHeader::GetMultiLinkElement() const
{
    return m_multiLinkElement;
}

void
MgtAssocResponseHeader::SetAssociationId(uint16_t aid)
{
    m_aid = aid;
}

uint16_t
MgtAssocResponseHeader::GetAssociationId() const
{
    return m_aid;
}

void
MgtAssocResponseHeader::SetErpInformation(const ErpInformation& erpInformation)
{
    m_erpInformation = erpInformation;
}

void
MgtAssocResponseHeader::SetErpInformation(ErpInformation&& erpInformation)
{
    m_erpInformation = std::move(erpInformation);
}

const std::optional<ErpInformation>&
MgtAssocResponseHeader::GetErpInformation() const
{
    return m_erpInformation;
}

void
MgtAssocResponseHeader::SetEdcaParameterSet(const EdcaParameterSet& edcaparameters)
{
    m_edcaParameterSet = edcaparameters;
}

void
MgtAssocResponseHeader::SetEdcaParameterSet(EdcaParameterSet&& edcaparameters)
{
    m_edcaParameterSet = std::move(edcaparameters);
}

void
MgtAssocResponseHeader::SetMuEdcaParameterSet(const MuEdcaParameterSet& muEdcaParameters)
{
    m_muEdcaParameterSet = muEdcaParameters;
}

void
MgtAssocResponseHeader::SetMuEdcaParameterSet(MuEdcaParameterSet&& muEdcaParameters)
{
    m_muEdcaParameterSet = std::move(muEdcaParameters);
}

const std::optional<EdcaParameterSet>&
MgtAssocResponseHeader::GetEdcaParameterSet() const
{
    return m_edcaParameterSet;
}

const std::optional<MuEdcaParameterSet>&
MgtAssocResponseHeader::GetMuEdcaParameterSet() const
{
    return m_muEdcaParameterSet;
}

TypeId
MgtAssocResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocResponseHeader>();
    return tid;
}

TypeId
MgtAssocResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MgtAssocResponseHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += m_code.GetSerializedSize();
    size += 2; // aid
    size += m_rates.GetSerializedSize();
    if (m_erpInformation.has_value())
    {
        size += m_erpInformation->GetSerializedSize();
    }
    if (m_rates.GetNRates() > 8)
    {
        size += m_rates.extended->GetSerializedSize();
    }
    if (m_edcaParameterSet.has_value())
    {
        size += m_edcaParameterSet->GetSerializedSize();
    }
    if (m_extendedCapability.has_value())
    {
        size += m_extendedCapability->GetSerializedSize();
    }
    if (m_htCapability.has_value())
    {
        size += m_htCapability->GetSerializedSize();
    }
    if (m_htOperation.has_value())
    {
        size += m_htOperation->GetSerializedSize();
    }
    if (m_vhtCapability.has_value())
    {
        size += m_vhtCapability->GetSerializedSize();
    }
    if (m_vhtOperation.has_value())
    {
        size += m_vhtOperation->GetSerializedSize();
    }
    if (m_heCapability.has_value())
    {
        size += m_heCapability->GetSerializedSize();
    }
    if (m_heOperation.has_value())
    {
        size += m_heOperation->GetSerializedSize();
    }
    if (m_muEdcaParameterSet.has_value())
    {
        size += m_muEdcaParameterSet->GetSerializedSize();
    }
    if (m_multiLinkElement.has_value())
    {
        size += m_multiLinkElement->GetSerializedSize();
    }
    if (m_ehtCapability.has_value())
    {
        size += m_ehtCapability->GetSerializedSize();
    }
    if (m_ehtOperation.has_value())
    {
        size += m_ehtOperation->GetSerializedSize();
    }
    return size;
}

void
MgtAssocResponseHeader::Print(std::ostream& os) const
{
    os << "status code=" << m_code << ", "
       << "aid=" << m_aid << ", "
       << "rates=" << m_rates << ", ";
    if (m_erpInformation.has_value())
    {
        os << "ERP information=" << *m_erpInformation << ", ";
    }
    if (m_extendedCapability.has_value())
    {
        os << "Extended Capabilities=" << *m_extendedCapability << " , ";
    }
    if (m_htCapability.has_value())
    {
        os << "HT Capabilities=" << *m_htCapability << " , ";
    }
    if (m_htOperation.has_value())
    {
        os << "HT Operation=" << *m_htOperation << " , ";
    }
    if (m_vhtCapability.has_value())
    {
        os << "VHT Capabilities=" << *m_vhtCapability << " , ";
    }
    if (m_vhtOperation.has_value())
    {
        os << "VHT Operation=" << *m_vhtOperation << " , ";
    }
    if (m_heCapability.has_value())
    {
        os << "HE Capabilities=" << *m_heCapability << " , ";
    }
    if (m_heOperation.has_value())
    {
        os << "HE Operation=" << *m_heOperation << " , ";
    }
    if (m_ehtCapability.has_value())
    {
        os << "EHT Capabilities=" << *m_ehtCapability;
    }
    if (m_ehtOperation.has_value())
    {
        os << "EHT Operation=" << *m_ehtOperation;
    }
}

void
MgtAssocResponseHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i = m_code.Serialize(i);
    i.WriteHtolsbU16(m_aid);
    i = m_rates.Serialize(i);
    if (m_erpInformation.has_value())
    {
        i = m_erpInformation->Serialize(i);
    }
    if (m_rates.GetNRates() > 8)
    {
        i = m_rates.extended->Serialize(i);
    }
    if (m_edcaParameterSet.has_value())
    {
        i = m_edcaParameterSet->Serialize(i);
    }
    if (m_extendedCapability.has_value())
    {
        i = m_extendedCapability->Serialize(i);
    }
    if (m_htCapability.has_value())
    {
        i = m_htCapability->Serialize(i);
    }
    if (m_htOperation.has_value())
    {
        i = m_htOperation->Serialize(i);
    }
    if (m_vhtCapability.has_value())
    {
        i = m_vhtCapability->Serialize(i);
    }
    if (m_vhtOperation.has_value())
    {
        i = m_vhtOperation->Serialize(i);
    }
    if (m_heCapability.has_value())
    {
        i = m_heCapability->Serialize(i);
    }
    if (m_heOperation.has_value())
    {
        i = m_heOperation->Serialize(i);
    }
    if (m_muEdcaParameterSet.has_value())
    {
        i = m_muEdcaParameterSet->Serialize(i);
    }
    if (m_multiLinkElement.has_value())
    {
        i = m_multiLinkElement->Serialize(i);
    }
    if (m_ehtCapability.has_value())
    {
        i = m_ehtCapability->Serialize(i);
    }
    if (m_ehtOperation.has_value())
    {
        i = m_ehtOperation->Serialize(i);
    }
}

uint32_t
MgtAssocResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator tmp;
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    i = m_code.Deserialize(i);
    m_aid = i.ReadLsbtohU16();
    i = m_rates.Deserialize(i);
    i = WifiInformationElement::DeserializeIfPresent(m_erpInformation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_rates.extended, i, &m_rates);
    i = WifiInformationElement::DeserializeIfPresent(m_edcaParameterSet, i);
    i = WifiInformationElement::DeserializeIfPresent(m_extendedCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_htOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_vhtOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heCapability, i);
    i = WifiInformationElement::DeserializeIfPresent(m_heOperation, i);
    i = WifiInformationElement::DeserializeIfPresent(m_muEdcaParameterSet, i);
    i = WifiInformationElement::DeserializeIfPresent(m_multiLinkElement,
                                                     i,
                                                     WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
    const bool is2_4Ghz = m_rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    i = WifiInformationElement::DeserializeIfPresent(m_ehtCapability, i, is2_4Ghz, m_heCapability);
    i = WifiInformationElement::DeserializeIfPresent(m_ehtOperation, i);
    return i.GetDistanceFrom(start);
}

/**********************************************************
 *   ActionFrame
 **********************************************************/
WifiActionHeader::WifiActionHeader()
{
}

WifiActionHeader::~WifiActionHeader()
{
}

void
WifiActionHeader::SetAction(WifiActionHeader::CategoryValue type,
                            WifiActionHeader::ActionValue action)
{
    m_category = static_cast<uint8_t>(type);
    switch (type)
    {
    case QOS: {
        m_actionValue = static_cast<uint8_t>(action.qos);
        break;
    }
    case BLOCK_ACK: {
        m_actionValue = static_cast<uint8_t>(action.blockAck);
        break;
    }
    case PUBLIC: {
        m_actionValue = static_cast<uint8_t>(action.publicAction);
        break;
    }
    case RADIO_MEASUREMENT: {
        m_actionValue = static_cast<uint8_t>(action.radioMeasurementAction);
        break;
    }
    case MESH: {
        m_actionValue = static_cast<uint8_t>(action.meshAction);
        break;
    }
    case MULTIHOP: {
        m_actionValue = static_cast<uint8_t>(action.multihopAction);
        break;
    }
    case SELF_PROTECTED: {
        m_actionValue = static_cast<uint8_t>(action.selfProtectedAction);
        break;
    }
    case DMG: {
        m_actionValue = static_cast<uint8_t>(action.dmgAction);
        break;
    }
    case FST: {
        m_actionValue = static_cast<uint8_t>(action.fstAction);
        break;
    }
    case UNPROTECTED_DMG: {
        m_actionValue = static_cast<uint8_t>(action.unprotectedDmgAction);
        break;
    }
    case VENDOR_SPECIFIC_ACTION: {
        break;
    }
    }
}

WifiActionHeader::CategoryValue
WifiActionHeader::GetCategory() const
{
    switch (m_category)
    {
    case QOS:
        return QOS;
    case BLOCK_ACK:
        return BLOCK_ACK;
    case PUBLIC:
        return PUBLIC;
    case RADIO_MEASUREMENT:
        return RADIO_MEASUREMENT;
    case MESH:
        return MESH;
    case MULTIHOP:
        return MULTIHOP;
    case SELF_PROTECTED:
        return SELF_PROTECTED;
    case DMG:
        return DMG;
    case FST:
        return FST;
    case UNPROTECTED_DMG:
        return UNPROTECTED_DMG;
    case VENDOR_SPECIFIC_ACTION:
        return VENDOR_SPECIFIC_ACTION;
    default:
        NS_FATAL_ERROR("Unknown action value");
        return SELF_PROTECTED;
    }
}

WifiActionHeader::ActionValue
WifiActionHeader::GetAction() const
{
    ActionValue retval;
    retval.selfProtectedAction =
        PEER_LINK_OPEN; // Needs to be initialized to something to quiet valgrind in default cases
    switch (m_category)
    {
    case QOS:
        switch (m_actionValue)
        {
        case ADDTS_REQUEST:
            retval.qos = ADDTS_REQUEST;
            break;
        case ADDTS_RESPONSE:
            retval.qos = ADDTS_RESPONSE;
            break;
        case DELTS:
            retval.qos = DELTS;
            break;
        case SCHEDULE:
            retval.qos = SCHEDULE;
            break;
        case QOS_MAP_CONFIGURE:
            retval.qos = QOS_MAP_CONFIGURE;
            break;
        default:
            NS_FATAL_ERROR("Unknown qos action code");
            retval.qos = ADDTS_REQUEST; /* quiet compiler */
        }
        break;

    case BLOCK_ACK:
        switch (m_actionValue)
        {
        case BLOCK_ACK_ADDBA_REQUEST:
            retval.blockAck = BLOCK_ACK_ADDBA_REQUEST;
            break;
        case BLOCK_ACK_ADDBA_RESPONSE:
            retval.blockAck = BLOCK_ACK_ADDBA_RESPONSE;
            break;
        case BLOCK_ACK_DELBA:
            retval.blockAck = BLOCK_ACK_DELBA;
            break;
        default:
            NS_FATAL_ERROR("Unknown block ack action code");
            retval.blockAck = BLOCK_ACK_ADDBA_REQUEST; /* quiet compiler */
        }
        break;

    case PUBLIC:
        switch (m_actionValue)
        {
        case QAB_REQUEST:
            retval.publicAction = QAB_REQUEST;
            break;
        case QAB_RESPONSE:
            retval.publicAction = QAB_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown public action code");
            retval.publicAction = QAB_REQUEST; /* quiet compiler */
        }
        break;

    case RADIO_MEASUREMENT:
        switch (m_actionValue)
        {
        case RADIO_MEASUREMENT_REQUEST:
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REQUEST;
            break;
        case RADIO_MEASUREMENT_REPORT:
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REPORT;
            break;
        case LINK_MEASUREMENT_REQUEST:
            retval.radioMeasurementAction = LINK_MEASUREMENT_REQUEST;
            break;
        case LINK_MEASUREMENT_REPORT:
            retval.radioMeasurementAction = LINK_MEASUREMENT_REPORT;
            break;
        case NEIGHBOR_REPORT_REQUEST:
            retval.radioMeasurementAction = NEIGHBOR_REPORT_REQUEST;
            break;
        case NEIGHBOR_REPORT_RESPONSE:
            retval.radioMeasurementAction = NEIGHBOR_REPORT_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown radio measurement action code");
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REQUEST; /* quiet compiler */
        }
        break;

    case SELF_PROTECTED:
        switch (m_actionValue)
        {
        case PEER_LINK_OPEN:
            retval.selfProtectedAction = PEER_LINK_OPEN;
            break;
        case PEER_LINK_CONFIRM:
            retval.selfProtectedAction = PEER_LINK_CONFIRM;
            break;
        case PEER_LINK_CLOSE:
            retval.selfProtectedAction = PEER_LINK_CLOSE;
            break;
        case GROUP_KEY_INFORM:
            retval.selfProtectedAction = GROUP_KEY_INFORM;
            break;
        case GROUP_KEY_ACK:
            retval.selfProtectedAction = GROUP_KEY_ACK;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.selfProtectedAction = PEER_LINK_OPEN; /* quiet compiler */
        }
        break;

    case MESH:
        switch (m_actionValue)
        {
        case LINK_METRIC_REPORT:
            retval.meshAction = LINK_METRIC_REPORT;
            break;
        case PATH_SELECTION:
            retval.meshAction = PATH_SELECTION;
            break;
        case PORTAL_ANNOUNCEMENT:
            retval.meshAction = PORTAL_ANNOUNCEMENT;
            break;
        case CONGESTION_CONTROL_NOTIFICATION:
            retval.meshAction = CONGESTION_CONTROL_NOTIFICATION;
            break;
        case MDA_SETUP_REQUEST:
            retval.meshAction = MDA_SETUP_REQUEST;
            break;
        case MDA_SETUP_REPLY:
            retval.meshAction = MDA_SETUP_REPLY;
            break;
        case MDAOP_ADVERTISMENT_REQUEST:
            retval.meshAction = MDAOP_ADVERTISMENT_REQUEST;
            break;
        case MDAOP_ADVERTISMENTS:
            retval.meshAction = MDAOP_ADVERTISMENTS;
            break;
        case MDAOP_SET_TEARDOWN:
            retval.meshAction = MDAOP_SET_TEARDOWN;
            break;
        case TBTT_ADJUSTMENT_REQUEST:
            retval.meshAction = TBTT_ADJUSTMENT_REQUEST;
            break;
        case TBTT_ADJUSTMENT_RESPONSE:
            retval.meshAction = TBTT_ADJUSTMENT_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.meshAction = LINK_METRIC_REPORT; /* quiet compiler */
        }
        break;

    case MULTIHOP: // not yet supported
        switch (m_actionValue)
        {
        case PROXY_UPDATE: // not used so far
            retval.multihopAction = PROXY_UPDATE;
            break;
        case PROXY_UPDATE_CONFIRMATION: // not used so far
            retval.multihopAction = PROXY_UPDATE;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.multihopAction = PROXY_UPDATE; /* quiet compiler */
        }
        break;

    case DMG:
        switch (m_actionValue)
        {
        case DMG_POWER_SAVE_CONFIGURATION_REQUEST:
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_REQUEST;
            break;
        case DMG_POWER_SAVE_CONFIGURATION_RESPONSE:
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_RESPONSE;
            break;
        case DMG_INFORMATION_REQUEST:
            retval.dmgAction = DMG_INFORMATION_REQUEST;
            break;
        case DMG_INFORMATION_RESPONSE:
            retval.dmgAction = DMG_INFORMATION_RESPONSE;
            break;
        case DMG_HANDOVER_REQUEST:
            retval.dmgAction = DMG_HANDOVER_REQUEST;
            break;
        case DMG_HANDOVER_RESPONSE:
            retval.dmgAction = DMG_HANDOVER_RESPONSE;
            break;
        case DMG_DTP_REQUEST:
            retval.dmgAction = DMG_DTP_REQUEST;
            break;
        case DMG_DTP_RESPONSE:
            retval.dmgAction = DMG_DTP_RESPONSE;
            break;
        case DMG_RELAY_SEARCH_REQUEST:
            retval.dmgAction = DMG_RELAY_SEARCH_REQUEST;
            break;
        case DMG_RELAY_SEARCH_RESPONSE:
            retval.dmgAction = DMG_RELAY_SEARCH_RESPONSE;
            break;
        case DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST:
            retval.dmgAction = DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST;
            break;
        case DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT:
            retval.dmgAction = DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT;
            break;
        case DMG_RLS_REQUEST:
            retval.dmgAction = DMG_RLS_REQUEST;
            break;
        case DMG_RLS_RESPONSE:
            retval.dmgAction = DMG_RLS_RESPONSE;
            break;
        case DMG_RLS_ANNOUNCEMENT:
            retval.dmgAction = DMG_RLS_ANNOUNCEMENT;
            break;
        case DMG_RLS_TEARDOWN:
            retval.dmgAction = DMG_RLS_TEARDOWN;
            break;
        case DMG_RELAY_ACK_REQUEST:
            retval.dmgAction = DMG_RELAY_ACK_REQUEST;
            break;
        case DMG_RELAY_ACK_RESPONSE:
            retval.dmgAction = DMG_RELAY_ACK_RESPONSE;
            break;
        case DMG_TPA_REQUEST:
            retval.dmgAction = DMG_TPA_REQUEST;
            break;
        case DMG_TPA_RESPONSE:
            retval.dmgAction = DMG_TPA_RESPONSE;
            break;
        case DMG_ROC_REQUEST:
            retval.dmgAction = DMG_ROC_REQUEST;
            break;
        case DMG_ROC_RESPONSE:
            retval.dmgAction = DMG_ROC_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown DMG management action code");
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_REQUEST; /* quiet compiler */
        }
        break;

    case FST:
        switch (m_actionValue)
        {
        case FST_SETUP_REQUEST:
            retval.fstAction = FST_SETUP_REQUEST;
            break;
        case FST_SETUP_RESPONSE:
            retval.fstAction = FST_SETUP_RESPONSE;
            break;
        case FST_TEAR_DOWN:
            retval.fstAction = FST_TEAR_DOWN;
            break;
        case FST_ACK_REQUEST:
            retval.fstAction = FST_ACK_REQUEST;
            break;
        case FST_ACK_RESPONSE:
            retval.fstAction = FST_ACK_RESPONSE;
            break;
        case ON_CHANNEL_TUNNEL_REQUEST:
            retval.fstAction = ON_CHANNEL_TUNNEL_REQUEST;
            break;
        default:
            NS_FATAL_ERROR("Unknown FST management action code");
            retval.fstAction = FST_SETUP_REQUEST; /* quiet compiler */
        }
        break;

    case UNPROTECTED_DMG:
        switch (m_actionValue)
        {
        case UNPROTECTED_DMG_ANNOUNCE:
            retval.unprotectedDmgAction = UNPROTECTED_DMG_ANNOUNCE;
            break;
        case UNPROTECTED_DMG_BRP:
            retval.unprotectedDmgAction = UNPROTECTED_DMG_BRP;
            break;
        case UNPROTECTED_MIMO_BF_SETUP:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_SETUP;
            break;
        case UNPROTECTED_MIMO_BF_POLL:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_POLL;
            break;
        case UNPROTECTED_MIMO_BF_FEEDBACK:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_FEEDBACK;
            break;
        case UNPROTECTED_MIMO_BF_SELECTION:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_SELECTION;
            break;
        default:
            NS_FATAL_ERROR("Unknown Unprotected DMG action code");
            retval.unprotectedDmgAction = UNPROTECTED_DMG_ANNOUNCE; /* quiet compiler */
        }
        break;

    default:
        NS_FATAL_ERROR("Unsupported action");
        retval.selfProtectedAction = PEER_LINK_OPEN; /* quiet compiler */
    }
    return retval;
}

TypeId
WifiActionHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiActionHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiActionHeader>();
    return tid;
}

TypeId
WifiActionHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

std::string
WifiActionHeader::CategoryValueToString(CategoryValue value) const
{
    switch (value)
    {
    case QOS:
        return "QoS";
    case BLOCK_ACK:
        return "BlockAck";
    case PUBLIC:
        return "Public";
    case RADIO_MEASUREMENT:
        return "RadioMeasurement";
    case MESH:
        return "Mesh";
    case MULTIHOP:
        return "Multihop";
    case SELF_PROTECTED:
        return "SelfProtected";
    case DMG:
        return "Dmg";
    case FST:
        return "Fst";
    case UNPROTECTED_DMG:
        return "UnprotectedDmg";
    case VENDOR_SPECIFIC_ACTION:
        return "VendorSpecificAction";
    default:
        std::ostringstream convert;
        convert << value;
        return convert.str();
    }
}

std::string
WifiActionHeader::SelfProtectedActionValueToString(SelfProtectedActionValue value) const
{
    if (value == PEER_LINK_OPEN)
    {
        return "PeerLinkOpen";
    }
    else if (value == PEER_LINK_CONFIRM)
    {
        return "PeerLinkConfirm";
    }
    else if (value == PEER_LINK_CLOSE)
    {
        return "PeerLinkClose";
    }
    else if (value == GROUP_KEY_INFORM)
    {
        return "GroupKeyInform";
    }
    else if (value == GROUP_KEY_ACK)
    {
        return "GroupKeyAck";
    }
    else
    {
        std::ostringstream convert;
        convert << value;
        return convert.str();
    }
}

void
WifiActionHeader::Print(std::ostream& os) const
{
    os << "category=" << CategoryValueToString((CategoryValue)m_category)
       << ", value=" << SelfProtectedActionValueToString((SelfProtectedActionValue)m_actionValue);
}

uint32_t
WifiActionHeader::GetSerializedSize() const
{
    return 2;
}

void
WifiActionHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_category);
    start.WriteU8(m_actionValue);
}

uint32_t
WifiActionHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_category = i.ReadU8();
    m_actionValue = i.ReadU8();
    return i.GetDistanceFrom(start);
}

/***************************************************
 *                 ADDBARequest
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAddBaRequestHeader);

MgtAddBaRequestHeader::MgtAddBaRequestHeader()
    : m_dialogToken(1),
      m_amsduSupport(1),
      m_bufferSize(0)
{
}

TypeId
MgtAddBaRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAddBaRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAddBaRequestHeader>();
    return tid;
}

TypeId
MgtAddBaRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtAddBaRequestHeader::Print(std::ostream& os) const
{
}

uint32_t
MgtAddBaRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1; // Dialog token
    size += 2; // Block ack parameter set
    size += 2; // Block ack timeout value
    size += 2; // Starting sequence control
    return size;
}

void
MgtAddBaRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_timeoutValue);
    i.WriteHtolsbU16(GetStartingSequenceControl());
}

uint32_t
MgtAddBaRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
    SetStartingSequenceControl(i.ReadLsbtohU16());
    return i.GetDistanceFrom(start);
}

void
MgtAddBaRequestHeader::SetDelayedBlockAck()
{
    m_policy = 0;
}

void
MgtAddBaRequestHeader::SetImmediateBlockAck()
{
    m_policy = 1;
}

void
MgtAddBaRequestHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = tid;
}

void
MgtAddBaRequestHeader::SetTimeout(uint16_t timeout)
{
    m_timeoutValue = timeout;
}

void
MgtAddBaRequestHeader::SetBufferSize(uint16_t size)
{
    m_bufferSize = size;
}

void
MgtAddBaRequestHeader::SetStartingSequence(uint16_t seq)
{
    m_startingSeq = seq;
}

void
MgtAddBaRequestHeader::SetStartingSequenceControl(uint16_t seqControl)
{
    m_startingSeq = (seqControl >> 4) & 0x0fff;
}

void
MgtAddBaRequestHeader::SetAmsduSupport(bool supported)
{
    m_amsduSupport = supported;
}

uint8_t
MgtAddBaRequestHeader::GetTid() const
{
    return m_tid;
}

bool
MgtAddBaRequestHeader::IsImmediateBlockAck() const
{
    return m_policy == 1;
}

uint16_t
MgtAddBaRequestHeader::GetTimeout() const
{
    return m_timeoutValue;
}

uint16_t
MgtAddBaRequestHeader::GetBufferSize() const
{
    return m_bufferSize;
}

bool
MgtAddBaRequestHeader::IsAmsduSupported() const
{
    return m_amsduSupport == 1;
}

uint16_t
MgtAddBaRequestHeader::GetStartingSequence() const
{
    return m_startingSeq;
}

uint16_t
MgtAddBaRequestHeader::GetStartingSequenceControl() const
{
    return (m_startingSeq << 4) & 0xfff0;
}

uint16_t
MgtAddBaRequestHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_amsduSupport;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= m_bufferSize << 6;
    return res;
}

void
MgtAddBaRequestHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = (params)&0x01;
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                 ADDBAResponse
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAddBaResponseHeader);

MgtAddBaResponseHeader::MgtAddBaResponseHeader()
    : m_dialogToken(1),
      m_amsduSupport(1),
      m_bufferSize(0)
{
}

TypeId
MgtAddBaResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAddBaResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAddBaResponseHeader>();
    return tid;
}

TypeId
MgtAddBaResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtAddBaResponseHeader::Print(std::ostream& os) const
{
    os << "status code=" << m_code;
}

uint32_t
MgtAddBaResponseHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1;                          // Dialog token
    size += m_code.GetSerializedSize(); // Status code
    size += 2;                          // Block ack parameter set
    size += 2;                          // Block ack timeout value
    return size;
}

void
MgtAddBaResponseHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i = m_code.Serialize(i);
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_timeoutValue);
}

uint32_t
MgtAddBaResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    i = m_code.Deserialize(i);
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
    return i.GetDistanceFrom(start);
}

void
MgtAddBaResponseHeader::SetDelayedBlockAck()
{
    m_policy = 0;
}

void
MgtAddBaResponseHeader::SetImmediateBlockAck()
{
    m_policy = 1;
}

void
MgtAddBaResponseHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = tid;
}

void
MgtAddBaResponseHeader::SetTimeout(uint16_t timeout)
{
    m_timeoutValue = timeout;
}

void
MgtAddBaResponseHeader::SetBufferSize(uint16_t size)
{
    m_bufferSize = size;
}

void
MgtAddBaResponseHeader::SetStatusCode(StatusCode code)
{
    m_code = code;
}

void
MgtAddBaResponseHeader::SetAmsduSupport(bool supported)
{
    m_amsduSupport = supported;
}

StatusCode
MgtAddBaResponseHeader::GetStatusCode() const
{
    return m_code;
}

uint8_t
MgtAddBaResponseHeader::GetTid() const
{
    return m_tid;
}

bool
MgtAddBaResponseHeader::IsImmediateBlockAck() const
{
    return m_policy == 1;
}

uint16_t
MgtAddBaResponseHeader::GetTimeout() const
{
    return m_timeoutValue;
}

uint16_t
MgtAddBaResponseHeader::GetBufferSize() const
{
    return m_bufferSize;
}

bool
MgtAddBaResponseHeader::IsAmsduSupported() const
{
    return m_amsduSupport == 1;
}

uint16_t
MgtAddBaResponseHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_amsduSupport;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= m_bufferSize << 6;
    return res;
}

void
MgtAddBaResponseHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = (params)&0x01;
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                     DelBa
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtDelBaHeader);

MgtDelBaHeader::MgtDelBaHeader()
    : m_reasonCode(1)
{
}

TypeId
MgtDelBaHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtDelBaHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtDelBaHeader>();
    return tid;
}

TypeId
MgtDelBaHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtDelBaHeader::Print(std::ostream& os) const
{
}

uint32_t
MgtDelBaHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 2; // DelBa parameter set
    size += 2; // Reason code
    return size;
}

void
MgtDelBaHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_reasonCode);
}

uint32_t
MgtDelBaHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    SetParameterSet(i.ReadLsbtohU16());
    m_reasonCode = i.ReadLsbtohU16();
    return i.GetDistanceFrom(start);
}

bool
MgtDelBaHeader::IsByOriginator() const
{
    return m_initiator == 1;
}

uint8_t
MgtDelBaHeader::GetTid() const
{
    NS_ASSERT(m_tid < 16);
    uint8_t tid = static_cast<uint8_t>(m_tid);
    return tid;
}

void
MgtDelBaHeader::SetByOriginator()
{
    m_initiator = 1;
}

void
MgtDelBaHeader::SetByRecipient()
{
    m_initiator = 0;
}

void
MgtDelBaHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = static_cast<uint16_t>(tid);
}

uint16_t
MgtDelBaHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_initiator << 11;
    res |= m_tid << 12;
    return res;
}

void
MgtDelBaHeader::SetParameterSet(uint16_t params)
{
    m_initiator = (params >> 11) & 0x01;
    m_tid = (params >> 12) & 0x0f;
}

} // namespace ns3
