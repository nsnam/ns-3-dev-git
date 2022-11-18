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

#include "reduced-neighbor-report.h"

#include "wifi-phy-operating-channel.h"

#include "ns3/abort.h"
#include "ns3/address-utils.h"

#include <iterator>

namespace ns3
{

ReducedNeighborReport::ReducedNeighborReport()
{
}

WifiInformationElementId
ReducedNeighborReport::ElementId() const
{
    return IE_REDUCED_NEIGHBOR_REPORT;
}

std::size_t
ReducedNeighborReport::GetNNbrApInfoFields() const
{
    return m_nbrApInfoFields.size();
}

void
ReducedNeighborReport::AddNbrApInfoField()
{
    m_nbrApInfoFields.emplace_back();
}

void
ReducedNeighborReport::SetOperatingChannel(std::size_t nbrApInfoId,
                                           const WifiPhyOperatingChannel& channel)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    uint8_t operatingClass = 0;
    uint8_t channelNumber = channel.GetNumber();

    // Information taken from Table E-4 of 802.11-2020
    switch (channel.GetPhyBand())
    {
    case WIFI_PHY_BAND_2_4GHZ:
        if (channel.GetWidth() == 20)
        {
            operatingClass = 81;
        }
        else if (channel.GetWidth() == 40)
        {
            operatingClass = 83;
        }
        break;
    case WIFI_PHY_BAND_5GHZ:
        if (channel.GetWidth() == 20)
        {
            if (channelNumber == 36 || channelNumber == 40 || channelNumber == 44 ||
                channelNumber == 48)
            {
                operatingClass = 115;
            }
            else if (channelNumber == 52 || channelNumber == 56 || channelNumber == 60 ||
                     channelNumber == 64)
            {
                operatingClass = 118;
            }
            else if (channelNumber == 100 || channelNumber == 104 || channelNumber == 108 ||
                     channelNumber == 112 || channelNumber == 116 || channelNumber == 120 ||
                     channelNumber == 124 || channelNumber == 128 || channelNumber == 132 ||
                     channelNumber == 136 || channelNumber == 140 || channelNumber == 144)
            {
                operatingClass = 121;
            }
            else if (channelNumber == 149 || channelNumber == 153 || channelNumber == 157 ||
                     channelNumber == 161 || channelNumber == 165 || channelNumber == 169 ||
                     channelNumber == 173 || channelNumber == 177 || channelNumber == 181)
            {
                operatingClass = 125;
            }
        }
        else if (channel.GetWidth() == 40)
        {
            if (channelNumber == 38 || channelNumber == 46)
            {
                operatingClass = 116;
            }
            else if (channelNumber == 54 || channelNumber == 62)
            {
                operatingClass = 119;
            }
            else if (channelNumber == 102 || channelNumber == 110 || channelNumber == 118 ||
                     channelNumber == 126 || channelNumber == 134 || channelNumber == 142)
            {
                operatingClass = 122;
            }
            else if (channelNumber == 151 || channelNumber == 159 || channelNumber == 167 ||
                     channelNumber == 175)
            {
                operatingClass = 126;
            }
        }
        else if (channel.GetWidth() == 80)
        {
            if (channelNumber == 42 || channelNumber == 58 || channelNumber == 106 ||
                channelNumber == 122 || channelNumber == 138 || channelNumber == 155 ||
                channelNumber == 171)
            {
                operatingClass = 128;
            }
        }
        else if (channel.GetWidth() == 160)
        {
            if (channelNumber == 50 || channelNumber == 114 || channelNumber == 163)
            {
                operatingClass = 129;
            }
        }
        break;
    case WIFI_PHY_BAND_6GHZ:
        if (channel.GetWidth() == 20)
        {
            operatingClass = 131;
        }
        else if (channel.GetWidth() == 40)
        {
            operatingClass = 132;
        }
        else if (channel.GetWidth() == 80)
        {
            operatingClass = 133;
        }
        else if (channel.GetWidth() == 160)
        {
            operatingClass = 134;
        }
        break;
    case WIFI_PHY_BAND_UNSPECIFIED:
    default:
        NS_ABORT_MSG("The provided channel has an unspecified PHY band");
        break;
    }

