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

#include <bit>
#include <cmath>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RadiotapHeader");

NS_OBJECT_ENSURE_REGISTERED(RadiotapHeader);

RadiotapHeader::RadiotapHeader()
{
    NS_LOG_FUNCTION(this);
}

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
    NS_LOG_FUNCTION(this);
    return m_length;
}

void
RadiotapHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);

    start.WriteU8(0);          // major version of radiotap header
    start.WriteU8(0);          // pad field
    start.WriteU16(m_length);  // entire length of radiotap data + header
    start.WriteU32(m_present); // bits describing which fields follow header
    if (m_presentExt)
    {
        start.WriteU32(*m_presentExt); // extended bitmasks
    }

    //
    // Time Synchronization Function Timer (when the first bit of the MPDU
    // arrived at the MAC)
    // Reference: https://www.radiotap.org/fields/TSFT.html
    //
    if (m_present & RADIOTAP_TSFT) // bit 0
    {
        start.WriteU64(m_tsft);
    }

    //
    // Properties of transmitted and received frames.
    // Reference: https://www.radiotap.org/fields/Flags.html
    //
    if (m_present & RADIOTAP_FLAGS) // bit 1
    {
        start.WriteU8(m_flags);
    }

    //
    // TX/RX data rate in units of 500 kbps
    // Reference: https://www.radiotap.org/fields/Rate.html
    //
    if (m_present & RADIOTAP_RATE) // bit 2
    {
        start.WriteU8(m_rate);
    }

    //
    // Tx/Rx frequency in MHz, followed by flags.
    // Reference: https://www.radiotap.org/fields/Channel.html
    //
    if (m_present & RADIOTAP_CHANNEL) // bit 3
    {
        SerializeChannel(start);
    }

    //
    // The hop set and pattern for frequency-hopping radios.  We don't need it but
    // still need to account for it.
    // Reference: https://www.radiotap.org/fields/FHSS.html
    //
    if (m_present & RADIOTAP_FHSS) // bit 4
    {
        start.WriteU8(0); // not yet implemented
    }

    //
    // RF signal power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20signal.html
    //
    if (m_present & RADIOTAP_DBM_ANTSIGNAL) // bit 5
    {
        start.WriteU8(m_antennaSignal);
    }

    //
    // RF noise power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20noise.html
    //
    if (m_present & RADIOTAP_DBM_ANTNOISE) // bit 6
    {
        start.WriteU8(m_antennaNoise);
    }

    //
    // Quality of Barker code lock.
    // Reference: https://www.radiotap.org/fields/Lock%20quality.html
    //
    if (m_present & RADIOTAP_LOCK_QUALITY) // bit 7
    {
        start.WriteU16(0); // not yet implemented
    }

    //
    // Transmit power expressed as unitless distance from max power
    // set at factory calibration (0 is max power).
    // Reference: https://www.radiotap.org/fields/TX%20attenuation.html
    //
    if (m_present & RADIOTAP_TX_ATTENUATION) // bit 8
    {
        start.WriteU16(0); // not yet implemented
    }

    //
    // Transmit power expressed as decibel distance from max power
    // set at factory calibration (0 is max power).
    // Reference: https://www.radiotap.org/fields/dB%20TX%20attenuation.html
    //
    if (m_present & RADIOTAP_DB_TX_ATTENUATION) // bit 9
    {
        start.WriteU16(0); // not yet implemented
    }

    //
    // Transmit power expressed as dBm (decibels from a 1 milliwatt reference).
    // This is the absolute power level measured at the antenna port.
    // Reference: https://www.radiotap.org/fields/dBm%20TX%20power.html
    //
    if (m_present & RADIOTAP_DBM_TX_POWER) // bit 10
    {
        start.WriteU8(0); // not yet implemented
    }

    //
    // Unitless indication of the Rx/Tx antenna for this packet.
    // The first antenna is antenna 0.
    // Reference: https://www.radiotap.org/fields/Antenna.html
    //
    if (m_present & RADIOTAP_ANTENNA) // bit 11
    {
        start.WriteU8(0); // not yet implemented
    }

    //
    // RF signal power at the antenna (decibel difference from an arbitrary fixed reference).
    // Reference: https://www.radiotap.org/fields/dB%20antenna%20signal.html
    //
    if (m_present & RADIOTAP_DB_ANTSIGNAL) // bit 12
    {
        start.WriteU8(0); // not yet implemented
    }

    //
    // RF noise power at the antenna (decibel difference from an arbitrary fixed reference).
    // Reference: https://www.radiotap.org/fields/dB%20antenna%20noise.html
    //
    if (m_present & RADIOTAP_DB_ANTNOISE) // bit 13
    {
        start.WriteU8(0); // not yet implemented
    }

    //
    // Properties of received frames.
    // Reference: https://www.radiotap.org/fields/RX%20flags.html
    //
    if (m_present & RADIOTAP_RX_FLAGS) // bit 14
    {
        start.WriteU16(0); // not yet implemented
    }

    //
    // MCS field.
    // Reference: https://www.radiotap.org/fields/MCS.html
    //
    if (m_present & RADIOTAP_MCS) // bit 19
    {
        SerializeMcs(start);
    }

    //
    // A-MPDU Status, information about the received or transmitted A-MPDU.
    // Reference: https://www.radiotap.org/fields/A-MPDU%20status.html
    //
    if (m_present & RADIOTAP_AMPDU_STATUS) // bit 20
    {
        SerializeAmpduStatus(start);
    }

    //
    // Information about the received or transmitted VHT frame.
    // Reference: https://www.radiotap.org/fields/VHT.html
    //
    if (m_present & RADIOTAP_VHT) // bit 21
    {
        SerializeVht(start);
    }

    //
    // HE field.
    // Reference: https://www.radiotap.org/fields/HE.html
    //
    if (m_present & RADIOTAP_HE) // bit 23
    {
        SerializeHe(start);
    }

    //
    // HE MU field.
    // Reference: https://www.radiotap.org/fields/HE-MU.html
    //
    if (m_present & RADIOTAP_HE_MU) // bit 24
    {
        SerializeHeMu(start);
    }

    //
    // HE MU other user field.
    // Reference: https://www.radiotap.org/fields/HE-MU-other-user.html
    //
    if (m_present & RADIOTAP_HE_MU_OTHER_USER) // bit 25
    {
        SerializeHeMuOtherUser(start);
    }

    //
    // U-SIG field.
    // Reference: https://www.radiotap.org/fields/U-SIG.html
    //
    if (m_presentExt && (*m_presentExt & RADIOTAP_USIG)) // bit 33
    {
        SerializeUsig(start);
    }

    //
    // EHT field.
    // Reference: https://www.radiotap.org/fields/EHT.html
    //
    if (m_presentExt && (*m_presentExt & RADIOTAP_EHT_SIG)) // bit 34
    {
        SerializeEht(start);
    }
}

