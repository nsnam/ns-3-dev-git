/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by Marco Miozzo <mmiozzo@cttc.es> (add data and ctrl diversity)
 */

#ifndef WIFI_SPECTRUM_SIGNAL_PARAMETERS_H
#define WIFI_SPECTRUM_SIGNAL_PARAMETERS_H

#include "wifi-phy-common.h"

#include "ns3/spectrum-signal-parameters.h"

namespace ns3
{

class WifiPpdu;

/**
 * @ingroup wifi
 *
 * Signal parameters for wifi
 */
struct WifiSpectrumSignalParameters : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    WifiSpectrumSignalParameters();

    /**
     * copy constructor
     *
     * @param p the wifi spectrum signal parameters
     */
    WifiSpectrumSignalParameters(const WifiSpectrumSignalParameters& p);

    Ptr<const WifiPpdu> ppdu; ///< The PPDU being transmitted
};

} // namespace ns3

#endif /* WIFI_SPECTRUM_SIGNAL_PARAMETERS_H */
