/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef MULTI_LINK_ELEMENT_H
#define MULTI_LINK_ELEMENT_H

#include "common-info-basic-mle.h"
#include "common-info-probe-req-mle.h"

#include "ns3/nstime.h"
#include "ns3/wifi-information-element.h"
#include "ns3/wifi-mac-header.h"

#include <memory>
#include <optional>
#include <variant>

namespace ns3
{

class MgtAssocRequestHeader;
class MgtReassocRequestHeader;
class MgtAssocResponseHeader;
class MgtProbeResponseHeader;

/// variant holding a reference to a (Re)Association Request
using AssocReqRefVariant = std::variant<std::reference_wrapper<MgtAssocRequestHeader>,
                                        std::reference_wrapper<MgtReassocRequestHeader>>;

/**
 * @brief The Multi-Link element
 * @ingroup wifi
 *
 * The 802.11be Multi-Link element (see Sec.9.4.2.312 of 802.11be D5.0)
 *
 * TODO:
 * - Add support for variants other than the Basic and Probe Request.
 */
class MultiLinkElement : public WifiInformationElement
{
  public:
    /**
     * @ingroup wifi
     * Multi-Link element variants
     *
     * Note that Multi-Link element variants can be added to this enum only when
     * the corresponding CommonInfo variant is implemented. This is because the
     * index of m_commonInfo, which is a std::variant, is casted to this enum and
     * the index of the "unset" variant must correspond to UNSET.
     */
    enum Variant : uint8_t
    {
        BASIC_VARIANT = 0,
        PROBE_REQUEST_VARIANT,
        // RECONFIGURATION_VARIANT,
        // TDLS_VARIANT,
        // PRIORITY_ACCESS_VARIANT,
        UNSET
    };

    /**
     * @ingroup wifi
     * SubElement IDs
     */
    enum SubElementId : uint8_t
    {
        PER_STA_PROFILE_SUBELEMENT_ID = 0
    };

    /// Typedef for structure holding a reference to the containing frame
    using ContainingFrame = std::variant<std::monostate,
                                         std::reference_wrapper<const MgtAssocRequestHeader>,
                                         std::reference_wrapper<const MgtReassocRequestHeader>,
                                         std::reference_wrapper<const MgtAssocResponseHeader>,
                                         std::reference_wrapper<const MgtProbeResponseHeader>>;

    /**
     * Construct a Multi-Link Element with no variant set.
     *
     * @param frame the management frame containing this Multi-Link Element
     */
    MultiLinkElement(ContainingFrame frame = {});
    /**
     * Constructor
     *
     * @param variant the Multi-Link element variant (cannot be UNSET)
     * @param frame the management frame containing this Multi-Link Element
     */
    MultiLinkElement(Variant variant, ContainingFrame frame = {});

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    /**
     * Get the Multi-Link element variant
     *
     * @return the Multi-Link element variant
     */
    Variant GetVariant() const;

    /// @return a reference to the Common Info field (the MLE variant must be Basic)
    CommonInfoBasicMle& GetCommonInfoBasic();

    /// @return a const reference to the Common Info field (the MLE variant must be Basic)
    const CommonInfoBasicMle& GetCommonInfoBasic() const;

    /**
     * Set the MLD MAC Address subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element.
     *
     * @param address the MLD MAC address
     */
    void SetMldMacAddress(Mac48Address address);

    /**
     * Return the MLD MAC Address subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element.
     *
     * @return the MLD MAC Address subfield in the Common Info field.
     */
    Mac48Address GetMldMacAddress() const;

    /**
     * Set the Link ID Info subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element.
     *
     * @param linkIdInfo the link ID information
     */
    void SetLinkIdInfo(uint8_t linkIdInfo);
    /**
     * Return true if the Link ID Info subfield in the Common Info field is present
     * and false otherwise. Make sure that this is a Basic Multi-Link Element.
     *
     * @return true if the Link ID Info subfield in the Common Info field is present
     *         and false otherwise
     */
    bool HasLinkIdInfo() const;
    /**
     * Return the Link ID Info subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element and the Link ID Info subfield is present.
     *
     * @return the Link ID Info subfield in the Common Info field
     */
    uint8_t GetLinkIdInfo() const;