uint32_t
RadiotapHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);

    uint8_t tmp = start.ReadU8(); // major version of radiotap header
    NS_ASSERT_MSG(tmp == 0x00, "RadiotapHeader::Deserialize(): Unexpected major version");
    start.ReadU8(); // pad field

    m_length = start.ReadU16();  // entire length of radiotap data + header
    m_present = start.ReadU32(); // bits describing which fields follow header
    uint32_t bytesRead = 8;

    if (m_present & RADIOTAP_EXT)
    {
        // If bit 31 of the it_present field is set, an extended it_present bitmask is present.
        m_presentExt = start.ReadU32();
        bytesRead += 4;
    }

    //
    // Time Synchronization Function Timer (when the first bit of the MPDU arrived at the MAC)
    // Reference: https://www.radiotap.org/fields/TSFT.html
    //
    if (m_present & RADIOTAP_TSFT) // bit 0
    {
        m_tsft = start.ReadU64();
        bytesRead += 8;
    }

    //
    // Properties of transmitted and received frames.
    // Reference: https://www.radiotap.org/fields/Flags.html
    //
    if (m_present & RADIOTAP_FLAGS) // bit 1
    {
        m_flags = start.ReadU8();
        ++bytesRead;
    }

    //
    // TX/RX data rate in units of 500 kbps
    // Reference: https://www.radiotap.org/fields/Rate.html
    //
    if (m_present & RADIOTAP_RATE) // bit 2
    {
        m_rate = start.ReadU8();
        ++bytesRead;
    }

    //
    // Tx/Rx frequency in MHz, followed by flags.
    // Reference: https://www.radiotap.org/fields/Channel.html
    //
    if (m_present & RADIOTAP_CHANNEL) // bit 3
    {
        bytesRead += DeserializeChannel(start, bytesRead);
    }

    //
    // The hop set and pattern for frequency-hopping radios.  We don't need it but
    // still need to account for it.
    // Reference: https://www.radiotap.org/fields/FHSS.html
    //
    if (m_present & RADIOTAP_FHSS) // bit 4
    {
        // not yet implemented
        start.ReadU8();
        ++bytesRead;
    }

    //
    // RF signal power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20signal.html
    //
    if (m_present & RADIOTAP_DBM_ANTSIGNAL) // bit 5
    {
        m_antennaSignal = start.ReadU8();
        ++bytesRead;
    }

    //
    // RF noise power at the antenna, decibel difference from an arbitrary, fixed
    // reference.
    // Reference: https://www.radiotap.org/fields/Antenna%20noise.html
    //
    if (m_present & RADIOTAP_DBM_ANTNOISE) // bit 6
    {
        m_antennaNoise = start.ReadU8();
        ++bytesRead;
    }

    //
    // Quality of Barker code lock.
    // Reference: https://www.radiotap.org/fields/Lock%20quality.html
    //
    if (m_present & RADIOTAP_LOCK_QUALITY) // bit 7
    {
        // not yet implemented
        start.ReadU16();
        bytesRead += 2;
    }

    //
    // Transmit power expressed as unitless distance from max power
    // set at factory calibration (0 is max power).
    // Reference: https://www.radiotap.org/fields/TX%20attenuation.html
    //
    if (m_present & RADIOTAP_TX_ATTENUATION) // bit 8
    {
        // not yet implemented
        start.ReadU16();
        bytesRead += 2;
    }

    //
    // Transmit power expressed as decibel distance from max power
    // set at factory calibration (0 is max power).
    // Reference: https://www.radiotap.org/fields/dB%20TX%20attenuation.html
    //
    if (m_present & RADIOTAP_DB_TX_ATTENUATION) // bit 9
    {
        // not yet implemented
        start.ReadU16();
        bytesRead += 2;
    }

    //
    // Transmit power expressed as dBm (decibels from a 1 milliwatt reference).
    // This is the absolute power level measured at the antenna port.
    // Reference: https://www.radiotap.org/fields/dBm%20TX%20power.html
    //
    if (m_present & RADIOTAP_DBM_TX_POWER) // bit 10
    {
        // not yet implemented
        start.ReadU8();
        ++bytesRead;
    }

    //
    // Unitless indication of the Rx/Tx antenna for this packet.
    // The first antenna is antenna 0.
    // Reference: https://www.radiotap.org/fields/Antenna.html
    //
    if (m_present & RADIOTAP_ANTENNA) // bit 11
    {
        // not yet implemented
        start.ReadU8();
        ++bytesRead;
    }

    //
    // RF signal power at the antenna (decibel difference from an arbitrary fixed reference).
    // Reference: https://www.radiotap.org/fields/dB%20antenna%20signal.html
    //
    if (m_present & RADIOTAP_DB_ANTSIGNAL) // bit 12
    {
        // not yet implemented
        start.ReadU8();
        ++bytesRead;
    }

    //
    // RF noise power at the antenna (decibel difference from an arbitrary fixed reference).
    // Reference: https://www.radiotap.org/fields/dB%20antenna%20noise.html
    //
    if (m_present & RADIOTAP_DB_ANTNOISE) // bit 13
    {
        // not yet implemented
        start.ReadU8();
        ++bytesRead;
    }

    //
    // Properties of received frames.
    // Reference: https://www.radiotap.org/fields/RX%20flags.html
    //
    if (m_present & RADIOTAP_RX_FLAGS) // bit 14
    {
        // not yet implemented
        start.ReadU16();
        bytesRead += 2;
    }

    //
    // MCS field.
    // Reference: https://www.radiotap.org/fields/MCS.html
    //
    if (m_present & RADIOTAP_MCS) // bit 19
    {
        bytesRead += DeserializeMcs(start, bytesRead);
    }

    //
    // A-MPDU Status, information about the received or transmitted A-MPDU.
    // Reference: https://www.radiotap.org/fields/A-MPDU%20status.html
    //
    if (m_present & RADIOTAP_AMPDU_STATUS)
    {
        bytesRead += DeserializeAmpduStatus(start, bytesRead);
    }

    //
    // Information about the received or transmitted VHT frame.
    // Reference: https://www.radiotap.org/fields/VHT.html
    //
    if (m_present & RADIOTAP_VHT) // bit 21
    {
        bytesRead += DeserializeVht(start, bytesRead);
    }

    //
    // HE field.
    // Reference: https://www.radiotap.org/fields/HE.html
    //
    if (m_present & RADIOTAP_HE) // bit 23
    {
        bytesRead += DeserializeHe(start, bytesRead);
    }

    //
    // HE MU field.
    // Reference: https://www.radiotap.org/fields/HE-MU.html
    //
    if (m_present & RADIOTAP_HE_MU) // bit 24
    {
        bytesRead += DeserializeHeMu(start, bytesRead);
    }

    //
    // HE MU other user field.
    // Reference: https://www.radiotap.org/fields/HE-MU-other-user.html
    //
    if (m_present & RADIOTAP_HE_MU_OTHER_USER) // bit 25
    {
        bytesRead += DeserializeHeMuOtherUser(start, bytesRead);
    }

    //
    // U-SIG field.
    // Reference: https://www.radiotap.org/fields/U-SIG.html
    //
    if (m_presentExt && (*m_presentExt & RADIOTAP_USIG)) // bit 33
    {
        bytesRead += DeserializeUsig(start, bytesRead);
    }

    //
    // EHT field.
    // Reference: https://www.radiotap.org/fields/EHT.html
    //
    if (m_presentExt && (*m_presentExt & RADIOTAP_EHT_SIG)) // bit 34
    {
        bytesRead += DeserializeEht(start, bytesRead);
    }

    NS_ASSERT_MSG(m_length == bytesRead,
                  "RadiotapHeader::Deserialize(): expected and actual lengths inconsistent");
    return bytesRead;
}

