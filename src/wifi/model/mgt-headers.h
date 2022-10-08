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

#ifndef MGT_HEADERS_H
#define MGT_HEADERS_H

#include "capability-information.h"
#include "edca-parameter-set.h"
#include "extended-capabilities.h"
#include "reduced-neighbor-report.h"
#include "ssid.h"
#include "status-code.h"
#include "supported-rates.h"

#include "ns3/dsss-parameter-set.h"
#include "ns3/eht-capabilities.h"
#include "ns3/erp-information.h"
#include "ns3/he-capabilities.h"
#include "ns3/he-operation.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-operation.h"
#include "ns3/mac48-address.h"
#include "ns3/mu-edca-parameter-set.h"
#include "ns3/multi-link-element.h"
#include "ns3/vht-capabilities.h"
#include "ns3/vht-operation.h"

namespace ns3
{

/**
 * \ingroup wifi
 * Implement the header for management frames of type association request.
 */
class MgtAssocRequestHeader : public Header
{
  public:
    MgtAssocRequestHeader();
    ~MgtAssocRequestHeader() override;

    /**
     * Set the Service Set Identifier (SSID).
     *
     * \param ssid SSID
     */
    void SetSsid(const Ssid& ssid);

    /** \copydoc SetSsid */
    void SetSsid(Ssid&& ssid);

    /**
     * Set the supported rates.
     *
     * \param rates the supported rates
     */
    void SetSupportedRates(const SupportedRates& rates);

    /** \copydoc SetSupportedRates */
    void SetSupportedRates(SupportedRates&& rates);

    /**
     * Set the listen interval.
     *
     * \param interval the listen interval
     */
    void SetListenInterval(uint16_t interval);

    /**
     * Set the Capability information.
     *
     * \param capabilities Capability information
     */
    void SetCapabilities(const CapabilityInformation& capabilities);

    /** \copydoc SetCapabilities */
    void SetCapabilities(CapabilityInformation&& capabilities);

    /**
     * Set the Extended Capabilities.
     *
     * \param extendedCapabilities the Extended Capabilities
     */
    void SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities);

    /** \copydoc SetExtendedCapabilities */
    void SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities);

    /**
     * Set the HT capabilities.
     *
     * \param htCapabilities HT capabilities
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);

    /** \copydoc SetHtCapabilities */
    void SetHtCapabilities(HtCapabilities&& htCapabilities);

    /**
     * Set the VHT capabilities.
     *
     * \param vhtCapabilities VHT capabilities
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);

    /** \copydoc SetVhtCapabilities */
    void SetVhtCapabilities(VhtCapabilities&& vhtCapabilities);

    /**
     * Set the HE capabilities.
     *
     * \param heCapabilities HE capabilities
     */
    void SetHeCapabilities(const HeCapabilities& heCapabilities);

    /** \copydoc SetHeCapabilities */
    void SetHeCapabilities(HeCapabilities&& heCapabilities);

    /**
     * Set the EHT capabilities.
     *
     * \param ehtCapabilities EHT capabilities
     */
    void SetEhtCapabilities(const EhtCapabilities& ehtCapabilities);

    /** \copydoc SetEhtCapabilities */
    void SetEhtCapabilities(EhtCapabilities&& ehtCapabilities);

    /**
     * Set the Multi-Link Element information element
     *
     * \param multiLinkElement the Multi-Link Element information element
     */
    void SetMultiLinkElement(const MultiLinkElement& multiLinkElement);

    /** \copydoc SetMultiLinkElement */
    void SetMultiLinkElement(MultiLinkElement&& multiLinkElement);

    /**
     * Return the Capability information.
     *
     * \return Capability information
     */
    const CapabilityInformation& GetCapabilities() const;
    /**
     * Return the extended capabilities, if present.
     *
     * \return the extended capabilities, if present
     */
    const std::optional<ExtendedCapabilities>& GetExtendedCapabilities() const;
    /**
     * Return the HT capabilities, if present.
     *
     * \return HT capabilities, if present
     */
    const std::optional<HtCapabilities>& GetHtCapabilities() const;
    /**
     * Return the VHT capabilities, if present.
     *
     * \return VHT capabilities, if present
     */
    const std::optional<VhtCapabilities>& GetVhtCapabilities() const;
    /**
     * Return the HE capabilities, if present.
     *
     * \return HE capabilities, if present
     */
    const std::optional<HeCapabilities>& GetHeCapabilities() const;
    /**
     * Return the EHT capabilities, if present.
     *
     * \return EHT capabilities, if present
     */
    const std::optional<EhtCapabilities>& GetEhtCapabilities() const;
    /**
     * Return the Service Set Identifier (SSID).
     *
     * \return SSID
     */
    const Ssid& GetSsid() const;
    /**
     * Return the supported rates.
     *
     * \return the supported rates
     */
    const SupportedRates& GetSupportedRates() const;
    /**
     * Return the listen interval.
     *
     * \return the listen interval
     */
    uint16_t GetListenInterval() const;
    /**
     * Return the Multi-Link Element information element, if present.
     *
     * \return the Multi-Link Element information element, if present
     */
    const std::optional<MultiLinkElement>& GetMultiLinkElement() const;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    Ssid m_ssid;                                              //!< Service Set ID (SSID)
    SupportedRates m_rates;                                   //!< List of supported rates
    CapabilityInformation m_capability;                       //!< Capability information
    std::optional<ExtendedCapabilities> m_extendedCapability; //!< Extended capabilities
    std::optional<HtCapabilities> m_htCapability;             //!< HT capabilities
    std::optional<VhtCapabilities> m_vhtCapability;           //!< VHT capabilities
    std::optional<HeCapabilities> m_heCapability;             //!< HE capabilities
    uint16_t m_listenInterval;                                //!< listen interval
    std::optional<EhtCapabilities> m_ehtCapability;           //!< EHT capabilities
    std::optional<MultiLinkElement> m_multiLinkElement;       //!< Multi-Link Element
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type reassociation request.
 */