    /**
     * Set the BSS Parameters Change Count subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element.
     *
     * @param count the BSS Parameters Change Count
     */
    void SetBssParamsChangeCount(uint8_t count);
    /**
     * Return true if the BSS Parameters Change Count subfield in the Common Info field is present
     * and false otherwise. Make sure that this is a Basic Multi-Link Element.
     *
     * @return true if the BSS Parameters Change Count subfield in the Common Info field is present
     *         and false otherwise
     */
    bool HasBssParamsChangeCount() const;
    /**
     * Return the BSS Parameters Change Count subfield in the Common Info field. Make sure that
     * this is a Basic Multi-Link Element and the BSS Parameters Change Count subfield is present.
     *
     * @return the BSS Parameters Change Count subfield in the Common Info field
     */
    uint8_t GetBssParamsChangeCount() const;

    /**
     * Set the EMLSR Support subfield of the EML Capabilities subfield in the Common Info field
     * to 1 if EMLSR mode is supported and set it to 0 otherwise. Make sure that this is a Basic
     * Multi-Link Element.
     *
     * @param supported whether EMLSR mode is supported
     */
    void SetEmlsrSupported(bool supported);
    /**
     * Set the EMLSR Padding Delay subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element.
     *
     * @param delay the EMLSR Padding delay (0us, 32us, 64us, 128us or 256us)
     */
    void SetEmlsrPaddingDelay(Time delay);
    /**
     * Set the EMLSR Transition Delay subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element.
     *
     * @param delay the EMLSR Transition delay (0us, 16us, 32us, 64us, 128us or 256us)
     */
    void SetEmlsrTransitionDelay(Time delay);
    /**
     * Set the Transition Timeout subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element.
     *
     * @param timeout the Transition Timeout (0us or 2^n us, with n=7..16)
     */
    void SetTransitionTimeout(Time timeout);
    /**
     * Return true if the EML Capabilities subfield in the Common Info field is present
     * and false otherwise. Make sure that this is a Basic Multi-Link Element.
     *
     * @return whether the EML Capabilities subfield in the Common Info field is present
     */
    bool HasEmlCapabilities() const;
    /**
     * Return true if the EMLSR Support subfield of the EML Capabilities subfield in the
     * Common Info field is set to 1 and false otherwise. Make sure that this is a Basic
     * Multi-Link Element and the EML Capabilities subfield is present.
     *
     * @return whether the EMLSR Support subfield is set to 1
     */
    bool IsEmlsrSupported() const;
    /**
     * Get the EMLSR Padding Delay subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element and the
     * EML Capabilities subfield is present.
     *
     * @return the EMLSR Padding Delay
     */
    Time GetEmlsrPaddingDelay() const;
    /**
     * Get the EMLSR Transition Delay subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element and the
     * EML Capabilities subfield is present.
     *
     * @return the EMLSR Transition Delay
     */
    Time GetEmlsrTransitionDelay() const;
    /**
     * Get the Transition Timeout subfield of the EML Capabilities subfield in the
     * Common Info field. Make sure that this is a Basic Multi-Link Element and the
     * EML Capabilities subfield is present.
     *
     * @return the Transition Timeout
     */
    Time GetTransitionTimeout() const;

    /**
     * Set the AP MLD ID subfield of Common Info field. Valid variants are Basic and Probe Request.
     *
     * @param id AP MLD ID
     */
    void SetApMldId(uint8_t id);

    /**
     * Get the AP MLD ID subfield of Common Info field (if present). Valid variants are Basic and
     * Probe Request.
     *
     * @return the AP MLD ID
     */
    std::optional<uint8_t> GetApMldId() const;

    mutable ContainingFrame m_containingFrame; //!< reference to the mgt frame containing this MLE

    /**
     * @ingroup wifi
     * Per-STA Profile Subelement of Multi-Link element.
     * See Sec. 9.4.2.312.2.3 of 802.11be D1.5
     *
     * The frame body of the management frame included in the Per-STA Profile field
     * is stored as a (unique) pointer to the Header base class, because we cannot
     * include mgt-headers.h here (otherwise, we would create a circular dependency).
     *
     * TODO:
     * - complete the implementation of STA Control and STA Info subfields
     */
    class PerStaProfileSubelement : public WifiInformationElement
    {
      public:
        /**
         * Constructor
         *
         * @param variant the Multi-Link element variant
         */
        PerStaProfileSubelement(Variant variant);

