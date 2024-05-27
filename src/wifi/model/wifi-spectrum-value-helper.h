/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2017 Orange Labs
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
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Rediet <getachew.redieteab@orange.com>
 */

#ifndef WIFI_SPECTRUM_VALUE_HELPER_H
#define WIFI_SPECTRUM_VALUE_HELPER_H

#include "wifi-types.h"

#include <ns3/spectrum-value.h>

#include <vector>

namespace ns3
{

/**
 * typedef for a pair of start and stop sub-band indices
 */
using WifiSpectrumBandIndices = std::pair<uint32_t, uint32_t>;

/**
 * \ingroup spectrum
 *
 *  This class defines all functions to create a spectrum model for
 *  Wi-Fi based on a a spectral model aligned with an OFDM subcarrier
 *  spacing of 312.5 KHz (model also reused for DSSS modulations)
 */
class WifiSpectrumValueHelper
{
  public:
    /**
     * Destructor
     */
    virtual ~WifiSpectrumValueHelper() = default;

    /**
     * Return a SpectrumModel instance corresponding to the center frequency
     * and channel width.  The spectrum model spans the channel width
     * +/- the guard bands (i.e. the model will span (channelWidth +
     * 2 * guardBandwidth) MHz of bandwidth).
     *
     * \param centerFrequencies center frequency (MHz) per segment
     * \param channelWidth total allocated channel width (MHz) over all segments
     * \param carrierSpacing carrier spacing (Hz)
     * \param guardBandwidth total width of the guard band (MHz), which will be split over the
     * segments
     *
     * \return the static SpectrumModel instance corresponding to the
     * given carrier frequency and channel width configuration.
     */
    static Ptr<SpectrumModel> GetSpectrumModel(const std::vector<uint16_t>& centerFrequencies,
                                               ChannelWidthMhz channelWidth,
                                               uint32_t carrierSpacing,
                                               ChannelWidthMhz guardBandwidth);

    /**
     * Create a transmit power spectral density corresponding to DSSS
     *
     * The center frequency typically corresponds to 802.11b channel
     * center frequencies but is not restricted to those frequencies.
     *
     * \note There is no channel width parameter; this method assumes 22 MHz
     *
     * \param centerFrequency center frequency (MHz)
     * \param txPowerW  transmit power (W) to allocate
     * \param guardBandwidth width of the guard band
     * \returns a pointer to a newly allocated SpectrumValue representing the DSSS Transmit Power
     * Spectral Density in W/Hz
     */
    static Ptr<SpectrumValue> CreateDsssTxPowerSpectralDensity(uint16_t centerFrequency,
                                                               double txPowerW,
                                                               ChannelWidthMhz guardBandwidth);