void
RadiotapHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << " tsft=" << m_tsft << " flags=" << std::hex << m_flags << std::dec << " rate=" << +m_rate;
    if (m_present & RADIOTAP_CHANNEL)
    {
        PrintChannel(os);
    }
    os << std::dec << " signal=" << +m_antennaSignal << " noise=" << +m_antennaNoise;
    if (m_present & RADIOTAP_MCS)
    {
        PrintMcs(os);
    }
    if (m_present & RADIOTAP_AMPDU_STATUS)
    {
        PrintAmpduStatus(os);
    }
    if (m_present & RADIOTAP_VHT)
    {
        PrintVht(os);
    }
    if (m_present & RADIOTAP_HE)
    {
        PrintHe(os);
    }
    if (m_present & RADIOTAP_HE_MU)
    {
        PrintHeMu(os);
    }
    if (m_present & RADIOTAP_HE_MU_OTHER_USER)
    {
        PrintHeMuOtherUser(os);
    }
    if (m_presentExt && (*m_presentExt & RADIOTAP_USIG))
    {
        PrintUsig(os);
    }
    if (m_presentExt && (*m_presentExt & RADIOTAP_EHT_SIG))
    {
        PrintEht(os);
    }
}

void
RadiotapHeader::SetTsft(uint64_t value)
{
    NS_LOG_FUNCTION(this << value);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_TSFT), "TSFT radiotap field already present");
    m_present |= RADIOTAP_TSFT;
    m_length += 8;
    m_tsft = value;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SetFrameFlags(uint8_t flags)
{
    NS_LOG_FUNCTION(this << +flags);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_FLAGS), "Flags radiotap field already present");
    m_present |= RADIOTAP_FLAGS;
    m_length += 1;
    m_flags = flags;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SetRate(uint8_t rate)
{
    NS_LOG_FUNCTION(this << +rate);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_RATE), "Rate radiotap field already present");
    m_present |= RADIOTAP_RATE;
    m_length += 1;
    m_rate = rate;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SetChannelFields(const ChannelFields& channelFields)
{
    NS_LOG_FUNCTION(this << channelFields.frequency << channelFields.flags);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_CHANNEL), "Channel radiotap field already present");
    m_channelPad = ((2 - m_length % 2) % 2);
    m_present |= RADIOTAP_CHANNEL;
    m_length += (sizeof(ChannelFields) + m_channelPad);
    m_channelFields = channelFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeChannel(Buffer::Iterator& start) const
{
    start.WriteU8(0, m_channelPad);
    start.WriteU16(m_channelFields.frequency);
    start.WriteU16(m_channelFields.flags);
}

