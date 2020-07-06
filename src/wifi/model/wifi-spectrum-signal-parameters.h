/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by Marco Miozzo <mmiozzo@cttc.es> (add data and ctrl diversity)
 */

#ifndef WIFI_SPECTRUM_SIGNAL_PARAMETERS_H
#define WIFI_SPECTRUM_SIGNAL_PARAMETERS_H

#include "ns3/spectrum-signal-parameters.h"

namespace ns3 {

class WifiPpdu;

/**
 * \ingroup wifi
 *
 * The transmit power spectral density flag, namely used
 * to correctly build PSD for HE TB PPDU non-OFDMA and
 * OFDMA portions.
 */
enum TxPsdFlag
{
  PSD_NON_HE_TB = 0,           //!< non-HE TB PPDU transmissions
  PSD_HE_TB_NON_OFDMA_PORTION, //!< preamble of HE TB PPDU, which should only be sent on minimum subset of 20 MHz channels containing RU
  PSD_HE_TB_OFDMA_PORTION      //!< OFDMA portion of HE TB PPDU, which should only be sent on RU
};

/**
* \brief Stream insertion operator.
*
* \param os the stream
* \param flag the transmit power spectral density flag
* \returns a reference to the stream
*/
inline std::ostream& operator<< (std::ostream& os, TxPsdFlag flag)
{
  switch (flag)
    {
    case PSD_NON_HE_TB:
      return (os << "PSD_NON_HE_TB");
    case PSD_HE_TB_NON_OFDMA_PORTION:
      return (os << "PSD_HE_TB_NON_OFDMA_PORTION");
    case PSD_HE_TB_OFDMA_PORTION:
      return (os << "PSD_HE_TB_OFDMA_PORTION");
    default:
      NS_FATAL_ERROR ("Invalid PSD flag");
      return (os << "INVALID");
    }
}


/**
 * \ingroup wifi
 *
 * Signal parameters for wifi
 */
struct WifiSpectrumSignalParameters : public SpectrumSignalParameters
{

  // inherited from SpectrumSignalParameters
  virtual Ptr<SpectrumSignalParameters> Copy ();

  /**
   * default constructor
   */
  WifiSpectrumSignalParameters ();

  /**
   * copy constructor
   *
   * \param p the wifi spectrum signal parameters
   */
  WifiSpectrumSignalParameters (const WifiSpectrumSignalParameters& p);

  Ptr<WifiPpdu> ppdu;                  ///< The PPDU being transmitted
  TxPsdFlag txPsdFlag {PSD_NON_HE_TB}; ///< The transmit power spectral density flag
};

}  // namespace ns3

#endif /* WIFI_SPECTRUM_SIGNAL_PARAMETERS_H */
