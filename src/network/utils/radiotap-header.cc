/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "radiotap-header.h"

#include "ns3/log.h"

#include <bitset>
#include <cmath>
#include <iomanip>

namespace ns3
{

namespace
{
/// Number of bits per "present" field in the radiotap header
constexpr std::size_t RADIOTAP_BITS_PER_PRESENT_FIELD{32};
/// Size in bytes of the TSFT field in the radiotap header
constexpr uint16_t RADIOTAP_TSFT_SIZE_B = 8;
/// Alignment in bytes of the TSFT field in the radiotap header
constexpr uint16_t RADIOTAP_TSFT_ALIGNMENT_B = 8;
/// Size in bytes of the Flags field in the radiotap header
constexpr uint16_t RADIOTAP_FLAGS_SIZE_B = 1;
/// Size in bytes of the Rate field in the radiotap header
constexpr uint16_t RADIOTAP_RATE_SIZE_B = 1;
/// Size in bytes of the Channel field in the radiotap header
constexpr uint16_t RADIOTAP_CHANNEL_SIZE_B = 4;
/// Alignment in bytes of the Channel field in the radiotap header
constexpr uint16_t RADIOTAP_CHANNEL_ALIGNMENT_B = 2;
/// Size in bytes of the Antenna Signal field in the radiotap header
constexpr uint16_t RADIOTAP_ANTENNA_SIGNAL_SIZE_B = 1;
/// Size in bytes of the Antenna Noise field in the radiotap header
constexpr uint16_t RADIOTAP_ANTENNA_NOISE_SIZE_B = 1;
/// Size in bytes of the MCS field in the radiotap header
constexpr uint16_t RADIOTAP_MCS_SIZE_B = 3;
/// Alignment in bytes of the MCS field in the radiotap header
constexpr uint16_t RADIOTAP_MCS_ALIGNMENT_B = 1;
/// Size in bytes of the A-MPDU status field in the radiotap header
constexpr uint16_t RADIOTAP_AMPDU_STATUS_SIZE_B = 8;
/// Alignment in bytes of the A-MPDU status field in the radiotap header
constexpr uint16_t RADIOTAP_AMPDU_STATUS_ALIGNMENT_B = 4;
/// Size in bytes of the VHT field in the radiotap header
constexpr uint16_t RADIOTAP_VHT_SIZE_B = 12;
/// Alignment in bytes of the VHT field in the radiotap header
constexpr uint16_t RADIOTAP_VHT_ALIGNMENT_B = 2;
/// Size in bytes of the HE field in the radiotap header
constexpr uint16_t RADIOTAP_HE_SIZE_B = 12;
/// Alignment in bytes of the HE field in the radiotap header
constexpr uint16_t RADIOTAP_HE_ALIGNMENT_B = 2;
/// Size in bytes of the HE MU field in the radiotap header
constexpr uint16_t RADIOTAP_HE_MU_SIZE_B = 12;
/// Alignment in bytes of the HE MU field in the radiotap header
constexpr uint16_t RADIOTAP_HE_MU_ALIGNMENT_B = 2;
/// Size in bytes of the HE MU Other User field in the radiotap header
constexpr uint16_t RADIOTAP_HE_MU_OTHER_USER_SIZE_B = 6;
/// Alignment in bytes of the HE MU Other User field in the radiotap header
constexpr uint16_t RADIOTAP_HE_MU_OTHER_USER_ALIGNMENT_B = 2;
/// Size in bytes of the TLV fields (without data) in the radiotap header
constexpr uint16_t RADIOTAP_TLV_HEADER_SIZE_B = 4;
/// Alignment in bytes of the TLV fields in the radiotap header
constexpr uint16_t RADIOTAP_TLV_ALIGNMENT_B = 4;
/// Size in bytes of the U-SIG field in the radiotap header
constexpr uint16_t RADIOTAP_USIG_SIZE_B = 12;
/// Alignment in bytes of the U-SIG field in the radiotap header
constexpr uint16_t RADIOTAP_USIG_ALIGNMENT_B = 4;
/// Size in bytes of the known subfield of the EHT field in the radiotap header
constexpr uint16_t RADIOTAP_EHT_KNOWN_SIZE_B = 4;
/// Size in bytes of a data subfield of the EHT field in the radiotap header
constexpr uint16_t RADIOTAP_EHT_DATA_SIZE_B = 4;
/// Size in bytes of a user info subfield of the EHT field in the radiotap header
constexpr uint16_t RADIOTAP_EHT_USER_INFO_SIZE_B = 4;
/// Alignment in bytes of the EHT field in the radiotap header
constexpr uint16_t RADIOTAP_EHT_ALIGNMENT_B = 4;

/**
 * @brief Calculate the padding needed to align a field
 * @param offset Current offset in bytes
 * @param alignment Alignment requirement in bytes
 * @return Number of padding bytes needed
 */
uint8_t
GetPadding(uint32_t offset, uint32_t alignment)
{
    uint8_t padding = (alignment - (offset % alignment)) % alignment;
    return padding;
}

/**
 * @brief Check if a specific field is present in the radiotap header
 * @param presentBits Vector of bitsets representing the present fields
 * @param field Field to check
 * @return true if the field is present, false otherwise
 */
bool
IsPresent(const std::vector<std::bitset<RADIOTAP_BITS_PER_PRESENT_FIELD>>& presentBits,
          uint32_t field)
{
    const std::size_t bitmaskIdx = field / RADIOTAP_BITS_PER_PRESENT_FIELD;
    const std::size_t bitIdx = field % RADIOTAP_BITS_PER_PRESENT_FIELD;
    return (presentBits.size() > bitmaskIdx) && presentBits.at(bitmaskIdx).test(bitIdx);
}

} // namespace

NS_LOG_COMPONENT_DEFINE("RadiotapHeader");

NS_OBJECT_ENSURE_REGISTERED(RadiotapHeader);

TypeId
RadiotapHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RadiotapHeader")
                            .SetParent<Header>()
                            .SetGroupName("Network")