class MgtReassocRequestHeader : public Header
{
  public:
    MgtReassocRequestHeader();
    ~MgtReassocRequestHeader() override;

    /**
     * Set the Service Set Identifier (SSID).
     *
     * \param ssid SSID
     */
    void SetSsid(const Ssid& ssid);

    /** \copydoc SetSsid */
    void SetSsid(Ssid&& ssid);

    /**
     * Set the supported rates.
     *
     * \param rates the supported rates
     */
    void SetSupportedRates(const SupportedRates& rates);

    /** \copydoc SetSupportedRates */
    void SetSupportedRates(SupportedRates&& rates);

    /**
     * Set the listen interval.
     *
     * \param interval the listen interval
     */
    void SetListenInterval(uint16_t interval);

    /**
     * Set the Capability information.
     *
     * \param capabilities Capability information
     */
    void SetCapabilities(const CapabilityInformation& capabilities);

    /** \copydoc SetCapabilities */
    void SetCapabilities(CapabilityInformation&& capabilities);

    /**
     * Set the Extended Capabilities.
     *
     * \param extendedCapabilities the Extended Capabilities
     */
    void SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities);

    /** \copydoc SetExtendedCapabilities */
    void SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities);

    /**
     * Set the HT capabilities.
     *
     * \param htCapabilities HT capabilities
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);

    /** \copydoc SetHtCapabilities */
    void SetHtCapabilities(HtCapabilities&& htCapabilities);

    /**
     * Set the VHT capabilities.
     *
     * \param vhtCapabilities VHT capabilities
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);

    /** \copydoc SetVhtCapabilities */
    void SetVhtCapabilities(VhtCapabilities&& vhtCapabilities);

    /**
     * Set the HE capabilities.
     *
     * \param heCapabilities HE capabilities
     */
    void SetHeCapabilities(const HeCapabilities& heCapabilities);

    /** \copydoc SetHeCapabilities */
    void SetHeCapabilities(HeCapabilities&& heCapabilities);

    /**
     * Set the EHT capabilities.
     *
     * \param ehtCapabilities EHT capabilities
     */
    void SetEhtCapabilities(const EhtCapabilities& ehtCapabilities);

    /** \copydoc SetEhtCapabilities */
    void SetEhtCapabilities(EhtCapabilities&& ehtCapabilities);

    /**
     * Set the Multi-Link Element information element
     *
     * \param multiLinkElement the Multi-Link Element information element
     */
    void SetMultiLinkElement(const MultiLinkElement& multiLinkElement);

    /** \copydoc SetMultiLinkElement */
    void SetMultiLinkElement(MultiLinkElement&& multiLinkElement);

    /**
     * Return the Capability information.
     *
     * \return Capability information
     */
    const CapabilityInformation& GetCapabilities() const;
    /**
     * Return the extended capabilities, if present.
     *
     * \return the extended capabilities, if present
     */
    const std::optional<ExtendedCapabilities>& GetExtendedCapabilities() const;
    /**
     * Return the HT capabilities, if present.
     *
     * \return HT capabilities, if present
     */
    const std::optional<HtCapabilities>& GetHtCapabilities() const;
    /**
     * Return the VHT capabilities, if present.
     *
     * \return VHT capabilities, if present
     */
    const std::optional<VhtCapabilities>& GetVhtCapabilities() const;
    /**
     * Return the HE capabilities, if present.
     *
     * \return HE capabilities, if present
     */
    const std::optional<HeCapabilities>& GetHeCapabilities() const;
    /**
     * Return the EHT capabilities, if present.
     *
     * \return EHT capabilities, if present
     */
    const std::optional<EhtCapabilities>& GetEhtCapabilities() const;
    /**
     * Return the Service Set Identifier (SSID).
     *
     * \return SSID
     */
    const Ssid& GetSsid() const;
    /**
     * Return the supported rates.
     *
     * \return the supported rates
     */
    const SupportedRates& GetSupportedRates() const;
    /**
     * Return the Multi-Link Element information element, if present.
     *
     * \return the Multi-Link Element information element, if present
     */
    const std::optional<MultiLinkElement>& GetMultiLinkElement() const;
    /**
     * Return the listen interval.
     *
     * \return the listen interval
     */
    uint16_t GetListenInterval() const;
    /**
     * Set the address of the current access point.
     *
     * \param currentApAddr address of the current access point
     */
    void SetCurrentApAddress(Mac48Address currentApAddr);

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    Mac48Address m_currentApAddr;       //!< Address of the current access point
    Ssid m_ssid;                        //!< Service Set ID (SSID)
    SupportedRates m_rates;             //!< List of supported rates
    CapabilityInformation m_capability; //!< Capability information
    std::optional<ExtendedCapabilities> m_extendedCapability; //!< Extended capabilities
    std::optional<HtCapabilities> m_htCapability;             //!< HT capabilities
    std::optional<VhtCapabilities> m_vhtCapability;           //!< VHT capabilities
    std::optional<HeCapabilities> m_heCapability;             //!< HE capabilities
    uint16_t m_listenInterval;                                //!< listen interval
    std::optional<EhtCapabilities> m_ehtCapability;           //!< EHT capabilities
    std::optional<MultiLinkElement> m_multiLinkElement;       //!< Multi-Link Element
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type association and reassociation response.
 */
class MgtAssocResponseHeader : public Header
{
  public:
    MgtAssocResponseHeader();
    ~MgtAssocResponseHeader() override;