    NS_ABORT_MSG_IF(operatingClass == 0,
                    "Operating class not found for channel number "
                        << channelNumber << " width " << channel.GetWidth() << " MHz "
                        << "band " << channel.GetPhyBand());

    // find the primary channel number
    uint16_t startingFreq = 0;

    switch (channel.GetPhyBand())
    {
    case WIFI_PHY_BAND_2_4GHZ:
        startingFreq = 2407;
        break;
    case WIFI_PHY_BAND_5GHZ:
        startingFreq = 5000;
        break;
    case WIFI_PHY_BAND_6GHZ:
        startingFreq = 5950;
        break;
    case WIFI_PHY_BAND_UNSPECIFIED:
    default:
        NS_ABORT_MSG("The provided channel has an unspecified PHY band");
        break;
    }

    uint8_t primaryChannelNumber =
        (channel.GetPrimaryChannelCenterFrequency(20) - startingFreq) / 5;

    m_nbrApInfoFields.at(nbrApInfoId).operatingClass = operatingClass;
    m_nbrApInfoFields.at(nbrApInfoId).channelNumber = primaryChannelNumber;
}

WifiPhyOperatingChannel
ReducedNeighborReport::GetOperatingChannel(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    WifiPhyBand band = WIFI_PHY_BAND_UNSPECIFIED;
    uint16_t width = 0;

    switch (m_nbrApInfoFields.at(nbrApInfoId).operatingClass)
    {
    case 81:
        band = WIFI_PHY_BAND_2_4GHZ;
        width = 20;
        break;
    case 83:
        band = WIFI_PHY_BAND_2_4GHZ;
        width = 40;
        break;
    case 115:
    case 118:
    case 121:
    case 125:
        band = WIFI_PHY_BAND_5GHZ;
        width = 20;
        break;
    case 116:
    case 119:
    case 122:
    case 126:
        band = WIFI_PHY_BAND_5GHZ;
        width = 40;
        break;
    case 128:
        band = WIFI_PHY_BAND_5GHZ;
        width = 80;
        break;
    case 129:
        band = WIFI_PHY_BAND_5GHZ;
        width = 160;
        break;
    case 131:
        band = WIFI_PHY_BAND_6GHZ;
        width = 20;
        break;
    case 132:
        band = WIFI_PHY_BAND_6GHZ;
        width = 40;
        break;
    case 133:
        band = WIFI_PHY_BAND_6GHZ;
        width = 80;
        break;
    case 134:
        band = WIFI_PHY_BAND_6GHZ;
        width = 160;
        break;
    default:
        break;
    }

    NS_ABORT_IF(band == WIFI_PHY_BAND_UNSPECIFIED || width == 0);

    uint16_t startingFreq = 0;

    switch (band)
    {
    case WIFI_PHY_BAND_2_4GHZ:
        startingFreq = 2407;
        break;
    case WIFI_PHY_BAND_5GHZ:
        startingFreq = 5000;
        break;
    case WIFI_PHY_BAND_6GHZ:
        startingFreq = 5950;
        break;
    case WIFI_PHY_BAND_UNSPECIFIED:
    default:
        NS_ABORT_MSG("Unspecified band");
        break;
    }

    uint8_t primaryChannelNumber = m_nbrApInfoFields.at(nbrApInfoId).channelNumber;
    uint16_t primaryChannelCenterFrequency = startingFreq + primaryChannelNumber * 5;

    uint8_t channelNumber = 0;
    uint16_t frequency = 0;

    for (const auto& channel : WifiPhyOperatingChannel::m_frequencyChannels)
    {
        if (std::get<2>(channel) == width && std::get<3>(channel) == WIFI_PHY_OFDM_CHANNEL &&
            std::get<4>(channel) == band &&
            primaryChannelCenterFrequency > std::get<1>(channel) - width / 2 &&
            primaryChannelCenterFrequency < std::get<1>(channel) + width / 2)
        {
            // the center frequency of the primary channel falls into the frequency
            // range of this channel
            bool found = false;

            if (band != WIFI_PHY_BAND_2_4GHZ)
            {
                found = true;
            }
            else
            {
                // frequency channels overlap in the 2.4 GHz band, hence we have to check
                // that the given primary channel center frequency can be the center frequency
                // of the primary20 channel of the channel under consideration
                switch (width)
                {
                case 20:
                    if (std::get<1>(channel) == primaryChannelCenterFrequency)
                    {
                        found = true;
                    }
                    break;
                case 40:
                    if (std::get<1>(channel) == primaryChannelCenterFrequency + 10 ||
                        std::get<1>(channel) == primaryChannelCenterFrequency - 10)
                    {
                        found = true;
                    }
                    break;
                default:
                    NS_ABORT_MSG("No channel of width " << width << " MHz in the 2.4 GHz band");
                }
            }

            if (found)
            {
                channelNumber = std::get<0>(channel);
                frequency = std::get<1>(channel);
                break;
            }
        }
    }

    NS_ABORT_IF(channelNumber == 0 || frequency == 0);

    WifiPhyOperatingChannel channel;
    channel.Set(channelNumber, frequency, width, WIFI_STANDARD_UNSPECIFIED, band);

    uint16_t channelLowestFreq = frequency - width / 2;
    uint16_t primaryChannelLowestFreq = primaryChannelCenterFrequency - 10;
    channel.SetPrimary20Index((primaryChannelLowestFreq - channelLowestFreq) / 20);

    return channel;
}