                            .AddConstructor<RadiotapHeader>();
    return tid;
}

TypeId
RadiotapHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RadiotapHeader::GetSerializedSize() const
{
    return m_length;
}

void
RadiotapHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);

    start.WriteU8(0);         // major version of radiotap header
    start.WriteU8(0);         // pad field
    start.WriteU16(m_length); // entire length of radiotap data + header
    NS_ASSERT(!m_present.empty());
    std::vector<std::bitset<RADIOTAP_BITS_PER_PRESENT_FIELD>> presentBits;
    for (const auto present : m_present)
    {
        start.WriteU32(present); // bits describing which fields follow header
        presentBits.emplace_back(present);
    }

    //
    // Time Synchronization Function Timer (when the first bit of the MPDU
    // arrived at the MAC)
    // Reference: https://www.radiotap.org/fields/TSFT.html
    //
    if (IsPresent(presentBits, RADIOTAP_TSFT))
    {
        SerializeTsft(start);
    }

    //
    // Properties of transmitted and received frames.
    // Reference: https://www.radiotap.org/fields/Flags.html
    //
    if (IsPresent(presentBits, RADIOTAP_FLAGS))
    {
        start.WriteU8(m_flags);
    }

    //
    // TX/RX data rate in units of 500 kbps
    // Reference: https://www.radiotap.org/fields/Rate.html
    //
    if (IsPresent(presentBits, RADIOTAP_RATE))
    {
        start.WriteU8(m_rate);
    }

    //
    // Tx/Rx frequency in MHz, followed by flags.
    // Reference: https://www.radiotap.org/fields/Channel.html
    //
    if (IsPresent(presentBits, RADIOTAP_CHANNEL))
    {
        SerializeChannel(start);
    }

    //
    // RF signal power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20signal.html
    //
    if (IsPresent(presentBits, RADIOTAP_DBM_ANTSIGNAL))
    {
        start.WriteU8(m_antennaSignal);
    }

    //
    // RF noise power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20noise.html
    //
    if (IsPresent(presentBits, RADIOTAP_DBM_ANTNOISE))
    {
        start.WriteU8(m_antennaNoise);
    }

    //
    // MCS field.
    // Reference: https://www.radiotap.org/fields/MCS.html
    //
    if (IsPresent(presentBits, RADIOTAP_MCS))
    {
        SerializeMcs(start);
    }

    //
    // A-MPDU Status, information about the received or transmitted A-MPDU.
    // Reference: https://www.radiotap.org/fields/A-MPDU%20status.html
    //
    if (IsPresent(presentBits, RADIOTAP_AMPDU_STATUS))
    {
        SerializeAmpduStatus(start);
    }

    //
    // Information about the received or transmitted VHT frame.
    // Reference: https://www.radiotap.org/fields/VHT.html
    //
    if (IsPresent(presentBits, RADIOTAP_VHT))
    {
        SerializeVht(start);
    }

    //
    // HE field.
    // Reference: https://www.radiotap.org/fields/HE.html
    //
    if (IsPresent(presentBits, RADIOTAP_HE))
    {
        SerializeHe(start);
    }

    //
    // HE MU field.
    // Reference: https://www.radiotap.org/fields/HE-MU.html
    //
    if (IsPresent(presentBits, RADIOTAP_HE_MU))
    {
        SerializeHeMu(start);
    }

    //
    // HE MU other user field.
    // Reference: https://www.radiotap.org/fields/HE-MU-other-user.html
    //
    if (IsPresent(presentBits, RADIOTAP_HE_MU_OTHER_USER))
    {
        SerializeHeMuOtherUser(start);
    }

    //
    // U-SIG field.
    // Reference: https://www.radiotap.org/fields/U-SIG.html
    //
    if (IsPresent(presentBits, RADIOTAP_USIG))
    {
        SerializeUsig(start);
    }

    //
    // EHT field.
    // Reference: https://www.radiotap.org/fields/EHT.html
    //
    if (IsPresent(presentBits, RADIOTAP_EHT))
    {
        SerializeEht(start);
    }
}