    /**
     * Return the status code.
     *
     * \return the status code
     */
    StatusCode GetStatusCode();
    /**
     * Return the supported rates.
     *
     * \return the supported rates
     */
    const SupportedRates& GetSupportedRates() const;
    /**
     * Return the Capability information.
     *
     * \return Capability information
     */
    const CapabilityInformation& GetCapabilities() const;
    /**
     * Return the extended capabilities, if present.
     *
     * \return the extended capabilities, if present
     */
    const std::optional<ExtendedCapabilities>& GetExtendedCapabilities() const;
    /**
     * Return the HT capabilities, if present.
     *
     * \return HT capabilities, if present
     */
    const std::optional<HtCapabilities>& GetHtCapabilities() const;
    /**
     * Return the HT operation, if present.
     *
     * \return HT operation, if present
     */
    const std::optional<HtOperation>& GetHtOperation() const;
    /**
     * Return the VHT capabilities, if present.
     *
     * \return VHT capabilities, if present
     */
    const std::optional<VhtCapabilities>& GetVhtCapabilities() const;
    /**
     * Return the VHT operation, if present.
     *
     * \return VHT operation, if present
     */
    const std::optional<VhtOperation>& GetVhtOperation() const;
    /**
     * Return the HE capabilities, if present.
     *
     * \return HE capabilities, if present
     */
    const std::optional<HeCapabilities>& GetHeCapabilities() const;
    /**
     * Return the HE operation, if present.
     *
     * \return HE operation, if present
     */
    const std::optional<HeOperation>& GetHeOperation() const;
    /**
     * Return the EHT capabilities, if present.
     *
     * \return EHT capabilities, if present
     */
    const std::optional<EhtCapabilities>& GetEhtCapabilities() const;
    /**
     * Return the Multi-Link Element information element, if present.
     *
     * \return the Multi-Link Element information element, if present
     */
    const std::optional<MultiLinkElement>& GetMultiLinkElement() const;
    /**
     * Return the association ID.
     *
     * \return the association ID
     */
    uint16_t GetAssociationId() const;
    /**
     * Return the ERP information, if present.
     *
     * \return the ERP information, if present
     */
    const std::optional<ErpInformation>& GetErpInformation() const;
    /**
     * Return the EDCA Parameter Set, if present.
     *
     * \return the EDCA Parameter Set, if present
     */
    const std::optional<EdcaParameterSet>& GetEdcaParameterSet() const;
    /**
     * Return the MU EDCA Parameter Set, if present.
     *
     * \return the MU EDCA Parameter Set, if present
     */
    const std::optional<MuEdcaParameterSet>& GetMuEdcaParameterSet() const;
    /**
     * Set the Capability information.
     *
     * \param capabilities Capability information
     */
    void SetCapabilities(const CapabilityInformation& capabilities);

    /** \copydoc SetCapabilities */
    void SetCapabilities(CapabilityInformation&& capabilities);

    /**
     * Set the extended capabilities.
     *
     * \param extendedCapabilities the extended capabilities
     */
    void SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities);

    /** \copydoc SetExtendedCapabilities */
    void SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities);

    /**
     * Set the VHT operation.
     *
     * \param vhtOperation VHT operation
     */
    void SetVhtOperation(const VhtOperation& vhtOperation);

    /** \copydoc SetVhtOperation */
    void SetVhtOperation(VhtOperation&& vhtOperation);

    /**
     * Set the VHT capabilities.
     *
     * \param vhtCapabilities VHT capabilities
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);

    /** \copydoc SetVhtCapabilities */
    void SetVhtCapabilities(VhtCapabilities&& vhtCapabilities);

    /**
     * Set the HT capabilities.
     *
     * \param htCapabilities HT capabilities
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);

    /** \copydoc SetHtCapabilities */
    void SetHtCapabilities(HtCapabilities&& htCapabilities);

    /**
     * Set the HT operation.
     *
     * \param htOperation HT operation
     */
    void SetHtOperation(const HtOperation& htOperation);

    /** \copydoc SetHtOperation */
    void SetHtOperation(HtOperation&& htOperation);

    /**
     * Set the supported rates.
     *
     * \param rates the supported rates
     */
    void SetSupportedRates(const SupportedRates& rates);

    /** \copydoc SetSupportedRates */
    void SetSupportedRates(SupportedRates&& rates);

    /**
     * Set the status code.
     *
     * \param code the status code
     */
    void SetStatusCode(StatusCode code);

    /**
     * Set the association ID.
     *
     * \param aid the association ID
     */
    void SetAssociationId(uint16_t aid);

    /**
     * Set the ERP information.
     *
     * \param erpInformation the ERP information
     */
    void SetErpInformation(const ErpInformation& erpInformation);

    /** \copydoc SetErpInformation */
    void SetErpInformation(ErpInformation&& erpInformation);

    /**
     * Set the EDCA Parameter Set.
     *
     * \param edcaParameterSet the EDCA Parameter Set
     */
    void SetEdcaParameterSet(const EdcaParameterSet& edcaParameterSet);

    /** \copydoc SetEdcaParameterSet */
    void SetEdcaParameterSet(EdcaParameterSet&& edcaParameterSet);

    /**
     * Set the MU EDCA Parameter Set.
     *
     * \param muEdcaParameterSet the MU EDCA Parameter Set
     */
    void SetMuEdcaParameterSet(const MuEdcaParameterSet& muEdcaParameterSet);

    /** \copydoc SetMuEdcaParameterSet */
    void SetMuEdcaParameterSet(MuEdcaParameterSet&& muEdcaParameterSet);

    /**
     * Set the HE capabilities.
     *
     * \param heCapabilities HE capabilities
     */
    void SetHeCapabilities(const HeCapabilities& heCapabilities);

    /** \copydoc SetHeCapabilities */
    void SetHeCapabilities(HeCapabilities&& heCapabilities);

    /**
     * Set the HE operation.
     *
     * \param heOperation HE operation
     */
    void SetHeOperation(const HeOperation& heOperation);

    /** \copydoc SetHeOperation */
    void SetHeOperation(HeOperation&& heOperation);

    /**
     * Set the EHT capabilities.
     *
     * \param ehtCapabilities EHT capabilities
     */
    void SetEhtCapabilities(const EhtCapabilities& ehtCapabilities);

    /** \copydoc SetEhtCapabilities */
    void SetEhtCapabilities(EhtCapabilities&& ehtCapabilities);

    /**
     * Set the Multi-Link Element information element
     *
     * \param multiLinkElement the Multi-Link Element information element
     */
    void SetMultiLinkElement(const MultiLinkElement& multiLinkElement);

    /** \copydoc SetMultiLinkElement */
    void SetMultiLinkElement(MultiLinkElement&& multiLinkElement);

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    SupportedRates m_rates;                                   //!< List of supported rates
    CapabilityInformation m_capability;                       //!< Capability information
    StatusCode m_code;                                        //!< Status code
    uint16_t m_aid;                                           //!< AID
    std::optional<ExtendedCapabilities> m_extendedCapability; //!< extended capabilities
    std::optional<HtCapabilities> m_htCapability;             //!< HT capabilities
    std::optional<HtOperation> m_htOperation;                 //!< HT operation
    std::optional<VhtCapabilities> m_vhtCapability;           //!< VHT capabilities
    std::optional<VhtOperation> m_vhtOperation;               //!< VHT operation
    std::optional<ErpInformation> m_erpInformation;           //!< ERP information
    std::optional<EdcaParameterSet> m_edcaParameterSet;       //!< EDCA Parameter Set
    std::optional<HeCapabilities> m_heCapability;             //!< HE capabilities
    std::optional<HeOperation> m_heOperation;                 //!< HE operation
    std::optional<MuEdcaParameterSet> m_muEdcaParameterSet;   //!< MU EDCA Parameter Set
    std::optional<EhtCapabilities> m_ehtCapability;           //!< EHT capabilities
    std::optional<MultiLinkElement> m_multiLinkElement;       //!< Multi-Link Element
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type probe request.
 */
