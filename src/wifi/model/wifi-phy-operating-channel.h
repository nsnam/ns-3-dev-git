/*
 * Copyright (c) 2021
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Stefano Avallone <stavallo@unina.it>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_PHY_OPERATING_CHANNEL_H
#define WIFI_PHY_OPERATING_CHANNEL_H

#include "wifi-phy-band.h"
#include "wifi-phy-common.h"

#include "ns3/he-ru.h"

#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace ns3
{

/**
 * A structure containing the information about a frequency channel
 */
struct FrequencyChannelInfo
{
    /**
     * @brief spaceship operator.
     *
     * @param info the frequency channel info
     * @returns -1 if the provided channel info is located at a lower channel number, 0 if the
     * provided channel info is identical or 1 if the provided channel info is located at a higher
     * channel number
     */
    auto operator<=>(const FrequencyChannelInfo& info) const = default;
    uint8_t number{0};                                        ///< the channel number
    MHz_u frequency{0};                                       ///< the center frequency
    MHz_u width{0};                                           ///< the channel width
    WifiPhyBand band{WifiPhyBand::WIFI_PHY_BAND_UNSPECIFIED}; ///< the PHY band
    FrequencyChannelType type{FrequencyChannelType::OFDM};    ///< the frequency channel type
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param info the frequency channel info
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const FrequencyChannelInfo& info);

/**
 * @ingroup wifi
 *
 * Class that keeps track of all information about the current PHY operating channel.
 */
class WifiPhyOperatingChannel
{
  public:
    /// Typedef for a const iterator pointing to a channel in the set of available channels
    using ConstIterator = std::set<FrequencyChannelInfo>::const_iterator;

    /// Comparison functor used to sort the segments by increasing frequencies
    struct Compare
    {
        /**
         * Functional operator for sorting the frequency segments.
         *
         * @param a const iterator pointing to the frequency channel for the first segment
         * @param b const iterator pointing to the frequency channel for the second segment
         * @return true if the center frequency of the first segment is lower than the center
         * frequency of the second segment
         */
        bool operator()(const ConstIterator& a, const ConstIterator& b) const;
    };

    /// Typedef for a set of const iterator pointing to the segments of a channel
    using ConstIteratorSet = std::set<ConstIterator, Compare>;

    /**
     * Create an uninitialized PHY operating channel.
     */
    WifiPhyOperatingChannel();

    /**
     * Create a PHY operating channel from an iterator pointing to a channel in the set of available
     * channels.
     *
     * @param it the iterator pointing to a channel in the set of available channels
     */
    WifiPhyOperatingChannel(ConstIterator it);

    /**
     * Create a PHY operating channel from iterators pointing to multiple frequency segments in the
     * set of available channels.
     *
     * @param its the iterators pointing to frequency segments in the set of available channels
     */
    WifiPhyOperatingChannel(const ConstIteratorSet& its);

    virtual ~WifiPhyOperatingChannel();

    /**
     * Check if the given WifiPhyOperatingChannel is equivalent.
     * Note that the primary20 channels are not compared.
     *
     * @param other another WifiPhyOperatingChannel
     *
     * @return true if the given WifiPhyOperatingChannel is equivalent,
     *         false otherwise
     */
    bool operator==(const WifiPhyOperatingChannel& other) const;

    /**
     * Check if the given WifiPhyOperatingChannel is different.
     *
     * @param other another WifiPhyOperatingChannel
     *
     * @return true if the given WifiPhyOperatingChannel is different,
     *         false otherwise
     */
    bool operator!=(const WifiPhyOperatingChannel& other) const;

    static const std::set<FrequencyChannelInfo>
        m_frequencyChannels; //!< Available frequency channels

    /**
     * Return true if a valid channel has been set, false otherwise.
     *
     * @return true if a valid channel has been set, false otherwise
     */
    bool IsSet() const;
    /**
     * Set the channel according to the specified parameters if a unique
     * frequency channel matches the specified criteria, or abort the
     * simulation otherwise.
     * If the channel width is a multiple of 20 MHz, the primary 20 MHz channel
     * is set to the 20 MHz subchannel with the lowest center frequency.
     *
     * @param segments the frequency segments
     * @param standard the standard
     */
    void Set(const std::vector<FrequencyChannelInfo>& segments, WifiStandard standard);
    /**
     * Set the default channel of the given width and for the given standard and band.
     * If the channel width is a multiple of 20 MHz, the primary 20 MHz channel
     * is set to the 20 MHz subchannel with the lowest center frequency.
     *
     * @param width the channel width
     * @param standard the standard
     * @param band the PHY band
     */
    void SetDefault(MHz_u width, WifiStandard standard, WifiPhyBand band);