std::size_t
ReducedNeighborReport::GetNTbttInformationFields(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size();
}

void
ReducedNeighborReport::AddTbttInformationField(std::size_t nbrApInfoId)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.emplace_back();
}

void
ReducedNeighborReport::WriteTbttInformationLength(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    uint8_t length = 0; // reserved value

    auto it = std::next(m_nbrApInfoFields.begin(), nbrApInfoId);

    if (it->hasBssid && !it->hasShortSsid && !it->hasBssParams && !it->has20MHzPsd &&
        !it->hasMldParams)
    {
        length = 7;
    }
    else if (it->hasBssid && it->hasShortSsid && it->hasBssParams && it->has20MHzPsd &&
             it->hasMldParams)
    {
        length = 16;
    }
    else
    {
        NS_ABORT_MSG("Unsupported TBTT Information field contents");
    }

    // set the TBTT Information Length field
    it->tbttInfoHdr.tbttInfoLength = length;
}

void
ReducedNeighborReport::ReadTbttInformationLength(std::size_t nbrApInfoId)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    auto it = std::next(m_nbrApInfoFields.begin(), nbrApInfoId);

    switch (it->tbttInfoHdr.tbttInfoLength)
    {
    case 7:
        it->hasBssid = true;
        it->hasShortSsid = false;
        it->hasBssParams = false;
        it->has20MHzPsd = false;
        it->hasMldParams = false;
        break;
    case 16:
        it->hasBssid = true;
        it->hasShortSsid = true;
        it->hasBssParams = true;
        it->has20MHzPsd = true;
        it->hasMldParams = true;
        break;
    default:
        NS_ABORT_MSG(
            "Unsupported TBTT Information Length value: " << it->tbttInfoHdr.tbttInfoLength);
    }
}

void
ReducedNeighborReport::SetBssid(std::size_t nbrApInfoId, std::size_t index, Mac48Address bssid)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).bssid = bssid;

    m_nbrApInfoFields.at(nbrApInfoId).hasBssid = true;
}

bool
ReducedNeighborReport::HasBssid(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return m_nbrApInfoFields.at(nbrApInfoId).hasBssid;
}

Mac48Address
ReducedNeighborReport::GetBssid(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasBssid(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).bssid;
}

void
ReducedNeighborReport::SetShortSsid(std::size_t nbrApInfoId, std::size_t index, uint32_t shortSsid)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).shortSsid = shortSsid;

    m_nbrApInfoFields.at(nbrApInfoId).hasShortSsid = true;
}

bool
ReducedNeighborReport::HasShortSsid(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return m_nbrApInfoFields.at(nbrApInfoId).hasShortSsid;
}

uint32_t
ReducedNeighborReport::GetShortSsid(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasShortSsid(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).shortSsid;
}

void
ReducedNeighborReport::SetBssParameters(std::size_t nbrApInfoId,
                                        std::size_t index,
                                        uint8_t bssParameters)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).bssParameters = bssParameters;

    m_nbrApInfoFields.at(nbrApInfoId).hasBssParams = true;
}