class MgtProbeRequestHeader : public Header
{
  public:
    ~MgtProbeRequestHeader() override;

    /**
     * Set the Service Set Identifier (SSID).
     *
     * \param ssid SSID
     */
    void SetSsid(const Ssid& ssid);

    /** \copydoc SetSsid */
    void SetSsid(Ssid&& ssid);

    /**
     * Set the supported rates.
     *
     * \param rates the supported rates
     */
    void SetSupportedRates(const SupportedRates& rates);

    /** \copydoc SetSupportedRates */
    void SetSupportedRates(SupportedRates&& rates);

    /**
     * Set the extended capabilities.
     *
     * \param extendedCapabilities the extended capabilities
     */
    void SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities);

    /** \copydoc SetExtendedCapabilities */
    void SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities);

    /**
     * Set the HT capabilities.
     *
     * \param htCapabilities HT capabilities
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);

    /** \copydoc SetHtCapabilities */
    void SetHtCapabilities(HtCapabilities&& htCapabilities);

    /**
     * Set the VHT capabilities.
     *
     * \param vhtCapabilities VHT capabilities
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);

    /** \copydoc SetVhtCapabilities */
    void SetVhtCapabilities(VhtCapabilities&& vhtCapabilities);

    /**
     * Set the HE capabilities.
     *
     * \param heCapabilities HE capabilities
     */
    void SetHeCapabilities(const HeCapabilities& heCapabilities);

    /** \copydoc SetHeCapabilities */
    void SetHeCapabilities(HeCapabilities&& heCapabilities);

    /**
     * Set the EHT capabilities.
     *
     * \param ehtCapabilities EHT capabilities
     */
    void SetEhtCapabilities(const EhtCapabilities& ehtCapabilities);

    /** \copydoc SetEhtCapabilities */
    void SetEhtCapabilities(EhtCapabilities&& ehtCapabilities);

    /**
     * Return the Service Set Identifier (SSID).
     *
     * \return SSID
     */
    const Ssid& GetSsid() const;

    /**
     * Return the supported rates.
     *
     * \return the supported rates
     */
    const SupportedRates& GetSupportedRates() const;

    /**
     * Return the extended capabilities, if present.
     *
     * \return the extended capabilities, if present
     */
    const std::optional<ExtendedCapabilities>& GetExtendedCapabilities() const;

    /**
     * Return the HT capabilities, if present.
     *
     * \return HT capabilities, if present
     */
    const std::optional<HtCapabilities>& GetHtCapabilities() const;

    /**
     * Return the VHT capabilities, if present.
     *
     * \return VHT capabilities, if present
     */
    const std::optional<VhtCapabilities>& GetVhtCapabilities() const;

    /**
     * Return the HE capabilities, if present.
     *
     * \return HE capabilities, if present
     */
    const std::optional<HeCapabilities>& GetHeCapabilities() const;

    /**
     * Return the EHT capabilities, if present.
     *
     * \return EHT capabilities, if present
     */
    const std::optional<EhtCapabilities>& GetEhtCapabilities() const;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    Ssid m_ssid;                                              //!< Service Set ID (SSID)
    SupportedRates m_rates;                                   //!< List of supported rates
    std::optional<ExtendedCapabilities> m_extendedCapability; //!< extended capabilities
    std::optional<HtCapabilities> m_htCapability;             //!< HT capabilities
    std::optional<VhtCapabilities> m_vhtCapability;           //!< VHT capabilities
    std::optional<HeCapabilities> m_heCapability;             //!< HE capabilities
    std::optional<EhtCapabilities> m_ehtCapability;           //!< EHT capabilities
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type probe response.
 */
class MgtProbeResponseHeader : public Header
{
  public:
    MgtProbeResponseHeader();
    ~MgtProbeResponseHeader() override;