uint32_t
RadiotapHeader::DeserializeChannel(Buffer::Iterator start, uint32_t bytesRead)
{
    m_channelPad = ((2 - bytesRead % 2) % 2);
    start.Next(m_channelPad);
    m_channelFields.frequency = start.ReadU16();
    m_channelFields.flags = start.ReadU16();
    return sizeof(ChannelFields) + m_channelPad;
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

    NS_ASSERT_MSG(!(m_present & RADIOTAP_DBM_ANTSIGNAL),
                  "Antenna signal radiotap field already present");
    m_present |= RADIOTAP_DBM_ANTSIGNAL;
    m_length += 1;

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

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SetAntennaNoisePower(double noise)
{
    NS_LOG_FUNCTION(this << noise);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_DBM_ANTNOISE),
                  "Antenna noise radiotap field already present");
    m_present |= RADIOTAP_DBM_ANTNOISE;
    m_length += 1;

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

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SetMcsFields(const McsFields& mcsFields)
{
    NS_LOG_FUNCTION(this << +mcsFields.known << +mcsFields.flags << +mcsFields.mcs);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_MCS), "MCS radiotap field already present");
    m_present |= RADIOTAP_MCS;
    m_length += sizeof(McsFields);
    m_mcsFields = mcsFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeMcs(Buffer::Iterator& start) const
{
    start.WriteU8(m_mcsFields.known);
    start.WriteU8(m_mcsFields.flags);
    start.WriteU8(m_mcsFields.mcs);
}

