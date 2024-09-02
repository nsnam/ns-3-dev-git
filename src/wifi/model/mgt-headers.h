/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
#include "wifi-mgt-header.h"

#include "ns3/dsss-parameter-set.h"
#include "ns3/eht-capabilities.h"
#include "ns3/eht-operation.h"
#include "ns3/erp-information.h"
#include "ns3/he-6ghz-band-capabilities.h"
#include "ns3/he-capabilities.h"
#include "ns3/he-operation.h"
#include "ns3/ht-capabilities.h"
#include "ns3/ht-operation.h"
#include "ns3/mac48-address.h"
#include "ns3/mu-edca-parameter-set.h"
#include "ns3/multi-link-element.h"
#include "ns3/tid-to-link-mapping-element.h"
#include "ns3/vht-capabilities.h"
#include "ns3/vht-operation.h"

namespace ns3
{

/**
 * Indicate which Information Elements cannot be included in a Per-STA Profile subelement of
 * a Basic Multi-Link Element (see Sec. 35.3.3.4 of 802.11be D3.1):
 *
 * An AP affiliated with an AP MLD shall not include a Timestamp field, a Beacon Interval field,
 * an AID field, a BSS Max Idle Period element, a Neighbor Report element, a Reduced Neighbor
 * Report element, a Multiple BSSID element, TIM element, Multiple BSSID-Index element, Multiple
 * BSSID Configuration element, TID-to-Link Mapping element, Multi-Link Traffic Indication element
 * or another Multi- Link element in the STA Profile field of the Basic Multi-Link element.
 *
 * A non-AP STA affiliated with a non-AP MLD shall not include a Listen Interval field, a Current
 * AP Address field, an SSID element, BSS Max Idle Period element or another Multi-Link element in
 * the STA Profile field of the Basic Multi-Link element.
 */

/** @copydoc CanBeInPerStaProfile */
template <>
struct CanBeInPerStaProfile<ReducedNeighborReport> : std::false_type
{
};

/** @copydoc CanBeInPerStaProfile */
template <>
struct CanBeInPerStaProfile<TidToLinkMapping> : std::false_type
{
};

/** @copydoc CanBeInPerStaProfile */
template <>
struct CanBeInPerStaProfile<MultiLinkElement> : std::false_type
{
};

/** @copydoc CanBeInPerStaProfile */
template <>
struct CanBeInPerStaProfile<Ssid> : std::false_type
{
};

/// List of Information Elements included in Probe Request frames
using ProbeRequestElems = std::tuple<Ssid,
                                     SupportedRates,
                                     std::optional<ExtendedSupportedRatesIE>,
                                     std::optional<DsssParameterSet>,
                                     std::optional<HtCapabilities>,
                                     std::optional<ExtendedCapabilities>,
                                     std::optional<VhtCapabilities>,
                                     std::optional<HeCapabilities>,
                                     std::optional<He6GhzBandCapabilities>,
                                     std::optional<MultiLinkElement>,
                                     std::optional<EhtCapabilities>>;

/// List of Information Elements included in Probe Response frames
using ProbeResponseElems = std::tuple<Ssid,
                                      SupportedRates,
                                      std::optional<DsssParameterSet>,
                                      std::optional<ErpInformation>,
                                      std::optional<ExtendedSupportedRatesIE>,
                                      std::optional<EdcaParameterSet>,
                                      std::optional<HtCapabilities>,
                                      std::optional<HtOperation>,
                                      std::optional<ExtendedCapabilities>,
                                      std::optional<VhtCapabilities>,
                                      std::optional<VhtOperation>,
                                      std::optional<ReducedNeighborReport>,
                                      std::optional<HeCapabilities>,
                                      std::optional<HeOperation>,
                                      std::optional<MuEdcaParameterSet>,
                                      std::optional<He6GhzBandCapabilities>,
                                      std::optional<MultiLinkElement>,
                                      std::optional<EhtCapabilities>,
                                      std::optional<EhtOperation>,
                                      std::vector<TidToLinkMapping>>;

/// List of Information Elements included in Association Request frames
using AssocRequestElems = std::tuple<Ssid,
                                     SupportedRates,
                                     std::optional<ExtendedSupportedRatesIE>,
                                     std::optional<HtCapabilities>,
                                     std::optional<ExtendedCapabilities>,
                                     std::optional<VhtCapabilities>,
                                     std::optional<HeCapabilities>,
                                     std::optional<He6GhzBandCapabilities>,
                                     std::optional<MultiLinkElement>,
                                     std::optional<EhtCapabilities>,
                                     std::vector<TidToLinkMapping>>;

/// List of Information Elements included in Association Response frames
using AssocResponseElems = std::tuple<SupportedRates,
                                      std::optional<ExtendedSupportedRatesIE>,
                                      std::optional<EdcaParameterSet>,
                                      std::optional<HtCapabilities>,
                                      std::optional<HtOperation>,
                                      std::optional<ExtendedCapabilities>,
                                      std::optional<VhtCapabilities>,
                                      std::optional<VhtOperation>,
                                      std::optional<HeCapabilities>,
                                      std::optional<HeOperation>,
                                      std::optional<MuEdcaParameterSet>,
                                      std::optional<He6GhzBandCapabilities>,
                                      std::optional<MultiLinkElement>,
                                      std::optional<EhtCapabilities>,
                                      std::optional<EhtOperation>,
                                      std::vector<TidToLinkMapping>>;

/**
 * @ingroup wifi
 * Implement the header for management frames of type association request.
 */
class MgtAssocRequestHeader
    : public MgtHeaderInPerStaProfile<MgtAssocRequestHeader, AssocRequestElems>
{
    friend class WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>;
    friend class MgtHeaderInPerStaProfile<MgtAssocRequestHeader, AssocRequestElems>;

  public:
    ~MgtAssocRequestHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;

    /**
     * Set the listen interval.
     *
     * @param interval the listen interval
     */
    void SetListenInterval(uint16_t interval);
    /**
     * Return the listen interval.
     *
     * @return the listen interval
     */
    uint16_t GetListenInterval() const;
    /**
     * @return a reference to the Capability information
     */
    CapabilityInformation& Capabilities();
    /**
     * @return a const reference to the Capability information
     */
    const CapabilityInformation& Capabilities() const;

  protected:
    /** @copydoc Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** @copydoc Header::Serialize */
    void SerializeImpl(Buffer::Iterator start) const;
    /** @copydoc Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);

    /**
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfileImpl(const MgtAssocRequestHeader& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * @param start an iterator which points to where the header should be written
     * @param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                      const MgtAssocRequestHeader& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * @param start an iterator which points to where the header should be read from
     * @param length the expected number of bytes to read
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                              uint16_t length,
                                              const MgtAssocRequestHeader& frame);

  private:
    CapabilityInformation m_capability; //!< Capability information
    uint16_t m_listenInterval{0};       //!< listen interval
};

/**
 * @ingroup wifi
 * Implement the header for management frames of type reassociation request.
 */
class MgtReassocRequestHeader
    : public MgtHeaderInPerStaProfile<MgtReassocRequestHeader, AssocRequestElems>
{
    friend class WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>;
    friend class MgtHeaderInPerStaProfile<MgtReassocRequestHeader, AssocRequestElems>;

  public:
    ~MgtReassocRequestHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;

    /**
     * Set the listen interval.
     *
     * @param interval the listen interval
     */
    void SetListenInterval(uint16_t interval);
    /**
     * Return the listen interval.
     *
     * @return the listen interval
     */
    uint16_t GetListenInterval() const;
    /**
     * @return a reference to the Capability information
     */
    CapabilityInformation& Capabilities();
    /**
     * @return a const reference to the Capability information
     */
    const CapabilityInformation& Capabilities() const;
    /**
     * Set the address of the current access point.
     *
     * @param currentApAddr address of the current access point
     */
    void SetCurrentApAddress(Mac48Address currentApAddr);

  protected:
    /** @copydoc Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** @copydoc Header::Serialize */
    void SerializeImpl(Buffer::Iterator start) const;
    /** @copydoc Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);
    /** @copydoc Header::Print */
    void PrintImpl(std::ostream& os) const;

    /**
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfileImpl(const MgtReassocRequestHeader& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * @param start an iterator which points to where the header should be written
     * @param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                      const MgtReassocRequestHeader& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * @param start an iterator which points to where the header should be read from
     * @param length the expected number of bytes to read
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                              uint16_t length,
                                              const MgtReassocRequestHeader& frame);

  private:
    Mac48Address m_currentApAddr;       //!< Address of the current access point
    CapabilityInformation m_capability; //!< Capability information
    uint16_t m_listenInterval{0};       //!< listen interval
};

/**
 * @ingroup wifi
 * Implement the header for management frames of type association and reassociation response.
 */
class MgtAssocResponseHeader
    : public MgtHeaderInPerStaProfile<MgtAssocResponseHeader, AssocResponseElems>
{
    friend class WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>;
    friend class MgtHeaderInPerStaProfile<MgtAssocResponseHeader, AssocResponseElems>;

  public:
    ~MgtAssocResponseHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;

    /**
     * Return the status code.
     *
     * @return the status code
     */
    StatusCode GetStatusCode();
    /**
     * Set the status code.
     *
     * @param code the status code
     */
    void SetStatusCode(StatusCode code);
    /**
     * @return a reference to the Capability information
     */
    CapabilityInformation& Capabilities();
    /**
     * @return a const reference to the Capability information
     */
    const CapabilityInformation& Capabilities() const;
    /**
     * Return the association ID.
     *
     * @return the association ID
     */
    uint16_t GetAssociationId() const;
    /**
     * Set the association ID.
     *
     * @param aid the association ID
     */
    void SetAssociationId(uint16_t aid);

  protected:
    /** @copydoc Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** @copydoc Header::Serialize */
    void SerializeImpl(Buffer::Iterator start) const;
    /** @copydoc Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);
    /** @copydoc Header::Print */
    void PrintImpl(std::ostream& os) const;

    /**
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfileImpl(const MgtAssocResponseHeader& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * @param start an iterator which points to where the header should be written
     * @param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                      const MgtAssocResponseHeader& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * @param start an iterator which points to where the header should be read from
     * @param length the expected number of bytes to read
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                              uint16_t length,
                                              const MgtAssocResponseHeader& frame);

  private:
    CapabilityInformation m_capability; //!< Capability information
    StatusCode m_code;                  //!< Status code
    uint16_t m_aid{0};                  //!< AID
};

/**
 * @ingroup wifi
 * Implement the header for management frames of type probe request.
 */
class MgtProbeRequestHeader : public WifiMgtHeader<MgtProbeRequestHeader, ProbeRequestElems>
{
  public:
    ~MgtProbeRequestHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;
};

/**
 * @ingroup wifi
 * Implement the header for management frames of type probe response.
 */
class MgtProbeResponseHeader
    : public MgtHeaderInPerStaProfile<MgtProbeResponseHeader, ProbeResponseElems>
{
    friend class WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>;
    friend class MgtHeaderInPerStaProfile<MgtProbeResponseHeader, ProbeResponseElems>;

  public:
    ~MgtProbeResponseHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /** @copydoc Header::GetInstanceTypeId */
    TypeId GetInstanceTypeId() const override;

    /**
     * Return the beacon interval in microseconds unit.
     *
     * @return beacon interval in microseconds unit
     */
    uint64_t GetBeaconIntervalUs() const;
    /**
     * Set the beacon interval in microseconds unit.
     *
     * @param us beacon interval in microseconds unit
     */
    void SetBeaconIntervalUs(uint64_t us);
    /**
     * @return a reference to the Capability information
     */
    CapabilityInformation& Capabilities();
    /**
     * @return a const reference to the Capability information
     */
    const CapabilityInformation& Capabilities() const;
    /**
     * Return the time stamp.
     *
     * @return time stamp
     */
    uint64_t GetTimestamp() const;

  protected:
    /** @copydoc Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** @copydoc Header::Serialize*/
    void SerializeImpl(Buffer::Iterator start) const;
    /** @copydoc Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);

    /**
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfileImpl(const MgtProbeResponseHeader& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * @param start an iterator which points to where the header should be written
     * @param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                      const MgtProbeResponseHeader& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * @param start an iterator which points to where the header should be read from
     * @param length the expected number of bytes to read
     * @param frame the frame containing the Multi-Link Element
     * @return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                              uint16_t length,
                                              const MgtProbeResponseHeader& frame);

  private:
    uint64_t m_timestamp;               //!< Timestamp
    uint64_t m_beaconInterval;          //!< Beacon interval
    CapabilityInformation m_capability; //!< Capability information
};

/**
 * @ingroup wifi
 * Implement the header for management frames of type beacon.
 */
class MgtBeaconHeader : public MgtProbeResponseHeader
{
  public:
    ~MgtBeaconHeader() override = default;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
};

} // namespace ns3

#endif /* MGT_HEADERS_H */