bool
ReducedNeighborReport::HasBssParameters(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return m_nbrApInfoFields.at(nbrApInfoId).hasBssParams;
}

uint8_t
ReducedNeighborReport::GetBssParameters(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasBssParameters(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).bssParameters;
}

void
ReducedNeighborReport::SetPsd20MHz(std::size_t nbrApInfoId, std::size_t index, uint8_t psd20MHz)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).psd20MHz = psd20MHz;

    m_nbrApInfoFields.at(nbrApInfoId).has20MHzPsd = true;
}

bool
ReducedNeighborReport::HasPsd20MHz(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return m_nbrApInfoFields.at(nbrApInfoId).has20MHzPsd;
}

uint8_t
ReducedNeighborReport::GetPsd20MHz(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasPsd20MHz(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).psd20MHz;
}

void
ReducedNeighborReport::SetMldParameters(std::size_t nbrApInfoId,
                                        std::size_t index,
                                        uint8_t mldId,
                                        uint8_t linkId,
                                        uint8_t changeCount)
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    auto it = std::next(m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.begin(), index);
    it->mldParameters.mldId = mldId;
    it->mldParameters.linkId = (linkId & 0x0f);
    it->mldParameters.bssParamsChangeCount = changeCount;

    m_nbrApInfoFields.at(nbrApInfoId).hasMldParams = true;
}

bool
ReducedNeighborReport::HasMldParameters(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return m_nbrApInfoFields.at(nbrApInfoId).hasMldParams;
}

uint8_t
ReducedNeighborReport::GetMldId(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasMldParameters(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).mldParameters.mldId;
}

uint8_t
ReducedNeighborReport::GetLinkId(std::size_t nbrApInfoId, std::size_t index) const
{
    NS_ASSERT(HasMldParameters(nbrApInfoId));
    NS_ASSERT(index < m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size());

    return m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.at(index).mldParameters.linkId &
           0x0f;
}

void
ReducedNeighborReport::WriteTbttInformationCount(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());
    NS_ASSERT(!m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.empty());

    // set the TBTT Information Count field
    m_nbrApInfoFields.at(nbrApInfoId).tbttInfoHdr.tbttInfoCount =
        m_nbrApInfoFields.at(nbrApInfoId).tbttInformationSet.size() - 1;
}

uint8_t
ReducedNeighborReport::ReadTbttInformationCount(std::size_t nbrApInfoId) const
{
    NS_ASSERT(nbrApInfoId < m_nbrApInfoFields.size());

    return 1 + m_nbrApInfoFields.at(nbrApInfoId).tbttInfoHdr.tbttInfoCount;
}

uint16_t
ReducedNeighborReport::GetInformationFieldSize() const
{
    uint16_t size = 0;

    for (const auto& neighborApInfo : m_nbrApInfoFields)
    {
        size += 4;

        size += 1 * neighborApInfo.tbttInformationSet.size();

        if (neighborApInfo.hasBssid)
        {
            size += 6 * neighborApInfo.tbttInformationSet.size();
        }
        if (neighborApInfo.hasShortSsid)
        {
            size += 4 * neighborApInfo.tbttInformationSet.size();
        }
        if (neighborApInfo.hasBssParams)
        {
            size += 1 * neighborApInfo.tbttInformationSet.size();
        }
        if (neighborApInfo.has20MHzPsd)
        {
            size += 1 * neighborApInfo.tbttInformationSet.size();
        }
        if (neighborApInfo.hasMldParams)
        {
            size += 3 * neighborApInfo.tbttInformationSet.size();
        }
    }

    return size;
}

