/*
 * Copyright (c) 2021 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef REDUCED_NEIGHBOR_REPORT_H
#define REDUCED_NEIGHBOR_REPORT_H

#include "wifi-information-element.h"

#include "ns3/mac48-address.h"

#include <vector>

namespace ns3
{

class WifiPhyOperatingChannel;

/**
 * \brief The Reduced Neighbor Report element
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the Reduced Neighbor Report element.
 */
class ReducedNeighborReport : public WifiInformationElement
{
  public:
    /**
     * MLD Parameters subfield
     */
    struct MldParameters
    {
        uint8_t mldId;                //!< MLD ID
        uint8_t linkId;               //!< Link ID
        uint8_t bssParamsChangeCount; //!< BSS Parameters Change Count
    };

    /**
     * TBTT Information field
     */
    struct TbttInformation
    {
        uint8_t neighborApTbttOffset{0};      //!< Neighbor AP TBTT Offset
        Mac48Address bssid;                   //!< BSSID (optional)
        uint32_t shortSsid{0};                //!< Short SSID (optional)
        uint8_t bssParameters{0};             //!< BSS parameters (optional)
        uint8_t psd20MHz{0};                  //!< 20 MHz PSD (optional)
        MldParameters mldParameters{0, 0, 0}; //!< MLD Parameters (optional)
    };

    /**
     * TBTT Information Header subfield
     */
    struct TbttInformationHeader
    {
        uint8_t type : 2;          //!< TBTT Information Field Type (2 bits)
        uint8_t filtered : 1;      //!< Filtered Neighbor AP (1 bit)
        uint8_t reserved : 1;      //!< Reserved (1 bit)
        uint8_t tbttInfoCount : 4; //!< TBTT Information Count (4 bits)
        uint8_t tbttInfoLength;    //!< TBTT Information Length (8 bits)
    };

    /**
     * Neighbor AP information field
     */
    struct NeighborApInformation
    {
        mutable TbttInformationHeader tbttInfoHdr{0, 0, 0, 0, 0}; //!< TBTT Information header
        uint8_t operatingClass{0};                                //!< Operating class
        uint8_t channelNumber{0};                                 //!< Primary channel number
        std::vector<TbttInformation> tbttInformationSet; //!< One or more TBTT Information fields

        bool hasBssid{false};     //!< whether BSSID is present in all TBTT Information fields
        bool hasShortSsid{false}; //!< whether Short SSID is present in all TBTT Information fields
        bool hasBssParams{
            false}; //!< whether BSS parameters is present in all TBTT Information fields
        bool has20MHzPsd{false}; //!< whether 20 MHz PSD is present in all TBTT Information fields
        bool hasMldParams{
            false}; //!< whether MLD Parameters is present in all TBTT Information fields
    };

    ReducedNeighborReport();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    /**
     * Get the number of Neighbor AP Information fields
     *
     * \return the number of Neighbor AP Information fields
     */
    std::size_t GetNNbrApInfoFields() const;
    /**
     * Add a Neighbor AP Information field
     */
    void AddNbrApInfoField();

    /**
     * Set the Operating Class and the Channel Number fields of the given
     * Neighbor AP Information field based on the given operating channel.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param channel the PHY operating channel
     */
    void SetOperatingChannel(std::size_t nbrApInfoId, const WifiPhyOperatingChannel& channel);
    /**
     * Get the operating channel coded into the Operating Class and the Channel Number
     * fields of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return the PHY operating channel
     */
    WifiPhyOperatingChannel GetOperatingChannel(std::size_t nbrApInfoId) const;

    /**
     * Get the number of TBTT Information fields included in the TBTT Information Set
     * field of the given Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return the number of TBTT Information fields
     */
    std::size_t GetNTbttInformationFields(std::size_t nbrApInfoId) const;
    /**
     * Add a TBTT Information fields to the TBTT Information Set field
     * of the given Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     */
    void AddTbttInformationField(std::size_t nbrApInfoId);

    /**
     * Set the BSSID field of the <i>i</i>-th TBTT Information field of the given
     * Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \param bssid the BSSID value
     */
    void SetBssid(std::size_t nbrApInfoId, std::size_t index, Mac48Address bssid);
    /**
     * Return true if the BSSID field is present in all the TBTT Information fields
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return true if the BSSID field is present
     */
    bool HasBssid(std::size_t nbrApInfoId) const;
    /**
     * Get the BSSID field (must be present) in the <i>i</i>-th TBTT Information field
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the BSSID field
     */
    Mac48Address GetBssid(std::size_t nbrApInfoId, std::size_t index) const;