        /**
         * Copy constructor performing a deep copy of the object
         *
         * @param perStaProfile the object to copy
         */
        PerStaProfileSubelement(const PerStaProfileSubelement& perStaProfile);

        /**
         * Copy assignment operator performing a deep copy of the object
         *
         * @param perStaProfile the object to copy-assign
         * @return a reference to this object
         */
        PerStaProfileSubelement& operator=(const PerStaProfileSubelement& perStaProfile);

        /**
         * Use default move assignment operator
         *
         * @param perStaProfile the object to move-assign
         * @return a reference to this object
         */
        PerStaProfileSubelement& operator=(PerStaProfileSubelement&& perStaProfile) = default;

        WifiInformationElementId ElementId() const override;

        /**
         * Set the Link ID subfield in the STA Control field
         *
         * @param linkId the Link ID value
         */
        void SetLinkId(uint8_t linkId);
        /**
         * Get the Link ID subfield in the STA Control field
         *
         * @return the Link ID subfield in the STA Control field
         */
        uint8_t GetLinkId() const;

        /**
         * Set the Complete Profile flag in the STA Control field
         */
        void SetCompleteProfile();
        /**
         * @return whether the Complete Profile flag in the STA Control field is set
         */
        bool IsCompleteProfileSet() const;

        /**
         * Set the STA MAC Address subfield in the STA Info field
         *
         * @param address the MAC address to set
         */
        void SetStaMacAddress(Mac48Address address);
        /**
         * Return true if the STA MAC Address subfield in the STA Info field is present
         *
         * @return true if the STA MAC Address subfield in the STA Info field is present
         */
        bool HasStaMacAddress() const;
        /**
         * Get the STA MAC Address subfield in the STA Info field, if present
         *
         * @return the STA MAC Address subfield in the STA Info field, if present
         */
        Mac48Address GetStaMacAddress() const;

        /**
         * Set the BSS Parameters Change Count subfield in the STA Info field.
         *
         * @param count BSS Parameters Change Count
         */
        void SetBssParamsChgCnt(uint8_t count);

        /// @return whether the BSS Parameters Change Count subfield in STA Info field is present
        bool HasBssParamsChgCnt() const;

        /**
         * Get BSS Parameters Change Count subfield in the STA Info field.
         *
         * @return count value
         */
        uint8_t GetBssParamsChgCnt() const;

        /**
         * Include the given (Re)Association Request frame body in the STA Profile field
         * of this Per-STA Profile subelement
         *
         * @param assoc the given (Re)Association Request frame body
         */
        void SetAssocRequest(
            const std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>& assoc);
        /** @copydoc SetAssocRequest */
        void SetAssocRequest(std::variant<MgtAssocRequestHeader, MgtReassocRequestHeader>&& assoc);
        /**
         * Return true if an Association Request frame body is included in the
         * STA Profile field of this Per-STA Profile subelement
         *
         * @return true if an Association Request frame body is included
         */
        bool HasAssocRequest() const;
        /**
         * Return true if a Reassociation Request frame body is included in the
         * STA Profile field of this Per-STA Profile subelement
         *
         * @return true if a Reassociation Request frame body is included
         */
        bool HasReassocRequest() const;
        /**
         * Get the (Re)Association Request frame body included in the STA Profile
         * field of this Per-STA Profile subelement
         *
         * @return the (Re)Association Request frame body
         */
        AssocReqRefVariant GetAssocRequest() const;

        /**
         * Include the given (Re)Association Response frame body in the STA Profile field
         * of this Per-STA Profile subelement
         *
         * @param assoc the given (Re)Association Response frame body
         */
        void SetAssocResponse(const MgtAssocResponseHeader& assoc);
        /** @copydoc SetAssocResponse */
        void SetAssocResponse(MgtAssocResponseHeader&& assoc);
        /**
         * Return true if a (Re)Association Response frame body is included in the
         * STA Profile field of this Per-STA Profile subelement
         *
         * @return true if a (Re)Association Response frame body is included
         */
        bool HasAssocResponse() const;
        /**
         * Get the (Re)Association Response frame body included in the STA Profile
         * field of this Per-STA Profile subelement
         *
         * @return the (Re)Association Response frame body
         */
        MgtAssocResponseHeader& GetAssocResponse() const;