    /**
     * Return the Service Set Identifier (SSID).
     *
     * \return SSID
     */
    const Ssid& GetSsid() const;
    /**
     * Return the beacon interval in microseconds unit.
     *
     * \return beacon interval in microseconds unit
     */
    uint64_t GetBeaconIntervalUs() const;
    /**
     * Return the supported rates.
     *
     * \return the supported rates
     */
    const SupportedRates& GetSupportedRates() const;
    /**
     * Return the Capability information.
     *
     * \return Capability information
     */
    const CapabilityInformation& GetCapabilities() const;
    /**
     * Return the DSSS Parameter Set, if present.
     *
     * \return the DSSS Parameter Set, if present
     */
    const std::optional<DsssParameterSet>& GetDsssParameterSet() const;
    /**
     * Return the extended capabilities, if present.
     *
     * \return the extended capabilities, if present
     */
    const std::optional<ExtendedCapabilities>& GetExtendedCapabilities() const;
    /**
     * Return the HT capabilities, if present.
     *
     * \return HT capabilities, if present
     */
    const std::optional<HtCapabilities>& GetHtCapabilities() const;
    /**
     * Return the HT operation, if present.
     *
     * \return HT operation, if present
     */
    const std::optional<HtOperation>& GetHtOperation() const;
    /**
     * Return the VHT capabilities, if present.
     *
     * \return VHT capabilities, if present
     */
    const std::optional<VhtCapabilities>& GetVhtCapabilities() const;
    /**
     * Return the VHT operation, if present.
     *
     * \return VHT operation, if present
     */
    const std::optional<VhtOperation>& GetVhtOperation() const;
    /**
     * Return the HE capabilities, if present.
     *
     * \return HE capabilities, if present
     */
    const std::optional<HeCapabilities>& GetHeCapabilities() const;
    /**
     * Return the HE operation, if present.
     *
     * \return HE operation, if present
     */
    const std::optional<HeOperation>& GetHeOperation() const;
    /**
     * Return the EHT capabilities, if present.
     *
     * \return EHT capabilities, if present
     */
    const std::optional<EhtCapabilities>& GetEhtCapabilities() const;
    /**
     * Return the ERP information, if present.
     *
     * \return the ERP information, if present
     */
    const std::optional<ErpInformation>& GetErpInformation() const;
    /**
     * Return the EDCA Parameter Set, if present.
     *
     * \return the EDCA Parameter Set, if present
     */
    const std::optional<EdcaParameterSet>& GetEdcaParameterSet() const;
    /**
     * Return the MU EDCA Parameter Set, if present.
     *
     * \return the MU EDCA Parameter Set, if present
     */
    const std::optional<MuEdcaParameterSet>& GetMuEdcaParameterSet() const;

    /**
     * Return the Reduced Neighbor Report information element, if present.
     *
     * \return the Reduced Neighbor Report information element, if present
     */
    const std::optional<ReducedNeighborReport>& GetReducedNeighborReport() const;
    /**
     * Return the Multi-Link Element information element, if present.
     *
     * \return the Multi-Link Element information element, if present
     */
    const std::optional<MultiLinkElement>& GetMultiLinkElement() const;
    /**
     * Set the Capability information.
     *
     * \param capabilities Capability information
     */
    void SetCapabilities(const CapabilityInformation& capabilities);

    /** \copydoc SetCapabilities */
    void SetCapabilities(CapabilityInformation&& capabilities);

    /**
     * Set the extended capabilities.
     *
     * \param extendedCapabilities the extended capabilities
     */
    void SetExtendedCapabilities(const ExtendedCapabilities& extendedCapabilities);

    /** \copydoc SetExtendedCapabilities */
    void SetExtendedCapabilities(ExtendedCapabilities&& extendedCapabilities);

    /**
     * Set the HT capabilities.
     *
     * \param htCapabilities HT capabilities
     */
    void SetHtCapabilities(const HtCapabilities& htCapabilities);

    /** \copydoc SetHtCapabilities */
    void SetHtCapabilities(HtCapabilities&& htCapabilities);

    /**
     * Set the HT operation.
     *
     * \param htOperation HT operation
     */
    void SetHtOperation(const HtOperation& htOperation);

    /** \copydoc SetHtOperation */
    void SetHtOperation(HtOperation&& htOperation);

    /**
     * Set the VHT capabilities.
     *
     * \param vhtCapabilities VHT capabilities
     */
    void SetVhtCapabilities(const VhtCapabilities& vhtCapabilities);

    /** \copydoc SetVhtCapabilities */
    void SetVhtCapabilities(VhtCapabilities&& vhtCapabilities);

    /**
     * Set the VHT operation.
     *
     * \param vhtOperation VHT operation
     */
    void SetVhtOperation(const VhtOperation& vhtOperation);

    /** \copydoc SetVhtOperation */
    void SetVhtOperation(VhtOperation&& vhtOperation);

    /**
     * Set the HE capabilities.
     *
     * \param heCapabilities HE capabilities
     */
    void SetHeCapabilities(const HeCapabilities& heCapabilities);

    /** \copydoc SetHeCapabilities */
    void SetHeCapabilities(HeCapabilities&& heCapabilities);

    /**
     * Set the HE operation.
     *
     * \param heOperation HE operation
     */
    void SetHeOperation(const HeOperation& heOperation);

    /** \copydoc SetHeOperation */
    void SetHeOperation(HeOperation&& heOperation);

    /**
     * Set the EHT capabilities.
     *
     * \param ehtCapabilities EHT capabilities
     */
    void SetEhtCapabilities(const EhtCapabilities& ehtCapabilities);

    /** \copydoc SetEhtCapabilities */
    void SetEhtCapabilities(EhtCapabilities&& ehtCapabilities);

    /**
     * Set the Service Set Identifier (SSID).
     *
     * \param ssid SSID
     */
    void SetSsid(const Ssid& ssid);

    /** \copydoc SetSsid */
    void SetSsid(Ssid&& ssid);

    /**
     * Set the beacon interval in microseconds unit.
     *
     * \param us beacon interval in microseconds unit
     */
    void SetBeaconIntervalUs(uint64_t us);