    /**
     * Set the Short SSID field of the <i>i</i>-th TBTT Information field of the given
     * Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \param shortSsid the short SSID value
     */
    void SetShortSsid(std::size_t nbrApInfoId, std::size_t index, uint32_t shortSsid);
    /**
     * Return true if the Short SSID field is present in all the TBTT Information fields
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return true if the Short SSID field is present
     */
    bool HasShortSsid(std::size_t nbrApInfoId) const;
    /**
     * Get the Short SSID field (must be present) in the <i>i</i>-th TBTT Information field
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the Short SSID field
     */
    uint32_t GetShortSsid(std::size_t nbrApInfoId, std::size_t index) const;

    /**
     * Set the BSS Parameters field of the <i>i</i>-th TBTT Information field of the given
     * Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \param bssParameters the BSS Parameters value
     */
    void SetBssParameters(std::size_t nbrApInfoId, std::size_t index, uint8_t bssParameters);
    /**
     * Return true if the BSS Parameters field is present in all the TBTT Information fields
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return true if the BSS Parameters field is present
     */
    bool HasBssParameters(std::size_t nbrApInfoId) const;
    /**
     * Get the BSS Parameters field (must be present) in the <i>i</i>-th TBTT Information field
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the BSS Parameters field
     */
    uint8_t GetBssParameters(std::size_t nbrApInfoId, std::size_t index) const;

    /**
     * Set the 20 MHz PSD field of the <i>i</i>-th TBTT Information field of the given
     * Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \param psd20MHz the 20 MHz PSD value
     */
    void SetPsd20MHz(std::size_t nbrApInfoId, std::size_t index, uint8_t psd20MHz);
    /**
     * Return true if the 20 MHz PSD field is present in all the TBTT Information fields
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return true if the 20 MHz PSD field is present
     */
    bool HasPsd20MHz(std::size_t nbrApInfoId) const;
    /**
     * Get the 20 MHz PSD field (must be present) in the <i>i</i>-th TBTT Information field
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the 20 MHz PSD field
     */
    uint8_t GetPsd20MHz(std::size_t nbrApInfoId, std::size_t index) const;

    /**
     * Set the MLD Parameters subfield of the <i>i</i>-th TBTT Information field of the given
     * Neighbor AP Information field
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \param mldId the MLD ID value
     * \param linkId the Link ID value
     * \param changeSequence the Change Sequence value
     */
    void SetMldParameters(std::size_t nbrApInfoId,
                          std::size_t index,
                          uint8_t mldId,
                          uint8_t linkId,
                          uint8_t changeSequence);
    /**
     * Return true if the MLD Parameters subfield is present in all the TBTT Information fields
     * of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return true if the MLD Parameters subfield is present
     */
    bool HasMldParameters(std::size_t nbrApInfoId) const;
    /**
     * Get the MLD ID value in the MLD Parameters subfield (must be present) in the
     * <i>i</i>-th TBTT Information field of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the MLD ID value
     */
    uint8_t GetMldId(std::size_t nbrApInfoId, std::size_t index) const;
    /**
     * Get the Link ID value in the MLD Parameters subfield (must be present) in the
     * <i>i</i>-th TBTT Information field of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \param index the index of the given TBTT Information field
     * \return the Link ID value
     */
    uint8_t GetLinkId(std::size_t nbrApInfoId, std::size_t index) const;

  private:
    /**
     * Set the TBTT Information Count field of the given Neighbor AP Information field
     * based on the size of the tbttInformationSet field.
     *
     * This method is marked as const because it needs to be called within the
     * SerializeInformationField method. In fact, only when serializing this object
     * we can set the TBTT Information Count field based on the number of TBTT Information
     * fields included in the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     */
    void WriteTbttInformationCount(std::size_t nbrApInfoId) const;
    /**
     * Get the TBTT Information Count field of the given Neighbor AP Information field.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     * \return the TBTT Information Count
     */
    uint8_t ReadTbttInformationCount(std::size_t nbrApInfoId) const;

    /**
     * Set the TBTT Information Length field of the given Neighbor AP Information field
     * based on the xxxPresent flags of the NeighborApInformation struct
     *
     * This method is marked as const because it needs to be called within the
     * SerializeInformationField method. In fact, only when serializing this object
     * we can set the TBTT Information Length field based on the TBTT Information
     * field contents.
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     */
    void WriteTbttInformationLength(std::size_t nbrApInfoId) const;
    /**
     * Use the TBTT Information Length field of the given Neighbor AP Information field
     * to set the xxxPresent flags of the NeighborApInformation struct
     *
     * \param nbrApInfoId identifier of the given Neighbor AP Information field
     */
    void ReadTbttInformationLength(std::size_t nbrApInfoId);

    std::vector<NeighborApInformation>
        m_nbrApInfoFields; //!< one or more Neighbor AP Information fields
};

} // namespace ns3

#endif /* REDUCED_NEIGHBOR_REPORT_H */
