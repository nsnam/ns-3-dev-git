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
 *          Giuseppe Piro <g.piro@poliba.it>
 */

#ifndef ISM_SPECTRUM_VALUE_HELPER_H
#define ISM_SPECTRUM_VALUE_HELPER_H

#include <ns3/spectrum-value.h>

namespace ns3
{

/**
 * \ingroup spectrum
 *
 * Implements Wifi SpectrumValue for the 2.4 GHz ISM band only, with a
 * 5 MHz spectrum resolution.
 *
 */
class SpectrumValue5MhzFactory
{
  public:
    /**
     * Destructor
     */
    virtual ~SpectrumValue5MhzFactory() = default;
    /**
     * Creates a SpectrumValue instance with a constant value for all frequencies
     *
     * @param psd the constant value
     *
     * @return a Ptr to a newly created SpectrumValue
     */
    virtual Ptr<SpectrumValue> CreateConstant(double psd);
    /**
     * Creates a SpectrumValue instance that represents the TX Power Spectral
     * Density  of a wifi device corresponding to the provided parameters
     *
     * Since the spectrum model has a resolution of 5 MHz, we model
     * the transmitted signal with a constant density over a 20MHz
     * bandwidth centered on the center frequency of the channel. The
     * transmission power outside the transmission power density is
     * calculated considering the transmit spectrum mask, see IEEE
     * Std. 802.11-2007, Annex I.  The two bands just outside of the main
     * 20 MHz are allocated power at -28 dB down from the center 20 MHz,
     * and the two bands outside of this are allocated power at -40 dB down
     * (with a total bandwidth of 60 MHz containing non-zero power allocation).
     *
     * @param txPower the total TX power in W
     * @param channel the number of the channel (1 <= channel <= 13)
     *
     * @return a Ptr to a newly created SpectrumValue
     */
    virtual Ptr<SpectrumValue> CreateTxPowerSpectralDensity(double txPower, uint8_t channel);
};

} // namespace ns3

#endif /*  ISM_SPECTRUM_VALUE_HELPER_H */
