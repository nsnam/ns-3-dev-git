/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef FREQS_ISM2400MHZ_RES1MHZ_H
#define FREQS_ISM2400MHZ_RES1MHZ_H

#include "spectrum-value.h"

namespace ns3
{

/**
 * @brief Spectrum model logger for frequencies in the 2.4 GHz ISM band
 * with 1 MHz resolution.
 * @return Spectrum model for 2.4 GHz ISM band
 */
Ptr<SpectrumModel> SpectrumModelIsm2400MhzRes1Mhz();

} // namespace ns3

#endif /* FREQS_ISM2400MHZ_RES1MHZ_H */