        /**
         * Include the given Probe Response frame body in the STA Profile field
         * of this Per-STA Profile subelement
         *
         * @param probeResp the given Probe Response frame body
         */
        void SetProbeResponse(const MgtProbeResponseHeader& probeResp);

        /// @copydoc SetProbeResponse
        void SetProbeResponse(MgtProbeResponseHeader&& probeResp);

        /**
         * Return true if a Probe Response frame body is included in the
         * STA Profile field of this Per-STA Profile subelement
         *
         * @return true if a Probe Response frame body is included
         */
        bool HasProbeResponse() const;

        /**
         * Get the Probe Response frame body included in the STA Profile
         * field of this Per-STA Profile subelement
         *
         * @return the Probe Response frame body
         */
        MgtProbeResponseHeader& GetProbeResponse() const;

        /**
         * Get the size in bytes of the serialized STA Info Length subfield of
         * the STA Info field
         *
         * @return the size in bytes of the serialized STA Info Length subfield
         */
        uint8_t GetStaInfoLength() const;

        mutable ContainingFrame
            m_containingFrame; //!< the mgt frame containing this Per-STA Profile

      private:
        uint16_t GetInformationFieldSize() const override;
        void SerializeInformationField(Buffer::Iterator start) const override;
        uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

        /**
         * Deserialize information of Per-STA Profile Subelement in Probe Request Multi-link
         * Element.
         *
         * @param start an iterator which points to where the information should be written
         * @param length the expected number of octets to read
         * @return the number of octets read
         */
        uint16_t DeserProbeReqMlePerSta(ns3::Buffer::Iterator start, uint16_t length);

        Variant m_variant;                        //!< Multi-Link element variant
        uint16_t m_staControl;                    //!< STA Control field
        Mac48Address m_staMacAddress;             //!< STA MAC address
        std::optional<uint8_t> m_bssParamsChgCnt; //!< BSS Params Change Count (Basic MLE)
        std::variant<std::monostate,
                     std::unique_ptr<MgtAssocRequestHeader>,
                     std::unique_ptr<MgtReassocRequestHeader>,
                     std::unique_ptr<MgtAssocResponseHeader>,
                     std::unique_ptr<MgtProbeResponseHeader>>
            m_staProfile; /**< STA Profile field, containing the frame body of a frame of the
                               same type as the frame containing the Multi-Link Element */
    };

    /**
     * Add a Per-STA Profile Subelement in the Link Info field
     */
    void AddPerStaProfileSubelement();
    /**
     * Return the number of Per-STA Profile Subelement in the Link Info field
     *
     * @return the number of Per-STA Profile Subelement in the Link Info field
     */
    std::size_t GetNPerStaProfileSubelements() const;
    /**
     * Get a reference to the <i>i</i>-th Per-STA Profile Subelement in the Link Info field
     *
     * @param i the index of the Per-STA Profile Subelement in the Link Info field
     * @return a reference to the <i>i</i>-th Per-STA Profile Subelement in the Link Info field
     */
    PerStaProfileSubelement& GetPerStaProfile(std::size_t i);
    /**
     * Get a reference to the <i>i</i>-th Per-STA Profile Subelement in the Link Info field
     *
     * @param i the index of the Per-STA Profile Subelement in the Link Info field
     * @return a reference to the <i>i</i>-th Per-STA Profile Subelement in the Link Info field
     */
    const PerStaProfileSubelement& GetPerStaProfile(std::size_t i) const;

  private:
    /**
     * Set the variant of this Multi-Link Element
     *
     * @param variant the variant of this Multi-Link Element
     */
    void SetVariant(Variant variant);

    /// Typedef for structure holding a Common Info field
    using CommonInfo = std::variant<CommonInfoBasicMle,
                                    CommonInfoProbeReqMle,
                                    // TODO Add other variants
                                    std::monostate /* UNSET variant*/>;

    CommonInfo m_commonInfo; //!< Common Info field

    /*
     * Link Info field
     */
    std::vector<PerStaProfileSubelement>
        m_perStaProfileSubelements; //!< Per-STA Profile Subelements
};

} // namespace ns3

#endif /* MULTI_LINK_ELEMENT_H */