    /**
     * Create a transmit power spectral density corresponding to OFDM
     * (802.11a/g).  Channel width may vary between 20, 10, and 5 MHz.
     * Guard bandwidth also typically varies with channel width.
     *
     * \param centerFrequency center frequency (MHz)
     * \param channelWidth channel width
     * \param txPowerW  transmit power (W) to allocate
     * \param guardBandwidth width of the guard band
     * \param minInnerBandDbr the minimum relative power in the inner band (in dBr)
     * \param minOuterbandDbr the minimum relative power in the outer band (in dBr)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \return a pointer to a newly allocated SpectrumValue representing the OFDM Transmit Power
     * Spectral Density in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateOfdmTxPowerSpectralDensity(uint16_t centerFrequency,
                                                               ChannelWidthMhz channelWidth,
                                                               double txPowerW,
                                                               ChannelWidthMhz guardBandwidth,
                                                               double minInnerBandDbr = -20,
                                                               double minOuterbandDbr = -28,
                                                               double lowestPointDbr = -40);

    /**
     * Create a transmit power spectral density corresponding to OFDM duplicated over multiple 20
     * MHz subchannels. Channel width may vary between 20, 40, 80, and 160 MHz.
     *
     * \param centerFrequencies center frequency (MHz) per segment
     * \param channelWidth total allocated channel width (MHz) over all segments
     * \param txPowerW  transmit power (W) to allocate
     * \param guardBandwidth width of the guard band
     * \param minInnerBandDbr the minimum relative power in the inner band (in dBr)
     * \param minOuterbandDbr the minimum relative power in the outer band (in dBr)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \param puncturedSubchannels bitmap indicating whether a 20 MHz subchannel is punctured or not
     * \return a pointer to a newly allocated SpectrumValue representing the duplicated 20 MHz OFDM
     * Transmit Power Spectral Density in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateDuplicated20MhzTxPowerSpectralDensity(
        const std::vector<uint16_t>& centerFrequencies,
        ChannelWidthMhz channelWidth,
        double txPowerW,
        ChannelWidthMhz guardBandwidth,
        double minInnerBandDbr = -20,
        double minOuterbandDbr = -28,
        double lowestPointDbr = -40,
        const std::vector<bool>& puncturedSubchannels = {});

    /**
     * Create a transmit power spectral density corresponding to OFDM
     * High Throughput (HT) (802.11n/ac).  Channel width may vary between
     * 20, 40, 80, and 160 MHz.
     *
     * \param centerFrequencies center frequency (MHz) per segment
     * \param channelWidth total allocated channel width (MHz) over all segments
     * \param txPowerW  transmit power (W) to allocate
     * \param guardBandwidth width of the guard band
     * \param minInnerBandDbr the minimum relative power in the inner band (in dBr)
     * \param minOuterbandDbr the minimum relative power in the outer band (in dBr)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \return a pointer to a newly allocated SpectrumValue representing the HT OFDM Transmit Power
     * Spectral Density in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateHtOfdmTxPowerSpectralDensity(
        const std::vector<uint16_t>& centerFrequencies,
        ChannelWidthMhz channelWidth,
        double txPowerW,
        ChannelWidthMhz guardBandwidth,
        double minInnerBandDbr = -20,
        double minOuterbandDbr = -28,
        double lowestPointDbr = -40);

    /**
     * Create a transmit power spectral density corresponding to OFDM
     * High Efficiency (HE) (802.11ax) for contiguous channels.
     * Channel width may vary between 20, 40, 80, and 160 MHz.
     *
     * \param centerFrequency center frequency (MHz)
     * \param channelWidth channel width (MHz)
     * \param txPowerW transmit power (W) to allocate
     * \param guardBandwidth width of the guard band (MHz)
     * \param minInnerBandDbr the minimum relative power in the inner band (in dBr)
     * \param minOuterbandDbr the minimum relative power in the outer band (in dBr)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \param puncturedSubchannels bitmap indicating whether a 20 MHz subchannel is punctured or not
     * \return a pointer to a newly allocated SpectrumValue representing the HE OFDM Transmit Power
     * Spectral Density in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateHeOfdmTxPowerSpectralDensity(
        uint16_t centerFrequency,
        ChannelWidthMhz channelWidth,
        double txPowerW,
        ChannelWidthMhz guardBandwidth,
        double minInnerBandDbr = -20,
        double minOuterbandDbr = -28,
        double lowestPointDbr = -40,
        const std::vector<bool>& puncturedSubchannels = {});

    /**
     * Create a transmit power spectral density corresponding to OFDM
     * High Efficiency (HE) (802.11ax) made of one or more frequency segment(s).
     * Channel width may vary between 20, 40, 80, and 160 MHz.
     *
     * \param centerFrequencies center frequency (MHz) per segment
     * \param channelWidth total allocated channel width (MHz) over all segments
     * \param txPowerW transmit power (W) to allocate
     * \param guardBandwidth width of the guard band (MHz)
     * \param minInnerBandDbr the minimum relative power in the inner band (in dBr)
     * \param minOuterbandDbr the minimum relative power in the outer band (in dBr)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \param puncturedSubchannels bitmap indicating whether a 20 MHz subchannel is punctured or not
     * \return a pointer to a newly allocated SpectrumValue representing the HE OFDM Transmit Power
     * Spectral Density in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateHeOfdmTxPowerSpectralDensity(
        const std::vector<uint16_t>& centerFrequencies,
        uint16_t channelWidth,
        double txPowerW,
        uint16_t guardBandwidth,
        double minInnerBandDbr = -20,
        double minOuterbandDbr = -28,
        double lowestPointDbr = -40,
        const std::vector<bool>& puncturedSubchannels = {});

    /**
     * Create a transmit power spectral density corresponding to the OFDMA part
     * of HE TB PPDUs for a given RU.
     * An ideal (i.e. rectangular) spectral mask is considered for the time being.
     *
     * \param centerFrequencies center frequency (MHz) per segment
     * \param channelWidth total allocated channel width (MHz) over all segments
     * \param txPowerW  transmit power (W) to allocate
     * \param guardBandwidth width of the guard band (MHz)
     * \param ru the RU band used by the STA
     * \return a pointer to a newly allocated SpectrumValue representing the HE OFDM Transmit Power
     * Spectral Density on the RU used by the STA in W/Hz for each Band
     */
    static Ptr<SpectrumValue> CreateHeMuOfdmTxPowerSpectralDensity(
        const std::vector<uint16_t>& centerFrequencies,
        ChannelWidthMhz channelWidth,
        double txPowerW,
        ChannelWidthMhz guardBandwidth,
        const std::vector<WifiSpectrumBandIndices>& ru);