    /**
     * Set the supported rates.
     *
     * \param rates the supported rates
     */
    void SetSupportedRates(const SupportedRates& rates);

    /** \copydoc SetSupportedRates */
    void SetSupportedRates(SupportedRates&& rates);

    /**
     * Set the DSSS Parameter Set.
     *
     * \param dsssParameterSet the DSSS Parameter Set
     */
    void SetDsssParameterSet(const DsssParameterSet& dsssParameterSet);

    /** \copydoc SetDsssParameterSet */
    void SetDsssParameterSet(DsssParameterSet&& dsssParameterSet);

    /**
     * Set the ERP information.
     *
     * \param erpInformation the ERP information
     */
    void SetErpInformation(const ErpInformation& erpInformation);

    /** \copydoc SetErpInformation */
    void SetErpInformation(ErpInformation&& erpInformation);

    /**
     * Set the EDCA Parameter Set.
     *
     * \param edcaParameterSet the EDCA Parameter Set
     */
    void SetEdcaParameterSet(const EdcaParameterSet& edcaParameterSet);

    /** \copydoc SetEdcaParameterSet */
    void SetEdcaParameterSet(EdcaParameterSet&& edcaParameterSet);

    /**
     * Set the MU EDCA Parameter Set.
     *
     * \param muEdcaParameterSet the MU EDCA Parameter Set
     */
    void SetMuEdcaParameterSet(const MuEdcaParameterSet& muEdcaParameterSet);

    /** \copydoc SetMuEdcaParameterSet */
    void SetMuEdcaParameterSet(MuEdcaParameterSet&& muEdcaParameterSet);

    /**
     * Set the Reduced Neighbor Report information element
     *
     * \param reducedNeighborReport the Reduced Neighbor Report information element
     */
    void SetReducedNeighborReport(const ReducedNeighborReport& reducedNeighborReport);

    /** \copydoc SetReducedNeighborReport */
    void SetReducedNeighborReport(ReducedNeighborReport&& reducedNeighborReport);

    /**
     * Set the Multi-Link Element information element
     *
     * \param multiLinkElement the Multi-Link Element information element
     */
    void SetMultiLinkElement(const MultiLinkElement& multiLinkElement);

    /** \copydoc SetMultiLinkElement */
    void SetMultiLinkElement(MultiLinkElement&& multiLinkElement);

    /**
     * Return the time stamp.
     *
     * \return time stamp
     */
    uint64_t GetTimestamp();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint64_t m_timestamp;                                     //!< Timestamp
    Ssid m_ssid;                                              //!< Service set ID (SSID)
    uint64_t m_beaconInterval;                                //!< Beacon interval
    SupportedRates m_rates;                                   //!< List of supported rates
    CapabilityInformation m_capability;                       //!< Capability information
    std::optional<DsssParameterSet> m_dsssParameterSet;       //!< DSSS Parameter Set
    std::optional<ExtendedCapabilities> m_extendedCapability; //!< extended capabilities
    std::optional<HtCapabilities> m_htCapability;             //!< HT capabilities
    std::optional<HtOperation> m_htOperation;                 //!< HT operation
    std::optional<VhtCapabilities> m_vhtCapability;           //!< VHT capabilities
    std::optional<VhtOperation> m_vhtOperation;               //!< VHT operation
    std::optional<HeCapabilities> m_heCapability;             //!< HE capabilities
    std::optional<HeOperation> m_heOperation;                 //!< HE operation
    std::optional<ErpInformation> m_erpInformation;           //!< ERP information
    std::optional<EdcaParameterSet> m_edcaParameterSet;       //!< EDCA Parameter Set
    std::optional<MuEdcaParameterSet> m_muEdcaParameterSet;   //!< MU EDCA Parameter Set
    std::optional<EhtCapabilities> m_ehtCapability;           //!< EHT capabilities
    std::optional<ReducedNeighborReport>
        m_reducedNeighborReport;                        //!< Reduced Neighbor Report information
    std::optional<MultiLinkElement> m_multiLinkElement; //!< Multi-Link Element
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type beacon.
 */
class MgtBeaconHeader : public MgtProbeResponseHeader
{
  public:
    /** Register this type. */
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
};

/****************************
 *     Action frames
 *****************************/

/**
 * \ingroup wifi
 *
 * See IEEE 802.11 chapter 7.3.1.11
 * Header format: | category: 1 | action value: 1 |
 *
 */
class WifiActionHeader : public Header
{
  public:
    WifiActionHeader();
    ~WifiActionHeader() override;

    /*
     * Compatible with table 8-38 IEEE 802.11, Part11, (Year 2012)
     * Category values - see 802.11-2012 Table 8-38
     */

    /// CategoryValue enumeration
    enum CategoryValue // table 8-38 staring from IEEE 802.11, Part11, (Year 2012)
    {
        QOS = 1,
        BLOCK_ACK = 3,
        PUBLIC = 4,
        RADIO_MEASUREMENT = 5, // Category: Radio Measurement
        MESH = 13,             // Category: Mesh
        MULTIHOP = 14,         // not used so far
        SELF_PROTECTED = 15,   // Category: Self Protected
        DMG = 16,              // Category: DMG
        FST = 18,              // Category: Fast Session Transfer
        UNPROTECTED_DMG = 20,  // Category: Unprotected DMG
        // Since vendor specific action has no stationary Action value,the parse process is not
        // here. Refer to vendor-specific-action in wave module.
        VENDOR_SPECIFIC_ACTION = 127,
    };

    /// QosActionValue enumeration
    enum QosActionValue
    {
        ADDTS_REQUEST = 0,
        ADDTS_RESPONSE = 1,
        DELTS = 2,
        SCHEDULE = 3,
        QOS_MAP_CONFIGURE = 4,
    };

    /**
     * Block Ack Action field values
     * See 802.11 Table 8-202
     */
    enum BlockAckActionValue
    {
        BLOCK_ACK_ADDBA_REQUEST = 0,
        BLOCK_ACK_ADDBA_RESPONSE = 1,
        BLOCK_ACK_DELBA = 2
    };

