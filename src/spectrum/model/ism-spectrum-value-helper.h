/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2017 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es>
 *          Giuseppe Piro <g.piro@poliba.it>
 */

#ifndef ISM_SPECTRUM_VALUE_HELPER_H
#define ISM_SPECTRUM_VALUE_HELPER_H

#include "spectrum-value.h"

namespace ns3
{

/**
 * @ingroup spectrum
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