uint32_t
RadiotapHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    m_present.clear();

    auto tmp = start.ReadU8(); // major version of radiotap header
    NS_ASSERT_MSG(tmp == 0x00, "RadiotapHeader::Deserialize(): Unexpected major version");
    start.ReadU8(); // pad field

    m_length = start.ReadU16();              // entire length of radiotap data + header
    m_present.emplace_back(start.ReadU32()); // bits describing which fields follow header
    uint32_t bytesRead = MIN_HEADER_SIZE;

    std::size_t index{0};
    std::vector<std::bitset<RADIOTAP_BITS_PER_PRESENT_FIELD>> presentBits;
    presentBits.emplace_back(m_present.at(0));
    while (presentBits.at(index++).test(RADIOTAP_MORE_PRESENT))
    {
        // If bit 31 of the it_present field is set, another it_present bitmask is present.
        m_present.emplace_back(start.ReadU32());
        bytesRead += (RADIOTAP_BITS_PER_PRESENT_FIELD / 8);
        presentBits.emplace_back(m_present.at(index));
    }

    //
    // Time Synchronization Function Timer (when the first bit of the MPDU arrived at the MAC)
    // Reference: https://www.radiotap.org/fields/TSFT.html
    //
    if (IsPresent(presentBits, RADIOTAP_TSFT))
    {
        const auto size = DeserializeTsft(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // Properties of transmitted and received frames.
    // Reference: https://www.radiotap.org/fields/Flags.html
    //
    if (IsPresent(presentBits, RADIOTAP_FLAGS))
    {
        m_flags = start.ReadU8();
        ++bytesRead;
    }

    //
    // TX/RX data rate in units of 500 kbps
    // Reference: https://www.radiotap.org/fields/Rate.html
    //
    if (IsPresent(presentBits, RADIOTAP_RATE))
    {
        m_rate = start.ReadU8();
        ++bytesRead;
    }

    //
    // Tx/Rx frequency in MHz, followed by flags.
    // Reference: https://www.radiotap.org/fields/Channel.html
    //
    if (IsPresent(presentBits, RADIOTAP_CHANNEL))
    {
        const auto size = DeserializeChannel(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // RF signal power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20signal.html
    //
    if (IsPresent(presentBits, RADIOTAP_DBM_ANTSIGNAL))
    {
        m_antennaSignal = start.ReadU8();
        ++bytesRead;
    }

    //
    // RF noise power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20noise.html
    //
    if (IsPresent(presentBits, RADIOTAP_DBM_ANTNOISE))
    {
        m_antennaNoise = start.ReadU8();
        ++bytesRead;
    }

    //
    // MCS field.
    // Reference: https://www.radiotap.org/fields/MCS.html
    //
    if (IsPresent(presentBits, RADIOTAP_MCS))
    {
        const auto size = DeserializeMcs(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // A-MPDU Status, information about the received or transmitted A-MPDU.
    // Reference: https://www.radiotap.org/fields/A-MPDU%20status.html
    //
    if (IsPresent(presentBits, RADIOTAP_AMPDU_STATUS))
    {
        const auto size = DeserializeAmpduStatus(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // Information about the received or transmitted VHT frame.
    // Reference: https://www.radiotap.org/fields/VHT.html
    //
    if (IsPresent(presentBits, RADIOTAP_VHT))
    {
        const auto size = DeserializeVht(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // HE field.
    // Reference: https://www.radiotap.org/fields/HE.html
    //
    if (IsPresent(presentBits, RADIOTAP_HE))
    {
        const auto size = DeserializeHe(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // HE MU field.
    // Reference: https://www.radiotap.org/fields/HE-MU.html
    //
    if (IsPresent(presentBits, RADIOTAP_HE_MU))
    {
        const auto size = DeserializeHeMu(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // HE MU other user field.
    // Reference: https://www.radiotap.org/fields/HE-MU-other-user.html
    if (IsPresent(presentBits, RADIOTAP_HE_MU_OTHER_USER))
    {
        const auto size = DeserializeHeMuOtherUser(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // U-SIG field.
    // Reference: https://www.radiotap.org/fields/U-SIG.html
    //
    if (IsPresent(presentBits, RADIOTAP_USIG))
    {
        const auto size = DeserializeUsig(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    //
    // EHT field.
    // Reference: https://www.radiotap.org/fields/EHT.html
    //
    if (IsPresent(presentBits, RADIOTAP_EHT))
    {
        const auto size = DeserializeEht(start, bytesRead);
        start.Next(size);
        bytesRead += size;
    }

    NS_ASSERT_MSG(m_length == bytesRead,
                  "RadiotapHeader::Deserialize(): expected and actual lengths inconsistent ("
                      << m_length << " != " << bytesRead << ")");
    return bytesRead;
}

void
RadiotapHeader::Print(std::ostream& os) const
{
    os << " tsft=" << m_tsft << " flags=" << std::hex << m_flags << std::dec << " rate=" << +m_rate;
    if (m_present.at(0) & RADIOTAP_CHANNEL)
    {
        PrintChannel(os);
    }
    os << std::dec << " signal=" << +m_antennaSignal << " noise=" << +m_antennaNoise;
    if (m_present.at(0) & RADIOTAP_MCS)
    {
        PrintMcs(os);
    }
    if (m_present.at(0) & RADIOTAP_AMPDU_STATUS)
    {
        PrintAmpduStatus(os);
    }
    if (m_present.at(0) & RADIOTAP_VHT)
    {
        PrintVht(os);
    }
    if (m_present.at(0) & RADIOTAP_HE)
    {
        PrintHe(os);
    }
    if (m_present.at(0) & RADIOTAP_HE_MU)
    {
        PrintHeMu(os);
    }
    if (m_present.at(0) & RADIOTAP_HE_MU_OTHER_USER)
    {
        PrintHeMuOtherUser(os);
    }
    if ((m_present.size() > 1) && m_present.at(1) & RADIOTAP_USIG)
    {
        PrintUsig(os);
    }
    if ((m_present.size() > 1) && m_present.at(1) & RADIOTAP_EHT)
    {
        PrintEht(os);
    }
}

void
RadiotapHeader::UpdatePresentField(uint32_t field)
{
    NS_LOG_FUNCTION(this << field);
    const std::size_t bitmaskIdx = field / RADIOTAP_BITS_PER_PRESENT_FIELD;
    NS_ASSERT_MSG(bitmaskIdx < m_present.size(),
                  "Number of it_present words (" << m_present.size()
                                                 << ") less than expected index " << bitmaskIdx);
    const std::size_t fieldIdx = field % RADIOTAP_BITS_PER_PRESENT_FIELD;
    const uint32_t flag = (1 << fieldIdx);
    if (field != RADIOTAP_TLV)
    {
        NS_ASSERT_MSG(!(m_present.at(bitmaskIdx) & flag),
                      "Radiotap field " << field << " already set in present field at index "
                                        << bitmaskIdx);
    }
    if (const uint32_t morePresentWordFlag = (1 << RADIOTAP_MORE_PRESENT); bitmaskIdx > 0)
    {
        // Ensure that the bit indicating more present words is set in the previous word
        m_present.at(bitmaskIdx - 1) |= morePresentWordFlag;
    }
    m_present.at(bitmaskIdx) |= flag;
}

void
RadiotapHeader::SetWifiHeader(std::size_t numPresentWords)
{
    NS_LOG_FUNCTION(this << numPresentWords);
    NS_ASSERT_MSG(numPresentWords > 0,
                  "RadiotapHeader::SetWifiHeader() requires at least one it_present word");
    NS_ASSERT_MSG(m_length == MIN_HEADER_SIZE,
                  "RadiotapHeader::SetWifiHeader() should be called before any other Set* method");
    NS_ASSERT_MSG(m_present.size() == 1, "RadiotapHeader::SetWifiHeader() should be called once");
    for (std::size_t i = 0; i < (numPresentWords - 1); ++i)
    {
        m_present.emplace_back(0);
        m_length += (RADIOTAP_BITS_PER_PRESENT_FIELD / 8);
    }
}

void
RadiotapHeader::SetTsft(uint64_t value)
{
    NS_LOG_FUNCTION(this << value);

    UpdatePresentField(RADIOTAP_TSFT);
    m_tsftPad = GetPadding(m_length, RADIOTAP_TSFT_ALIGNMENT_B);
    m_length += RADIOTAP_TSFT_SIZE_B + m_tsftPad;
    m_tsft = value;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeTsft(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_tsftPad);
    start.WriteU64(m_tsft);
}

uint32_t
RadiotapHeader::DeserializeTsft(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_tsftPad = GetPadding(bytesRead, RADIOTAP_TSFT_ALIGNMENT_B);
    start.Next(m_tsftPad);
    m_tsft = start.ReadU64();
    return RADIOTAP_TSFT_SIZE_B + m_tsftPad;
}

void
RadiotapHeader::SetFrameFlags(uint8_t flags)
{
    NS_LOG_FUNCTION(this << flags);

    UpdatePresentField(RADIOTAP_FLAGS);
    m_length += RADIOTAP_FLAGS_SIZE_B;
    m_flags = flags;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SetRate(uint8_t rate)
{
    NS_LOG_FUNCTION(this << rate);

    UpdatePresentField(RADIOTAP_RATE);
    m_length += RADIOTAP_RATE_SIZE_B;
    m_rate = rate;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SetChannelFields(const ChannelFields& channelFields)
{
    NS_LOG_FUNCTION(this << channelFields.frequency << channelFields.flags);

    UpdatePresentField(RADIOTAP_CHANNEL);
    m_channelPad = GetPadding(m_length, RADIOTAP_CHANNEL_ALIGNMENT_B);
    m_length += RADIOTAP_CHANNEL_SIZE_B + m_channelPad;
    m_channelFields = channelFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeChannel(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_channelPad);
    start.WriteU16(m_channelFields.frequency);
    start.WriteU16(m_channelFields.flags);
}

uint32_t
RadiotapHeader::DeserializeChannel(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_channelPad = GetPadding(bytesRead, RADIOTAP_CHANNEL_ALIGNMENT_B);
    start.Next(m_channelPad);
    m_channelFields.frequency = start.ReadU16();
    m_channelFields.flags = start.ReadU16();
    return RADIOTAP_CHANNEL_SIZE_B + m_channelPad;
}

void
RadiotapHeader::PrintChannel(std::ostream& os) const
{
    os << " channel.frequency=" << m_channelFields.frequency << " channel.flags=0x" << std::hex
       << m_channelFields.flags << std::dec;
}

void
RadiotapHeader::SetAntennaSignalPower(double signal)
{
    NS_LOG_FUNCTION(this << signal);

    UpdatePresentField(RADIOTAP_DBM_ANTSIGNAL);
    m_length += RADIOTAP_ANTENNA_SIGNAL_SIZE_B;

    if (signal > 127)
    {
        m_antennaSignal = 127;
    }
    else if (signal < -128)
    {
        m_antennaSignal = -128;
    }
    else
    {
        m_antennaSignal = static_cast<int8_t>(floor(signal + 0.5));
    }

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SetAntennaNoisePower(double noise)
{
    NS_LOG_FUNCTION(this << noise);

    UpdatePresentField(RADIOTAP_DBM_ANTNOISE);
    m_length += RADIOTAP_ANTENNA_NOISE_SIZE_B;

    if (noise > 127.0)
    {
        m_antennaNoise = 127;
    }
    else if (noise < -128.0)
    {
        m_antennaNoise = -128;
    }
    else
    {
        m_antennaNoise = static_cast<int8_t>(floor(noise + 0.5));
    }

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SetMcsFields(const McsFields& mcsFields)
{
    NS_LOG_FUNCTION(this << mcsFields.known << mcsFields.flags << mcsFields.mcs);

    UpdatePresentField(RADIOTAP_MCS);
    m_mcsPad = GetPadding(m_length, RADIOTAP_MCS_ALIGNMENT_B);
    m_length += RADIOTAP_MCS_SIZE_B + m_mcsPad;
    m_mcsFields = mcsFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeMcs(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_mcsPad);
    start.WriteU8(m_mcsFields.known);
    start.WriteU8(m_mcsFields.flags);
    start.WriteU8(m_mcsFields.mcs);
}

uint32_t
RadiotapHeader::DeserializeMcs(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_mcsPad = GetPadding(bytesRead, RADIOTAP_MCS_ALIGNMENT_B);
    start.Next(m_mcsPad);
    m_mcsFields.known = start.ReadU8();
    m_mcsFields.flags = start.ReadU8();
    m_mcsFields.mcs = start.ReadU8();
    return RADIOTAP_MCS_SIZE_B + m_mcsPad;
}

void
RadiotapHeader::PrintMcs(std::ostream& os) const
{
    os << " mcs.known=0x" << std::hex << +m_mcsFields.known << " mcs.flags0x=" << +m_mcsFields.flags
       << " mcsRate=" << std::dec << +m_mcsFields.mcs;
}

void
RadiotapHeader::SetAmpduStatus(const AmpduStatusFields& ampduStatusFields)
{
    NS_LOG_FUNCTION(this << ampduStatusFields.referenceNumber << ampduStatusFields.flags);

    UpdatePresentField(RADIOTAP_AMPDU_STATUS);
    m_ampduStatusPad = GetPadding(m_length, RADIOTAP_AMPDU_STATUS_ALIGNMENT_B);
    m_length += RADIOTAP_AMPDU_STATUS_SIZE_B + m_ampduStatusPad;
    m_ampduStatusFields = ampduStatusFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeAmpduStatus(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_ampduStatusPad);
    start.WriteU32(m_ampduStatusFields.referenceNumber);
    start.WriteU16(m_ampduStatusFields.flags);
    start.WriteU8(m_ampduStatusFields.crc);
    start.WriteU8(m_ampduStatusFields.reserved);
}

uint32_t
RadiotapHeader::DeserializeAmpduStatus(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_ampduStatusPad = GetPadding(bytesRead, RADIOTAP_AMPDU_STATUS_ALIGNMENT_B);
    start.Next(m_ampduStatusPad);
    m_ampduStatusFields.referenceNumber = start.ReadU32();
    m_ampduStatusFields.flags = start.ReadU16();
    m_ampduStatusFields.crc = start.ReadU8();
    m_ampduStatusFields.reserved = start.ReadU8();
    return RADIOTAP_AMPDU_STATUS_SIZE_B + m_ampduStatusPad;
}

void
RadiotapHeader::PrintAmpduStatus(std::ostream& os) const
{
    os << " ampduStatus.flags=0x" << std::hex << m_ampduStatusFields.flags << std::dec;
}

void
RadiotapHeader::SetVhtFields(const VhtFields& vhtFields)
{
    NS_LOG_FUNCTION(this << vhtFields.known << vhtFields.flags << vhtFields.mcsNss.at(0)
                         << vhtFields.mcsNss.at(1) << vhtFields.mcsNss.at(2)
                         << vhtFields.mcsNss.at(3) << vhtFields.coding << vhtFields.groupId
                         << vhtFields.partialAid);

    UpdatePresentField(RADIOTAP_VHT);
    m_vhtPad = GetPadding(m_length, RADIOTAP_VHT_ALIGNMENT_B);
    m_length += RADIOTAP_VHT_SIZE_B + m_vhtPad;
    m_vhtFields = vhtFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeVht(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_vhtPad);
    start.WriteU16(m_vhtFields.known);
    start.WriteU8(m_vhtFields.flags);
    start.WriteU8(m_vhtFields.bandwidth);
    for (const auto mcsNss : m_vhtFields.mcsNss)
    {
        start.WriteU8(mcsNss);
    }
    start.WriteU8(m_vhtFields.coding);
    start.WriteU8(m_vhtFields.groupId);
    start.WriteU16(m_vhtFields.partialAid);
}

uint32_t
RadiotapHeader::DeserializeVht(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_vhtPad = GetPadding(bytesRead, RADIOTAP_VHT_ALIGNMENT_B);
    start.Next(m_vhtPad);
    m_vhtFields.known = start.ReadU16();
    m_vhtFields.flags = start.ReadU8();
    m_vhtFields.bandwidth = start.ReadU8();
    for (auto& mcsNss : m_vhtFields.mcsNss)
    {
        mcsNss = start.ReadU8();
    }
    m_vhtFields.coding = start.ReadU8();
    m_vhtFields.groupId = start.ReadU8();
    m_vhtFields.partialAid = start.ReadU16();
    return RADIOTAP_VHT_SIZE_B + m_vhtPad;
}

void
RadiotapHeader::PrintVht(std::ostream& os) const
{
    os << " vht.known=0x" << m_vhtFields.known << " vht.flags=0x" << m_vhtFields.flags
       << " vht.bandwidth=" << std::dec << m_vhtFields.bandwidth;
    for (std::size_t i = 0; i < m_vhtFields.mcsNss.size(); ++i)
    {
        os << " vht.mcsNss[" << i << "]=" << +m_vhtFields.mcsNss.at(i);
    }
    os << " vht.coding=" << m_vhtFields.coding << " vht.groupId=" << m_vhtFields.groupId
       << " vht.partialAid=" << m_vhtFields.partialAid;
}

void
RadiotapHeader::SetHeFields(const HeFields& heFields)
{
    NS_LOG_FUNCTION(this << heFields.data1 << heFields.data2 << heFields.data3 << heFields.data4
                         << heFields.data5 << heFields.data6);

    UpdatePresentField(RADIOTAP_HE);
    m_hePad = GetPadding(m_length, RADIOTAP_HE_ALIGNMENT_B);
    m_length += RADIOTAP_HE_SIZE_B + m_hePad;
    m_heFields = heFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeHe(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_hePad);
    start.WriteU16(m_heFields.data1);
    start.WriteU16(m_heFields.data2);
    start.WriteU16(m_heFields.data3);
    start.WriteU16(m_heFields.data4);
    start.WriteU16(m_heFields.data5);
    start.WriteU16(m_heFields.data6);
}

uint32_t
RadiotapHeader::DeserializeHe(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_hePad = GetPadding(bytesRead, RADIOTAP_HE_ALIGNMENT_B);
    start.Next(m_hePad);
    m_heFields.data1 = start.ReadU16();
    m_heFields.data2 = start.ReadU16();
    m_heFields.data3 = start.ReadU16();
    m_heFields.data4 = start.ReadU16();
    m_heFields.data5 = start.ReadU16();
    m_heFields.data6 = start.ReadU16();
    return RADIOTAP_HE_SIZE_B + m_hePad;
}

void
RadiotapHeader::PrintHe(std::ostream& os) const
{
    os << " he.data1=0x" << std::hex << m_heFields.data1 << " he.data2=0x" << std::hex
       << m_heFields.data2 << " he.data3=0x" << std::hex << m_heFields.data3 << " he.data4=0x"
       << std::hex << m_heFields.data4 << " he.data5=0x" << std::hex << m_heFields.data5
       << " he.data6=0x" << std::hex << m_heFields.data6 << std::dec;
}

void
RadiotapHeader::SetHeMuFields(const HeMuFields& heMuFields)
{
    NS_LOG_FUNCTION(this << heMuFields.flags1 << heMuFields.flags2);

    UpdatePresentField(RADIOTAP_HE_MU);
    m_heMuPad = GetPadding(m_length, RADIOTAP_HE_MU_ALIGNMENT_B);
    m_length += RADIOTAP_HE_MU_SIZE_B + m_heMuPad;
    m_heMuFields = heMuFields;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                             << std::dec);
}

void
RadiotapHeader::SerializeHeMu(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_heMuPad);
    start.WriteU16(m_heMuFields.flags1);
    start.WriteU16(m_heMuFields.flags2);
    for (const auto ruChannel : m_heMuFields.ruChannel1)
    {
        start.WriteU8(ruChannel);
    }
    for (const auto ruChannel : m_heMuFields.ruChannel2)
    {
        start.WriteU8(ruChannel);
    }
}

uint32_t
RadiotapHeader::DeserializeHeMu(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_heMuPad = GetPadding(bytesRead, RADIOTAP_HE_MU_ALIGNMENT_B);
    start.Next(m_heMuPad);
    m_heMuFields.flags1 = start.ReadU16();
    m_heMuFields.flags2 = start.ReadU16();
    for (auto& ruChannel : m_heMuFields.ruChannel1)
    {
        ruChannel = start.ReadU8();
    }
    for (auto& ruChannel : m_heMuFields.ruChannel2)
    {
        ruChannel = start.ReadU8();
    }
    return RADIOTAP_HE_MU_SIZE_B + m_heMuPad;
}

void
RadiotapHeader::PrintHeMu(std::ostream& os) const
{
    os << " heMu.flags1=0x" << std::hex << m_heMuFields.flags1 << " heMu.flags2=0x"
       << m_heMuFields.flags2 << std::dec;
}

void
RadiotapHeader::SetHeMuOtherUserFields(const HeMuOtherUserFields& heMuOtherUserFields)
{
    NS_LOG_FUNCTION(this << heMuOtherUserFields.perUser1 << heMuOtherUserFields.perUser2
                         << heMuOtherUserFields.perUserPosition
                         << heMuOtherUserFields.perUserKnown);

    UpdatePresentField(RADIOTAP_HE_MU_OTHER_USER);
    m_heMuOtherUserPad = GetPadding(m_length, RADIOTAP_HE_MU_OTHER_USER_ALIGNMENT_B);
    m_length += RADIOTAP_HE_MU_OTHER_USER_SIZE_B + m_heMuOtherUserPad;
    m_heMuOtherUserFields = heMuOtherUserFields;

    NS_LOG_LOGIC(" m_length=" << m_length << " m_present=0x" << std::hex << m_present.at(0)
                              << std::dec);
}

void
RadiotapHeader::SerializeHeMuOtherUser(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_heMuOtherUserPad);
    start.WriteU16(m_heMuOtherUserFields.perUser1);
    start.WriteU16(m_heMuOtherUserFields.perUser2);
    start.WriteU8(m_heMuOtherUserFields.perUserPosition);
    start.WriteU8(m_heMuOtherUserFields.perUserKnown);
}

uint32_t
RadiotapHeader::DeserializeHeMuOtherUser(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    m_heMuOtherUserPad = GetPadding(bytesRead, RADIOTAP_HE_MU_OTHER_USER_ALIGNMENT_B);
    start.Next(m_heMuOtherUserPad);
    m_heMuOtherUserFields.perUser1 = start.ReadU16();
    m_heMuOtherUserFields.perUser2 = start.ReadU16();
    m_heMuOtherUserFields.perUserPosition = start.ReadU8();
    m_heMuOtherUserFields.perUserKnown = start.ReadU8();
    return RADIOTAP_HE_MU_OTHER_USER_SIZE_B + m_heMuOtherUserPad;
}

void
RadiotapHeader::PrintHeMuOtherUser(std::ostream& os) const
{
    os << " heMuOtherUser.perUser1=" << m_heMuOtherUserFields.perUser1
       << " heMuOtherUser.perUser2=" << m_heMuOtherUserFields.perUser2
       << " heMuOtherUser.perUserPosition=" << m_heMuOtherUserFields.perUserPosition
       << " heMuOtherUser.perUserKnown=0x" << std::hex << m_heMuOtherUserFields.perUserKnown
       << std::dec;
}

void
RadiotapHeader::SetUsigFields(const UsigFields& usigFields)
{
    NS_LOG_FUNCTION(this << usigFields.common << usigFields.mask << usigFields.value);

    UpdatePresentField(RADIOTAP_TLV);
    m_usigTlvPad = GetPadding(m_length, RADIOTAP_TLV_ALIGNMENT_B);
    m_usigTlv.type = RADIOTAP_USIG;
    m_usigTlv.length = RADIOTAP_USIG_SIZE_B;
    m_length += RADIOTAP_TLV_HEADER_SIZE_B + m_usigTlvPad;

    UpdatePresentField(RADIOTAP_USIG);
    m_usigPad = GetPadding(m_length, RADIOTAP_USIG_ALIGNMENT_B);
    m_usigFields = usigFields;
    m_length += m_usigTlv.length + m_usigPad;

    NS_LOG_LOGIC(" m_length=" << m_length << " m_present[0]=0x" << std::hex << m_present.at(0)
                              << " m_present[1]=0x" << m_present.at(1) << std::dec);
}

void
RadiotapHeader::SerializeUsig(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_usigTlvPad);
    start.WriteU16(m_usigTlv.type);
    start.WriteU16(m_usigTlv.length);
    start.WriteU8(0, m_usigPad);
    start.WriteU32(m_usigFields.common);
    start.WriteU32(m_usigFields.value);
    start.WriteU32(m_usigFields.mask);
}

uint32_t
RadiotapHeader::DeserializeUsig(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);
    const auto startBytesRead = bytesRead;
    m_usigTlvPad = GetPadding(bytesRead, RADIOTAP_TLV_ALIGNMENT_B);
    start.Next(m_usigTlvPad);
    bytesRead += m_usigTlvPad;
    m_usigTlv.type = start.ReadU16();
    m_usigTlv.length = start.ReadU16();
    bytesRead += RADIOTAP_TLV_HEADER_SIZE_B;
    m_usigPad = GetPadding(bytesRead, RADIOTAP_USIG_ALIGNMENT_B);
    start.Next(m_usigPad);
    bytesRead += m_usigPad;
    m_usigFields.common = start.ReadU32();
    m_usigFields.value = start.ReadU32();
    m_usigFields.mask = start.ReadU32();
    bytesRead += RADIOTAP_USIG_SIZE_B;
    return bytesRead - startBytesRead;
}

void
RadiotapHeader::PrintUsig(std::ostream& os) const
{
    os << " usig.common=0x" << std::hex << m_usigFields.common << " usig.value=0x"
       << m_usigFields.value << " usig.mask=0x" << m_usigFields.mask << std::dec;
}

void
RadiotapHeader::SetEhtFields(const EhtFields& ehtFields)
{
    NS_LOG_FUNCTION(this << ehtFields.known);

    UpdatePresentField(RADIOTAP_TLV);
    m_ehtTlvPad = GetPadding(m_length, RADIOTAP_TLV_ALIGNMENT_B);
    m_ehtTlv.type = RADIOTAP_EHT;
    m_ehtTlv.length = RADIOTAP_EHT_KNOWN_SIZE_B +
                      (RADIOTAP_EHT_DATA_SIZE_B * ehtFields.data.size()) +
                      (ehtFields.userInfo.size() * RADIOTAP_EHT_USER_INFO_SIZE_B);
    m_length += RADIOTAP_TLV_HEADER_SIZE_B + m_ehtTlvPad;

    UpdatePresentField(RADIOTAP_EHT);
    m_ehtPad = GetPadding(m_length, RADIOTAP_EHT_ALIGNMENT_B);
    m_ehtFields = ehtFields;
    m_length += m_ehtTlv.length + m_ehtPad;

    NS_LOG_LOGIC("m_length=" << m_length << " m_present[0]=0x" << std::hex << m_present.at(0)
                             << " m_present[1]=0x" << m_present.at(1) << std::dec);
}

void
RadiotapHeader::SerializeEht(Buffer::Iterator& start) const
{
    NS_LOG_FUNCTION(this << &start);
    start.WriteU8(0, m_ehtTlvPad);
    start.WriteU16(m_ehtTlv.type);
    start.WriteU16(m_ehtTlv.length);
    start.WriteU8(0, m_ehtPad);
    start.WriteU32(m_ehtFields.known);
    for (const auto dataField : m_ehtFields.data)
    {
        start.WriteU32(dataField);
    }
    for (const auto userInfoField : m_ehtFields.userInfo)
    {
        start.WriteU32(userInfoField);
    }
}

uint32_t
RadiotapHeader::DeserializeEht(Buffer::Iterator start, uint32_t bytesRead)
{
    NS_LOG_FUNCTION(this << &start << bytesRead);

    const auto startBytesRead = bytesRead;

    m_ehtTlvPad = GetPadding(bytesRead, RADIOTAP_TLV_ALIGNMENT_B);
    start.Next(m_ehtTlvPad);
    bytesRead += m_ehtTlvPad;
    m_ehtTlv.type = start.ReadU16();
    m_ehtTlv.length = start.ReadU16();
    bytesRead += RADIOTAP_TLV_HEADER_SIZE_B;

    m_ehtPad = GetPadding(bytesRead, RADIOTAP_EHT_ALIGNMENT_B);
    start.Next(m_ehtPad);
    bytesRead += m_ehtPad;
    m_ehtFields.known = start.ReadU32();
    bytesRead += RADIOTAP_EHT_KNOWN_SIZE_B;
    for (auto& dataField : m_ehtFields.data)
    {
        dataField = start.ReadU32();
        bytesRead += RADIOTAP_EHT_DATA_SIZE_B;
    }
    const auto userInfosBytes = m_ehtTlv.length - bytesRead - m_ehtTlvPad;
    NS_ASSERT(userInfosBytes % RADIOTAP_EHT_USER_INFO_SIZE_B == 0);
    const std::size_t numUsers = userInfosBytes / RADIOTAP_EHT_USER_INFO_SIZE_B;
    for (std::size_t i = 0; i < numUsers; ++i)
    {
        m_ehtFields.userInfo.push_back(start.ReadU32());
        bytesRead += RADIOTAP_EHT_USER_INFO_SIZE_B;
    }

    return bytesRead - startBytesRead;
}

void
RadiotapHeader::PrintEht(std::ostream& os) const
{
    os << " eht.known=0x" << std::hex << m_ehtFields.known;
    std::size_t index = 0;
    for (const auto dataField : m_ehtFields.data)
    {
        os << " eht.data" << index++ << "=0x" << dataField;
    }
    index = 0;
    for (const auto userInfoField : m_ehtFields.userInfo)
    {
        os << " eht.userInfo" << index++ << "=0x" << userInfoField;
    }
    os << std::dec;
}

} // namespace ns3