    /// PublicActionValue enumeration
    enum PublicActionValue
    {
        QAB_REQUEST = 16,
        QAB_RESPONSE = 17,
    };

    /// RadioMeasurementActionValue enumeration
    enum RadioMeasurementActionValue
    {
        RADIO_MEASUREMENT_REQUEST = 0,
        RADIO_MEASUREMENT_REPORT = 1,
        LINK_MEASUREMENT_REQUEST = 2,
        LINK_MEASUREMENT_REPORT = 3,
        NEIGHBOR_REPORT_REQUEST = 4,
        NEIGHBOR_REPORT_RESPONSE = 5
    };

    /// MeshActionValue enumeration
    enum MeshActionValue
    {
        LINK_METRIC_REPORT = 0,              // Action Value:0 in Category 13: Mesh
        PATH_SELECTION = 1,                  // Action Value:1 in Category 13: Mesh
        PORTAL_ANNOUNCEMENT = 2,             // Action Value:2 in Category 13: Mesh
        CONGESTION_CONTROL_NOTIFICATION = 3, // Action Value:3 in Category 13: Mesh
        MDA_SETUP_REQUEST =
            4, // Action Value:4 in Category 13: Mesh MCCA-Setup-Request (not used so far)
        MDA_SETUP_REPLY =
            5, // Action Value:5 in Category 13: Mesh MCCA-Setup-Reply (not used so far)
        MDAOP_ADVERTISMENT_REQUEST =
            6, // Action Value:6 in Category 13: Mesh MCCA-Advertisement-Request (not used so far)
        MDAOP_ADVERTISMENTS = 7,       // Action Value:7 in Category 13: Mesh (not used so far)
        MDAOP_SET_TEARDOWN = 8,        // Action Value:8 in Category 13: Mesh (not used so far)
        TBTT_ADJUSTMENT_REQUEST = 9,   // Action Value:9 in Category 13: Mesh (not used so far)
        TBTT_ADJUSTMENT_RESPONSE = 10, // Action Value:10 in Category 13: Mesh (not used so far)
    };

    /// MultihopActionValue enumeration
    enum MultihopActionValue
    {
        PROXY_UPDATE = 0,              // not used so far
        PROXY_UPDATE_CONFIRMATION = 1, // not used so far
    };

    /// SelfProtectedActionValue enumeration
    enum SelfProtectedActionValue // Category: 15 (Self Protected)
    {
        PEER_LINK_OPEN = 1,    // Mesh Peering Open
        PEER_LINK_CONFIRM = 2, // Mesh Peering Confirm
        PEER_LINK_CLOSE = 3,   // Mesh Peering Close
        GROUP_KEY_INFORM = 4,  // Mesh Group Key Inform
        GROUP_KEY_ACK = 5,     // Mesh Group Key Acknowledge
    };

    /**
     * DMG Action field values
     * See 802.11ad Table 8-281b
     */
    enum DmgActionValue
    {
        DMG_POWER_SAVE_CONFIGURATION_REQUEST = 0,
        DMG_POWER_SAVE_CONFIGURATION_RESPONSE = 1,
        DMG_INFORMATION_REQUEST = 2,
        DMG_INFORMATION_RESPONSE = 3,
        DMG_HANDOVER_REQUEST = 4,
        DMG_HANDOVER_RESPONSE = 5,
        DMG_DTP_REQUEST = 6,
        DMG_DTP_RESPONSE = 7,
        DMG_RELAY_SEARCH_REQUEST = 8,
        DMG_RELAY_SEARCH_RESPONSE = 9,
        DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST = 10,
        DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT = 11,
        DMG_RLS_REQUEST = 12,
        DMG_RLS_RESPONSE = 13,
        DMG_RLS_ANNOUNCEMENT = 14,
        DMG_RLS_TEARDOWN = 15,
        DMG_RELAY_ACK_REQUEST = 16,
        DMG_RELAY_ACK_RESPONSE = 17,
        DMG_TPA_REQUEST = 18,
        DMG_TPA_RESPONSE = 19,
        DMG_TPA_REPORT = 20,
        DMG_ROC_REQUEST = 21,
        DMG_ROC_RESPONSE = 22
    };

    /**
     * FST Action field values
     * See 802.11ad Table 8-281x
     */
    enum FstActionValue
    {
        FST_SETUP_REQUEST = 0,
        FST_SETUP_RESPONSE = 1,
        FST_TEAR_DOWN = 2,
        FST_ACK_REQUEST = 3,
        FST_ACK_RESPONSE = 4,
        ON_CHANNEL_TUNNEL_REQUEST = 5
    };

    /**
     * Unprotected DMG action field values
     * See 802.11ad Table 8-281ae
     */
    enum UnprotectedDmgActionValue
    {
        UNPROTECTED_DMG_ANNOUNCE = 0,
        UNPROTECTED_DMG_BRP = 1,
        UNPROTECTED_MIMO_BF_SETUP = 2,
        UNPROTECTED_MIMO_BF_POLL = 3,
        UNPROTECTED_MIMO_BF_FEEDBACK = 4,
        UNPROTECTED_MIMO_BF_SELECTION = 5,
    };

    /**
     * typedef for union of different ActionValues
     */
    typedef union {
        QosActionValue qos;                                 ///< qos
        BlockAckActionValue blockAck;                       ///< block ack
        RadioMeasurementActionValue radioMeasurementAction; ///< radio measurement
        PublicActionValue publicAction;                     ///< public
        SelfProtectedActionValue selfProtectedAction;       ///< self protected
        MultihopActionValue multihopAction;                 ///< multi hop
        MeshActionValue meshAction;                         ///< mesh
        DmgActionValue dmgAction;                           ///< dmg
        FstActionValue fstAction;                           ///< fst
        UnprotectedDmgActionValue unprotectedDmgAction;     ///< unprotected dmg
    } ActionValue;                                          ///< the action value