void
ReducedNeighborReport::SerializeInformationField(Buffer::Iterator start) const
{
    for (std::size_t id = 0; id < m_nbrApInfoFields.size(); ++id)
    {
        WriteTbttInformationCount(id);
        WriteTbttInformationLength(id);
    }

    for (auto& neighborApInfo : m_nbrApInfoFields)
    {
        // serialize the TBTT Information Header
        uint16_t tbttInfoHdr = 0;
        tbttInfoHdr |= neighborApInfo.tbttInfoHdr.type;
        tbttInfoHdr |= (neighborApInfo.tbttInfoHdr.filtered << 2);
        tbttInfoHdr |= (neighborApInfo.tbttInfoHdr.tbttInfoCount << 4);
        tbttInfoHdr |= (neighborApInfo.tbttInfoHdr.tbttInfoLength << 8);
        start.WriteHtolsbU16(tbttInfoHdr);

        start.WriteU8(neighborApInfo.operatingClass);
        start.WriteU8(neighborApInfo.channelNumber);

        for (const auto& tbttInformation : neighborApInfo.tbttInformationSet)
        {
            start.WriteU8(tbttInformation.neighborApTbttOffset);

            if (neighborApInfo.hasBssid)
            {
                WriteTo(start, tbttInformation.bssid);
            }
            if (neighborApInfo.hasShortSsid)
            {
                start.WriteHtolsbU32(tbttInformation.shortSsid);
            }
            if (neighborApInfo.hasBssParams)
            {
                start.WriteU8(tbttInformation.bssParameters);
            }
            if (neighborApInfo.has20MHzPsd)
            {
                start.WriteU8(tbttInformation.psd20MHz);
            }
            if (neighborApInfo.hasMldParams)
            {
                start.WriteU8(tbttInformation.mldParameters.mldId);
                uint16_t other = 0;
                other |= (tbttInformation.mldParameters.linkId & 0x0f);
                other |= (tbttInformation.mldParameters.bssParamsChangeCount << 4);
                start.WriteHtolsbU16(other);
            }
        }
    }
}

uint16_t
ReducedNeighborReport::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    Buffer::Iterator i = start;
    uint16_t count = 0;

    while (count < length)
    {
        AddNbrApInfoField();

        auto tbttInfoHdr = i.ReadLsbtohU16();
        m_nbrApInfoFields.back().tbttInfoHdr.type = tbttInfoHdr & 0x0003;
        m_nbrApInfoFields.back().tbttInfoHdr.filtered = (tbttInfoHdr >> 2) & 0x0001;
        m_nbrApInfoFields.back().tbttInfoHdr.tbttInfoCount = (tbttInfoHdr >> 4) & 0x000f;
        m_nbrApInfoFields.back().tbttInfoHdr.tbttInfoLength = (tbttInfoHdr >> 8) & 0x00ff;

        m_nbrApInfoFields.back().operatingClass = i.ReadU8();
        m_nbrApInfoFields.back().channelNumber = i.ReadU8();
        count += 4;

        std::size_t neighborId = m_nbrApInfoFields.size() - 1;
        ReadTbttInformationLength(neighborId);

        for (uint8_t j = 0; j < ReadTbttInformationCount(neighborId); j++)
        {
            AddTbttInformationField(neighborId);

            m_nbrApInfoFields.back().tbttInformationSet.back().neighborApTbttOffset = i.ReadU8();
            count++;

            if (m_nbrApInfoFields.back().hasBssid)
            {
                ReadFrom(i, m_nbrApInfoFields.back().tbttInformationSet.back().bssid);
                count += 6;
            }
            if (m_nbrApInfoFields.back().hasShortSsid)
            {
                m_nbrApInfoFields.back().tbttInformationSet.back().shortSsid = i.ReadLsbtohU32();
                count += 4;
            }
            if (m_nbrApInfoFields.back().hasBssParams)
            {
                m_nbrApInfoFields.back().tbttInformationSet.back().bssParameters = i.ReadU8();
                count += 1;
            }
            if (m_nbrApInfoFields.back().has20MHzPsd)
            {
                m_nbrApInfoFields.back().tbttInformationSet.back().psd20MHz = i.ReadU8();
                count += 1;
            }
            if (m_nbrApInfoFields.back().hasMldParams)
            {
                m_nbrApInfoFields.back().tbttInformationSet.back().mldParameters.mldId = i.ReadU8();
                uint16_t other = i.ReadLsbtohU16();
                count += 3;
                m_nbrApInfoFields.back().tbttInformationSet.back().mldParameters.linkId =
                    other & 0x000f;
                m_nbrApInfoFields.back()
                    .tbttInformationSet.back()
                    .mldParameters.bssParamsChangeCount = (other >> 4) & 0x00ff;
            }
        }
    }

    return count;
}

} // namespace ns3