    /**
     * Get the default channel number for a given segment of the given width and for the given
     * standard and band.
     *
     * @param width the channel width
     * @param standard the standard
     * @param band the PHY band
     * @param previousChannelNumber the channel number of the previous (in frequency) segment (if
     * non-contiguous operating channel is used). If there is no place for another segment that is
     * not contiguous to that previous one (at a higher frequency), an error is thrown
     * @return the default channel number
     */
    static uint8_t GetDefaultChannelNumber(
        MHz_u width,
        WifiStandard standard,
        WifiPhyBand band,
        std::optional<uint8_t> previousChannelNumber = std::nullopt);

    /**
     * Return the channel number for a given frequency segment.
     * Segments are ordered by increasing frequencies, hence by default
     * it returns the channel number of the segment occuping the lowest
     * frequencies when a non-contiguous operating channel is used.
     *
     * @param segment the index of the frequency segment (if operating channel is non-contiguous)
     * @return the channel number for a given frequency segment
     */
    uint8_t GetNumber(std::size_t segment = 0) const;
    /**
     * Return the center frequency for a given frequency segment.
     * Segments are ordered by increasing frequencies, hence by default
     * it returns the center frequency of the segment occuping the lowest
     * frequencies when a non-contiguous operating channel is used.
     *
     * @param segment the index of the frequency segment (if operating channel is non-contiguous)
     * @return the center frequency for a given frequency segment
     */
    MHz_u GetFrequency(std::size_t segment = 0) const;
    /**
     * Return the channel width for a given frequency segment.
     * Segments are ordered by increasing frequencies, hence by default
     * it returns the channel width of the segment occuping the lowest
     * frequencies when a non-contiguous operating channel is used.
     *
     * @param segment the index of the frequency segment (if operating channel is non-contiguous)
     * @return the channel width for a given frequency segment
     */
    MHz_u GetWidth(std::size_t segment = 0) const;
    /**
     * Return the width of the whole operating channel.
     *
     * @return the width of the whole operating channel
     */
    MHz_u GetTotalWidth() const;
    /**
     * Return the channel number per segment.
     * Segments are ordered by increasing frequencies.
     *
     * @return the channel number per segment
     */
    std::vector<uint8_t> GetNumbers() const;
    /**
     * Return the center frequency per segment.
     * Segments are ordered by increasing frequencies.
     *
     * @return the center frequency per segment
     */
    std::vector<MHz_u> GetFrequencies() const;
    /**
     * Return the channel width per segment.
     * Segments are ordered by increasing frequencies.
     *
     * @return the channel width per segment
     */
    std::vector<MHz_u> GetWidths() const;
    /**
     * Return the width type of the operating channel.
     *
     * @return the width type of the operating channel
     */
    WifiChannelWidthType GetWidthType() const;
    /**
     * Return the PHY band of the operating channel
     *
     * @return the PHY band of the operating channel
     */
    WifiPhyBand GetPhyBand() const;
    /**
     * Return whether the operating channel is an OFDM channel.
     *
     * @return whether the operating channel is an OFDM channel
     */
    bool IsOfdm() const;
    /**
     * Return whether the operating channel is a DSSS channel.
     *
     * @return whether the operating channel is a DSSS channel
     */
    bool IsDsss() const;
    /**
     * Return whether the operating channel is an 802.11p channel.
     *
     * @return whether the operating channel is an 802.11p channel
     */
    bool Is80211p() const;

    /**
     * If the operating channel width is a multiple of 20 MHz, return the index of the
     * primary channel of the given width within the operating channel (0 indicates
     * the 20 MHz subchannel with the lowest center frequency). Otherwise, return 0.
     *
     * @param primaryChannelWidth the width of the primary channel
     * @return the index of the requested primary channel within the operating channel
     */
    uint8_t GetPrimaryChannelIndex(MHz_u primaryChannelWidth) const;

    /**
     * If the operating channel width is made of a multiple of 20 MHz, return the index of the
     * secondary channel of the given width within the operating channel (0 indicates
     * the 20 MHz subchannel with the lowest center frequency). Otherwise, return 0.
     *
     * @param secondaryChannelWidth the width of the secondary channel
     * @return the index of the requested secondary channel within the operating channel
     */
    uint8_t GetSecondaryChannelIndex(MHz_u secondaryChannelWidth) const;

    /**
     * Set the index of the primary 20 MHz channel (0 indicates the 20 MHz subchannel
     * with the lowest center frequency among all segments).
     *
     * @param index the index of the primary 20 MHz channel
     */
    void SetPrimary20Index(uint8_t index);

    /**
     * Get the center frequency of the primary channel of the given width.
     *
     * @param primaryChannelWidth the width of the primary channel
     * @return the center frequency of the primary channel of the given width
     */
    MHz_u GetPrimaryChannelCenterFrequency(MHz_u primaryChannelWidth) const;

    /**
     * Get the center frequency of the secondary channel of the given width.
     *
     * @param secondaryChannelWidth the width of the secondary channel
     * @return the center frequency of the secondary channel of the given width
     */
    MHz_u GetSecondaryChannelCenterFrequency(MHz_u secondaryChannelWidth) const;