    /**
     * Set action for this Action header.
     *
     * \param type category
     * \param action action
     */
    void SetAction(CategoryValue type, ActionValue action);

    /**
     * Return the category value.
     *
     * \return CategoryValue
     */
    CategoryValue GetCategory();
    /**
     * Return the action value.
     *
     * \return ActionValue
     */
    ActionValue GetAction();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * Category value to string function
     * \param value the category value
     * \returns the category value string
     */
    std::string CategoryValueToString(CategoryValue value) const;
    /**
     * Self protected action value to string function
     * \param value the protected action value
     * \returns the self protected action value string
     */
    std::string SelfProtectedActionValueToString(SelfProtectedActionValue value) const;
    uint8_t m_category;    //!< Category of the action
    uint8_t m_actionValue; //!< Action value
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Add Block Ack request.
 */
class MgtAddBaRequestHeader : public Header
{
  public:
    MgtAddBaRequestHeader();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Enable delayed BlockAck.
     */
    void SetDelayedBlockAck();
    /**
     * Enable immediate BlockAck
     */
    void SetImmediateBlockAck();
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set timeout.
     *
     * \param timeout timeout
     */
    void SetTimeout(uint16_t timeout);
    /**
     * Set buffer size.
     *
     * \param size buffer size
     */
    void SetBufferSize(uint16_t size);
    /**
     * Set the starting sequence number.
     *
     * \param seq the starting sequence number
     */
    void SetStartingSequence(uint16_t seq);
    /**
     * Enable or disable A-MSDU support.
     *
     * \param supported enable or disable A-MSDU support
     */
    void SetAmsduSupport(bool supported);

    /**
     * Return the starting sequence number.
     *
     * \return the starting sequence number
     */
    uint16_t GetStartingSequence() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Return whether the Block Ack policy is immediate Block Ack.
     *
     * \return true if immediate Block Ack is being used, false otherwise
     */
    bool IsImmediateBlockAck() const;
    /**
     * Return the timeout.
     *
     * \return timeout
     */
    uint16_t GetTimeout() const;
    /**
     * Return the buffer size.
     *
     * \return the buffer size.
     */
    uint16_t GetBufferSize() const;
    /**
     * Return whether A-MSDU capability is supported.
     *
     * \return true is A-MSDU is supported, false otherwise
     */
    bool IsAmsduSupported() const;

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);
    /**
     * Return the raw sequence control.
     *
     * \return the raw sequence control
     */
    uint16_t GetStartingSequenceControl() const;
    /**
     * Set sequence control with the given raw value.
     *
     * \param seqControl the raw sequence control
     */
    void SetStartingSequenceControl(uint16_t seqControl);

    uint8_t m_dialogToken;   //!< Not used for now
    uint8_t m_amsduSupport;  //!< Flag if A-MSDU is supported
    uint8_t m_policy;        //!< Block Ack policy
    uint8_t m_tid;           //!< Traffic ID
    uint16_t m_bufferSize;   //!< Buffer size
    uint16_t m_timeoutValue; //!< Timeout
    uint16_t m_startingSeq;  //!< Starting sequence number
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Add Block Ack response.
 */
class MgtAddBaResponseHeader : public Header
{
  public:
    MgtAddBaResponseHeader();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Enable delayed BlockAck.
     */
    void SetDelayedBlockAck();
    /**
     * Enable immediate BlockAck.
     */
    void SetImmediateBlockAck();
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set timeout.
     *
     * \param timeout timeout
     */
    void SetTimeout(uint16_t timeout);
    /**
     * Set buffer size.
     *
     * \param size buffer size
     */
    void SetBufferSize(uint16_t size);
    /**
     * Set the status code.
     *
     * \param code the status code
     */
    void SetStatusCode(StatusCode code);
    /**
     * Enable or disable A-MSDU support.
     *
     * \param supported enable or disable A-MSDU support
     */
    void SetAmsduSupport(bool supported);

    /**
     * Return the status code.
     *
     * \return the status code
     */
    StatusCode GetStatusCode() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Return whether the Block Ack policy is immediate Block Ack.
     *
     * \return true if immediate Block Ack is being used, false otherwise
     */
    bool IsImmediateBlockAck() const;
    /**
     * Return the timeout.
     *
     * \return timeout
     */
    uint16_t GetTimeout() const;
    /**
     * Return the buffer size.
     *
     * \return the buffer size.
     */
    uint16_t GetBufferSize() const;
    /**
     * Return whether A-MSDU capability is supported.
     *
     * \return true is A-MSDU is supported, false otherwise
     */
    bool IsAmsduSupported() const;

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);

    uint8_t m_dialogToken;   //!< Not used for now
    StatusCode m_code;       //!< Status code
    uint8_t m_amsduSupport;  //!< Flag if A-MSDU is supported
    uint8_t m_policy;        //!< Block ACK policy
    uint8_t m_tid;           //!< Traffic ID
    uint16_t m_bufferSize;   //!< Buffer size
    uint16_t m_timeoutValue; //!< Timeout
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Delete Block Ack.
 */
class MgtDelBaHeader : public Header
{
  public:
    MgtDelBaHeader();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Check if the initiator bit in the DELBA is set.
     *
     * \return true if the initiator bit in the DELBA is set,
     *         false otherwise
     */
    bool IsByOriginator() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set the initiator bit in the DELBA.
     */
    void SetByOriginator();
    /**
     * Un-set the initiator bit in the DELBA.
     */
    void SetByRecipient();

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);

    uint16_t m_initiator;  //!< initiator
    uint16_t m_tid;        //!< Traffic ID
    uint16_t m_reasonCode; //!< Not used for now. Always set to 1: "Unspecified reason"
};

} // namespace ns3

#endif /* MGT_HEADERS_H */