uint32_t
RadiotapHeader::DeserializeMcs(Buffer::Iterator start, uint32_t bytesRead)
{
    m_mcsFields.known = start.ReadU8();
    m_mcsFields.flags = start.ReadU8();
    m_mcsFields.mcs = start.ReadU8();
    return sizeof(McsFields);
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

    NS_ASSERT_MSG(!(m_present & RADIOTAP_AMPDU_STATUS),
                  "A-MPDU status radiotap field already present");
    m_ampduStatusPad = ((4 - m_length % 4) % 4);
    m_present |= RADIOTAP_AMPDU_STATUS;
    m_length += (sizeof(ampduStatusFields) + m_ampduStatusPad);
    m_ampduStatusFields = ampduStatusFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeAmpduStatus(Buffer::Iterator& start) const
{
    start.WriteU8(0, m_ampduStatusPad);
    start.WriteU32(m_ampduStatusFields.referenceNumber);
    start.WriteU16(m_ampduStatusFields.flags);
    start.WriteU8(m_ampduStatusFields.crc);
    start.WriteU8(m_ampduStatusFields.reserved);
}

uint32_t
RadiotapHeader::DeserializeAmpduStatus(Buffer::Iterator start, uint32_t bytesRead)
{
    m_ampduStatusPad = ((4 - bytesRead % 4) % 4);
    start.Next(m_ampduStatusPad);
    m_ampduStatusFields.referenceNumber = start.ReadU32();
    m_ampduStatusFields.flags = start.ReadU16();
    m_ampduStatusFields.crc = start.ReadU8();
    m_ampduStatusFields.reserved = start.ReadU8();
    return sizeof(AmpduStatusFields) + m_ampduStatusPad;
}

void
RadiotapHeader::PrintAmpduStatus(std::ostream& os) const
{
    os << " ampduStatus.flags=0x" << std::hex << m_ampduStatusFields.flags << std::dec;
}

void
RadiotapHeader::SetVhtFields(const VhtFields& vhtFields)
{
    NS_LOG_FUNCTION(this << vhtFields.known << vhtFields.flags << +vhtFields.mcsNss.at(0)
                         << +vhtFields.mcsNss.at(1) << +vhtFields.mcsNss.at(2)
                         << +vhtFields.mcsNss.at(3) << +vhtFields.coding << +vhtFields.groupId
                         << +vhtFields.partialAid);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_VHT), "VHT radiotap field already present");
    m_vhtPad = ((2 - m_length % 2) % 2);
    m_present |= RADIOTAP_VHT;
    m_length += (sizeof(VhtFields) + m_vhtPad);
    m_vhtFields = vhtFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeVht(Buffer::Iterator& start) const
{
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
    m_vhtPad = ((2 - bytesRead % 2) % 2);
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
    return sizeof(VhtFields) + m_vhtPad;
}

