/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef VHT_OPERATION_H
#define VHT_OPERATION_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @brief The VHT Operation Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise
 * the VHT Operation Information Element
 */
class VhtOperation : public WifiInformationElement
{
  public:
    VhtOperation();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the Channel Width field in the VHT Operation information element.
     *
     * @param channelWidth the Channel Width field in the VHT Operation information element
     */
    void SetChannelWidth(uint8_t channelWidth);
    /**
     * Set the Channel Center Frequency Segment 0 field in the VHT Operation information element.
     *
     * @param channelCenterFrequencySegment0 the Channel Center Frequency Segment 0 field in the VHT
     * Operation information element
     */
    void SetChannelCenterFrequencySegment0(uint8_t channelCenterFrequencySegment0);
    /**
     * Set the Channel Center Frequency Segment 1 field in the VHT Operation information element.
     *
     * @param channelCenterFrequencySegment1 the Channel Center Frequency Segment 1 field in the VHT
     * Operation information element
     */
    void SetChannelCenterFrequencySegment1(uint8_t channelCenterFrequencySegment1);
    /**
     * Set the Basic VHT-MCS and NSS field in the VHT Operation information element.
     *
     * @param basicVhtMcsAndNssSet the Basic VHT-MCS and NSS field in the VHT Operation information
     * element
     */
    void SetBasicVhtMcsAndNssSet(uint16_t basicVhtMcsAndNssSet);
    /**
     * Set the Basic VHT-MCS and NSS field in the VHT Operation information element
     * by specifying the tuple (nss, maxMcs).
     *
     * @param nss the NSS
     * @param maxVhtMcs the maximum supported VHT-MCS value corresponding to that NSS
     */
    void SetMaxVhtMcsPerNss(uint8_t nss, uint8_t maxVhtMcs);

    /**
     * Return the Channel Width field in the VHT Operation information element.
     *
     * @return the Channel Width field in the VHT Operation information element
     */
    uint8_t GetChannelWidth() const;
    /**
     * Return the Channel Center Frequency Segment 0 field in the VHT Operation information element.
     *
     * @return the Channel Center Frequency Segment 0 field in the VHT Operation information element
     */
    uint8_t GetChannelCenterFrequencySegment0() const;
    /**
     * Return the Channel Center Frequency Segment 1 field in the VHT Operation information element.
     *
     * @return the Channel Center Frequency Segment 1 field in the VHT Operation information element
     */
    uint8_t GetChannelCenterFrequencySegment1() const;
    /**
     * Return the Basic VHT-MCS And Nss field in the VHT Operation information element.
     *
     * @return the Basic VHT-MCS And Nss field in the VHT Operation information element
     */
    uint16_t GetBasicVhtMcsAndNssSet() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    // VHT Operation Information
    uint8_t m_channelWidth;                   ///< channel width
    uint8_t m_channelCenterFrequencySegment0; ///< channel center frequency segment 0
    uint8_t m_channelCenterFrequencySegment1; ///< channel center frequency segment 1

    // Basic VHT-MCS and NSS Set
    uint16_t m_basicVhtMcsAndNssSet; ///< basic VHT MCS NSS set
};

} // namespace ns3

#endif /* VHT_OPERATION_H */