    /**
     * Get the channel indices of all the 20 MHz channels included in the primary
     * channel of the given width, if such primary channel exists, or an empty set,
     * otherwise.
     *
     * @param width the width of the primary channel
     * @return the channel indices of all the 20 MHz channels included in the primary
     *         channel of the given width, if such primary channel exists, or an empty set,
     *         otherwise
     */
    std::set<uint8_t> GetAll20MHzChannelIndicesInPrimary(MHz_u width) const;
    /**
     * Get the channel indices of all the 20 MHz channels included in the secondary
     * channel of the given width, if such secondary channel exists, or an empty set,
     * otherwise.
     *
     * @param width the width of the secondary channel
     * @return the channel indices of all the 20 MHz channels included in the secondary
     *         channel of the given width, if such secondary channel exists, or an empty set,
     *         otherwise
     */
    std::set<uint8_t> GetAll20MHzChannelIndicesInSecondary(MHz_u width) const;
    /**
     * Get the channel indices of all the 20 MHz channels included in the secondary
     * channel corresponding to the given primary channel, if such secondary channel
     * exists, or an empty set, otherwise.
     *
     * @param primaryIndices the channel indices of all the 20 MHz channels included
     *                       in the primary channel
     * @return the channel indices of all the 20 MHz channels included in the secondary
     *         channel corresponding to the given primary channel, if such secondary channel
     *         exists, or an empty set, otherwise
     */
    std::set<uint8_t> GetAll20MHzChannelIndicesInSecondary(
        const std::set<uint8_t>& primaryIndices) const;

    /**
     * Find the first frequency segment matching the specified parameters.
     *
     * @param number the channel number (use 0 to leave it unspecified)
     * @param frequency the channel center frequency in MHz (use 0 to leave it unspecified)
     * @param width the channel width (use 0 to leave it unspecified)
     * @param standard the standard (use WIFI_STANDARD_UNSPECIFIED not to check whether a
     *                 channel is suitable for a specific standard)
     * @param band the PHY band
     * @param start an iterator pointing to the channel to start the search with
     * @return an iterator pointing to the found channel, if any, or to past-the-end
     *         of the set of available channels
     */
    static ConstIterator FindFirst(uint8_t number,
                                   MHz_u frequency,
                                   MHz_u width,
                                   WifiStandard standard,
                                   WifiPhyBand band,
                                   ConstIterator start = m_frequencyChannels.begin());

    /**
     * Get channel number of the primary channel
     * @param primaryChannelWidth the width of the primary channel
     * @param standard the standard
     *
     * @return channel number of the primary channel
     */
    uint8_t GetPrimaryChannelNumber(MHz_u primaryChannelWidth, WifiStandard standard) const;

    /**
     * Get a WifiPhyOperatingChannel object corresponding to the primary channel of the given width.
     *
     * @param primaryChannelWidth the width of the primary channel in MHz
     * @return a WifiPhyOperatingChannel object corresponding to the primary channel of the given
     *         width
     */
    WifiPhyOperatingChannel GetPrimaryChannel(MHz_u primaryChannelWidth) const;

    /**
     * Get the channel indices of the minimum subset of 20 MHz channels containing the given RU.
     *
     * @param ru the given RU
     * @param width the width of the channel to which the given RU refers to; normally,
     *              it is the width of the PPDU for which the RU is allocated
     * @return the channel indices of the minimum subset of 20 MHz channels containing the given RU
     */
    std::set<uint8_t> Get20MHzIndicesCoveringRu(HeRu::RuSpec ru, MHz_u width) const;

    /**
     * Get the index of the segment that contains a given primary channel.
     *
     * @param primaryChannelWidth the width of the primary channel
     * @return the index of the segment that contains the primary channel
     */
    uint8_t GetPrimarySegmentIndex(MHz_u primaryChannelWidth) const;

    /**
     * Get the index of the segment that contains a given secondary channel.
     *
     * @param secondaryChannelWidth the width of the secondary channel
     * @return the index of the segment that contains the secondary channel
     */
    uint8_t GetSecondarySegmentIndex(MHz_u secondaryChannelWidth) const;

    /**
     * Get the number of frequency segments in the operating channel.
     * This is only more than one if a non-contiguous operating channel is used.
     *
     * @return the number of frequency segments
     */
    std::size_t GetNSegments() const;

  private:
    ConstIteratorSet m_channelIts; //!< const iterators pointing to the configured frequency channel
    uint8_t m_primary20Index;      /**< index of the primary20 channel (0 indicates the 20 MHz
                                        subchannel with the lowest center frequency) */
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param channel the operating channel
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const WifiPhyOperatingChannel& channel);

} // namespace ns3

#endif /* WIFI_PHY_OPERATING_CHANNEL_H */