void
RadiotapHeader::PrintVht(std::ostream& os) const
{
    os << " vht.known=0x" << m_vhtFields.known << " vht.flags=0x" << m_vhtFields.flags
       << " vht.bandwidth=" << std::dec << m_vhtFields.bandwidth
       << " vht.mcsNss[0]=" << +m_vhtFields.mcsNss.at(0)
       << " vht.mcsNss[1]=" << +m_vhtFields.mcsNss.at(1)
       << " vht.mcsNss[2]=" << +m_vhtFields.mcsNss.at(2)
       << " vht.mcsNss[3]=" << +m_vhtFields.mcsNss.at(3) << " vht.coding=" << m_vhtFields.coding
       << " vht.groupId=" << m_vhtFields.groupId << " vht.partialAid=" << m_vhtFields.partialAid;
}

void
RadiotapHeader::SetHeFields(const HeFields& heFields)
{
    NS_LOG_FUNCTION(this << heFields.data1 << heFields.data2 << heFields.data3 << heFields.data4
                         << heFields.data5 << heFields.data6);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_HE), "HE radiotap field already present");
    m_hePad = ((2 - m_length % 2) % 2);
    m_present |= RADIOTAP_HE;
    m_length += (sizeof(heFields) + m_hePad);
    m_heFields = heFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeHe(Buffer::Iterator& start) const
{
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
    m_hePad = ((2 - bytesRead % 2) % 2);
    start.Next(m_hePad);
    m_heFields.data1 = start.ReadU16();
    m_heFields.data2 = start.ReadU16();
    m_heFields.data3 = start.ReadU16();
    m_heFields.data4 = start.ReadU16();
    m_heFields.data5 = start.ReadU16();
    m_heFields.data6 = start.ReadU16();
    return sizeof(HeFields) + m_hePad;
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

    NS_ASSERT_MSG(!(m_present & RADIOTAP_HE_MU), "HE-MU radiotap field already present");
    m_heMuPad = ((2 - m_length % 2) % 2);
    m_present |= RADIOTAP_HE_MU;
    m_length += (sizeof(heMuFields) + m_heMuPad);
    m_heMuFields = heMuFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeHeMu(Buffer::Iterator& start) const
{
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
    m_heMuPad = ((2 - bytesRead % 2) % 2);
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
    return sizeof(HeMuFields) + m_heMuPad;
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
                         << +heMuOtherUserFields.perUserPosition
                         << +heMuOtherUserFields.perUserKnown);

    NS_ASSERT_MSG(!(m_present & RADIOTAP_HE_MU_OTHER_USER),
                  "HE-MU-other-user radiotap field already present");
    m_heMuOtherUserPad = ((2 - m_length % 2) % 2);
    m_present |= RADIOTAP_HE_MU_OTHER_USER;
    m_length += (sizeof(HeMuOtherUserFields) + m_heMuOtherUserPad);
    m_heMuOtherUserFields = heMuOtherUserFields;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << std::dec);
}

void
RadiotapHeader::SerializeHeMuOtherUser(Buffer::Iterator& start) const
{
    start.WriteU8(0, m_heMuOtherUserPad);
    start.WriteU16(m_heMuOtherUserFields.perUser1);
    start.WriteU16(m_heMuOtherUserFields.perUser2);
    start.WriteU8(m_heMuOtherUserFields.perUserPosition);
    start.WriteU8(m_heMuOtherUserFields.perUserKnown);
}