    /**
     * Create a transmit power spectral density corresponding to OFDM
     * transmit spectrum mask requirements for 11a/11g/11n/11ac/11ax
     * Channel width may vary between 5, 10, 20, 40, 80, and 160 MHz.
     * The default (standard) values are illustrated below.
     *
     *   [ guard band  ][    channel width     ][  guard band ]
     *                   __________   __________                  _ 0 dBr
     *                  /          | |          \
     *                 /           |_|           \                _ -20 dBr
     *             . '                             ' .
     *         . '                                     ' .        _ -28 dBr
     *       .'                                           '.
     *     .'                                               '.
     *   .'                                                   '.  _ lowest point
     *
     *   |-----|                                         |-----|  outerBand left/right
     *         |------|                           |-- ---|        middle band left/right
     *                |-|                       |-|               inner band left/right
     *                  |-----------------------|                 allocated sub-bands
     *   |-----------------------------------------------------|  mask band
     *
     * Please take note that, since guard tones are within the allocated band
     * while not being ideally allocated any power, the inner band had to be
     * shifted inwards and a flat junction band (at -20 dBr) had to be added
     * between the inner and the middle bands.
     *
     * \param c spectrumValue to allocate according to transmit power spectral density mask (in W/Hz
     * for each band)
     * \param allocatedSubBandsPerSegment vector of start and stop subcarrier indexes of the
     * allocated sub bands, for each segment \param maskBand start and stop subcarrier indexes of
     * transmit mask (in case signal doesn't cover whole SpectrumModel) \param txPowerPerBandW power
     * allocated to each subcarrier in the allocated sub bands \param nGuardBands size (in number of
     * subcarriers) of the guard band (left and right) \param innerSlopeWidth size (in number of
     * subcarriers) of the inner band (i.e. slope going from 0 dBr to -20 dBr in the figure above)
     * \param minInnerBandDbr the minimum relative power in the inner band (i.e., -20 dBr in the
     * figure above)
     * \param minOuterbandDbr the minimum relative power in the outer band (i.e., -28 dBr in the
     * figure above)
     * \param lowestPointDbr maximum relative power of the outermost subcarriers of the guard band
     * (in dBr)
     * \param puncturedSubBands vector of start and stop subcarrier indexes of the punctured sub
     * bands, for each segment
     * \param puncturedSlopeWidth size (in number of subcarriers) of the punctured band slope
     */
    static void CreateSpectrumMaskForOfdm(
        Ptr<SpectrumValue> c,
        const std::vector<std::vector<WifiSpectrumBandIndices>>& allocatedSubBandsPerSegment,
        const WifiSpectrumBandIndices& maskBand,
        double txPowerPerBandW,
        uint32_t nGuardBands,
        uint32_t innerSlopeWidth,
        double minInnerBandDbr,
        double minOuterbandDbr,
        double lowestPointDbr,
        const std::vector<std::vector<WifiSpectrumBandIndices>>& puncturedSubBands = {},
        uint32_t puncturedSlopeWidth = 0);