uint32_t
RadiotapHeader::DeserializeHeMuOtherUser(Buffer::Iterator start, uint32_t bytesRead)
{
    m_heMuOtherUserPad = ((2 - bytesRead % 2) % 2);
    start.Next(m_heMuOtherUserPad);
    m_heMuOtherUserFields.perUser1 = start.ReadU16();
    m_heMuOtherUserFields.perUser2 = start.ReadU16();
    m_heMuOtherUserFields.perUserPosition = start.ReadU8();
    m_heMuOtherUserFields.perUserKnown = start.ReadU8();
    return sizeof(HeMuOtherUserFields) + m_heMuOtherUserPad;
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
    if (!m_presentExt)
    {
        m_present |= RADIOTAP_TLV | RADIOTAP_EXT;
        m_presentExt = 0;
        m_length += sizeof(RadiotapExtFlags);
    }

    NS_ASSERT_MSG(!(*m_presentExt & RADIOTAP_USIG), "U-SIG radiotap field already present");
    *m_presentExt |= RADIOTAP_USIG;

    m_usigTlvPad = ((8 - m_length % 8) % 8);
    m_usigTlv.type = 32 + std::countr_zero<uint16_t>(RADIOTAP_USIG);
    m_usigTlv.length = sizeof(UsigFields);
    m_length += sizeof(TlvFields) + m_usigTlvPad;

    m_usigPad = ((4 - m_length % 4) % 4);
    m_usigFields = usigFields;
    m_length += m_usigTlv.length + m_usigPad;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << " m_presentExt=0x" << *m_presentExt << std::dec);
}

void
RadiotapHeader::SerializeUsig(Buffer::Iterator& start) const
{
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
    const auto startBytesRead = bytesRead;
    m_usigTlvPad = ((8 - bytesRead % 8) % 8);
    start.Next(m_usigTlvPad);
    bytesRead += m_usigTlvPad;
    m_usigTlv.type = start.ReadU16();
    m_usigTlv.length = start.ReadU16();
    bytesRead += sizeof(TlvFields);
    m_usigPad = ((4 - bytesRead % 4) % 4);
    start.Next(m_usigPad);
    bytesRead += m_usigPad;
    m_usigFields.common = start.ReadU32();
    m_usigFields.value = start.ReadU32();
    m_usigFields.mask = start.ReadU32();
    bytesRead += sizeof(UsigFields);
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
    if (!m_presentExt)
    {
        m_present |= RADIOTAP_TLV | RADIOTAP_EXT;
        m_presentExt = 0;
        m_length += sizeof(RadiotapExtFlags);
    }

    NS_ASSERT_MSG(!(*m_presentExt & RADIOTAP_EHT_SIG), "EHT radiotap field already present");
    *m_presentExt |= RADIOTAP_EHT_SIG;

    m_ehtTlvPad = ((8 - m_length % 8) % 8);
    m_ehtTlv.type = 32 + std::countr_zero<uint16_t>(RADIOTAP_EHT_SIG);
    m_ehtTlv.length = (40 + ehtFields.userInfo.size() * 4);
    m_length += sizeof(TlvFields) + m_ehtTlvPad;

    m_ehtPad = ((4 - m_length % 4) % 4);
    m_ehtFields = ehtFields;
    m_length += m_ehtTlv.length + m_ehtPad;

    NS_LOG_LOGIC(this << " m_length=" << m_length << " m_present=0x" << std::hex << m_present
                      << " m_presentExt=0x" << *m_presentExt << std::dec);
}

void
RadiotapHeader::SerializeEht(Buffer::Iterator& start) const
{
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
    const auto startBytesRead = bytesRead;

    m_ehtTlvPad = ((8 - bytesRead % 8) % 8);
    start.Next(m_ehtTlvPad);
    bytesRead += m_ehtTlvPad;
    m_ehtTlv.type = start.ReadU16();
    m_ehtTlv.length = start.ReadU16();
    bytesRead += sizeof(TlvFields);

    m_ehtPad = ((4 - bytesRead % 4) % 4);
    start.Next(m_ehtPad);
    bytesRead += m_ehtPad;
    m_ehtFields.known = start.ReadU32();
    bytesRead += 4;
    for (auto& dataField : m_ehtFields.data)
    {
        dataField = start.ReadU32();
        bytesRead += 4;
    }
    const auto userInfosBytes = m_ehtTlv.length - bytesRead - m_ehtTlvPad;
    NS_ASSERT(userInfosBytes % 4 == 0);
    const std::size_t numUsers = userInfosBytes / 4;
    for (std::size_t i = 0; i < numUsers; ++i)
    {
        m_ehtFields.userInfo.push_back(start.ReadU32());
        bytesRead += 4;
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