    /**
     * Normalize the transmit spectrum mask generated by CreateSpectrumMaskForOfdm
     * so that the total transmitted power corresponds to the input value.
     *
     * \param c spectrumValue to normalize (in W/Hz for each band)
     * \param txPowerW total transmit power (W) to allocate
     */
    static void NormalizeSpectrumMask(Ptr<SpectrumValue> c, double txPowerW);

    /**
     * Calculate the power of the specified band composed of uniformly-sized sub-bands.
     *
     * \param psd received Power Spectral Density in W/Hz
     * \param segments a vector of pair of start and stop indexes that defines each segment of the
     * band
     *
     * \return band power in W
     */
    static double GetBandPowerW(Ptr<SpectrumValue> psd,
                                const std::vector<WifiSpectrumBandIndices>& segments);
};

/**
 * \ingroup spectrum
 * Struct defining a frequency range between minFrequency (MHz) and maxFrequency (MHz).
 */
struct FrequencyRange
{
    uint16_t minFrequency{0}; ///< the minimum frequency in MHz
    uint16_t maxFrequency{0}; ///< the maximum frequency in MHz
};

/**
 * Compare two FrequencyRange values
 *
 * \param lhs the FrequencyRange value on the left of operator
 * \param rhs the FrequencyRange value on the right of operator
 *
 * \return true if minFrequency of left is less than minFrequency of right, false otherwise
 */
bool operator<(const FrequencyRange& lhs, const FrequencyRange& rhs);

/**
 * Compare two FrequencyRange values
 *
 * \param lhs the FrequencyRange value on the left of operator
 * \param rhs the FrequencyRange value on the right of operator
 *
 * \return true if both minFrequency and maxFrequency of left are equal to minFrequency and
 * maxFrequency of right respectively, false otherwise
 */
bool operator==(const FrequencyRange& lhs, const FrequencyRange& rhs);

/**
 * Compare two FrequencyRange values
 *
 * \param lhs the FrequencyRange value on the left of operator
 * \param rhs the FrequencyRange value on the right of operator
 *
 * \return true if either minFrequency or maxFrequency of left different from minFrequency or
 * maxFrequency of right respectively, false otherwise
 */
bool operator!=(const FrequencyRange& lhs, const FrequencyRange& rhs);

/**
 * Serialize FrequencyRange values to ostream (human-readable).
 *
 * \param os the output stream
 * \param freqRange the FrequencyRange
 *
 * \return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const FrequencyRange& freqRange);

/// Identifier for the frequency range covering the whole wifi spectrum
constexpr FrequencyRange WHOLE_WIFI_SPECTRUM = {2401, 7125};

/// Identifier for the frequency range covering the wifi spectrum in the 2.4 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_2_4_GHZ = {2401, 2483};

/// Identifier for the frequency range covering the wifi spectrum in the 5 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_5_GHZ = {5170, 5915};

/// Identifier for the frequency range covering the wifi spectrum in the 6 GHz band
constexpr FrequencyRange WIFI_SPECTRUM_6_GHZ = {5945, 7125};

} // namespace ns3

#endif /*  WIFI_SPECTRUM_VALUE_HELPER_H */
