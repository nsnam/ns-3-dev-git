/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.h by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#ifndef SPECTRUM_WIFI_PHY_H
#define SPECTRUM_WIFI_PHY_H

#include "ns3/antenna-model.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-model.h"
#include "wifi-phy.h"

namespace ns3 {

class WifiSpectrumPhyInterface;
class WifiPpdu;

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a spectrum-aware enhancement of the 802.11 SpectrumWifiPhy
 * model.
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::SpectrumPropagationLossModel
 * and ns3::PropagationDelayModel classes.
 *
 */
class SpectrumWifiPhy : public WifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SpectrumWifiPhy ();
  virtual ~SpectrumWifiPhy ();

  // Implementation of pure virtual method.
  void StartTx (Ptr<WifiPpdu> ppdu);
  Ptr<Channel> GetChannel (void) const;

  /**
   * Set the SpectrumChannel this SpectrumWifiPhy is to be connected to.
   *
   * \param channel the SpectrumChannel this SpectrumWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<SpectrumChannel> channel);

  /**
   * Input method for delivering a signal from the spectrum channel
   * and low-level PHY interface to this SpectrumWifiPhy instance.
   *
   * \param rxParams Input signal parameters
   */
  void StartRx (Ptr<SpectrumSignalParameters> rxParams);

  /**
   * Get the center frequency of the channel corresponding the current TxVector rather than
   * that of the supported channel width.
   * Consider that this "primary channel" is on the lower part for the time being.
   *
   * \param txVector the TXVECTOR that has the channel width that is to be used
   * \return the center frequency in MHz corresponding to the channel width to be used
   */
  uint16_t GetCenterFrequencyForChannelWidth (WifiTxVector txVector) const;

  /**
   * Method to encapsulate the creation of the WifiSpectrumPhyInterface
   * object (used to bind the WifiSpectrumPhy to a SpectrumChannel) and
   * to link it to this SpectrumWifiPhy instance
   *
   * \param device pointer to the NetDevice object including this new object
   */
  void CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device);
  /**
   * \param antenna an AntennaModel to include in the transmitted
   *                SpectrumSignalParameters (in case any objects downstream of the
   *                SpectrumWifiPhy wish to adjust signal properties based on the
   *                transmitted antenna model.  This antenna is also used when
   *                the underlying WifiSpectrumPhyInterface::GetRxAntenna() method
   *                is called.
   *
   * Note:  this method may be split into separate SetTx and SetRx
   * methods in the future if the modeling need for this arises
   */
  void SetAntenna (const Ptr<AntennaModel> antenna);
  /**
   * Get the antenna model used for reception
   *
   * \return the AntennaModel used for reception
   */
  Ptr<AntennaModel> GetRxAntenna (void) const;
  /**
   * \return the SpectrumModel that this SpectrumPhy expects to be used
   *         for all SpectrumValues that are passed to StartRx. If 0 is
   *         returned, it means that any model will be accepted.
   */
  Ptr<const SpectrumModel> GetRxSpectrumModel () const;

  /**
   * \return the width of each band (Hz)
   */
  uint32_t GetBandBandwidth (void) const;

  /**
   * \param currentChannelWidth channel width of the current transmission (MHz)
   * \return the width of the guard band (MHz)
   *
   * Note: in order to properly model out of band transmissions for OFDM, the guard
   * band has been configured so as to expand the modeled spectrum up to the
   * outermost referenced point in "Transmit spectrum mask" sections' PSDs of
   * each PHY specification of 802.11-2016 standard. It thus ultimately corresponds
   * to the current channel bandwidth (which can be different from devices max
   * channel width).
   */
  uint16_t GetGuardBandwidth (uint16_t currentChannelWidth) const;

  /**
   * Callback invoked when the PHY model starts to process a signal
   *
   * \param signalType Whether signal is WiFi (true) or foreign (false)
   * \param senderNodeId Node Id of the sender of the signal
   * \param rxPower received signal power (dBm)
   * \param duration Signal duration
   */
  typedef void (* SignalArrivalCallback) (bool signalType, uint32_t senderNodeId, double rxPower, Time duration);

  // The following four methods call to the base WifiPhy class method
  // but also generate a new SpectrumModel if called during runtime
  virtual void SetChannelNumber (uint8_t id);
  virtual void SetFrequency (uint16_t freq);
  virtual void SetChannelWidth (uint16_t channelwidth);
  virtual void ConfigureStandardAndBand (WifiPhyStandard standard, WifiPhyBand band);


protected:
  // Inherited
  void DoDispose (void);
  void DoInitialize (void);


private:
  /**
   * \param centerFrequency center frequency (MHz)
   * \param channelWidth channel width (MHz) of the channel for the current transmission
   * \param txPowerW power in W to spread across the bands
   * \param modulationClass the modulation class
   * \return Pointer to SpectrumValue
   *
   * This is a helper function to create the right TX PSD corresponding
   * to the standard in use.
   */
  Ptr<SpectrumValue> GetTxPowerSpectralDensity (uint16_t centerFrequency, uint16_t channelWidth, double txPowerW, WifiModulationClass modulationClass) const;

  /**
   * Perform run-time spectrum model change
   */
  void ResetSpectrumModel (void);

  Ptr<SpectrumChannel> m_channel;        //!< SpectrumChannel that this SpectrumWifiPhy is connected to

  Ptr<WifiSpectrumPhyInterface> m_wifiSpectrumPhyInterface; //!< Spectrum PHY interface
  Ptr<AntennaModel> m_antenna;                              //!< antenna model
  mutable Ptr<const SpectrumModel> m_rxSpectrumModel;       //!< receive spectrum model
  bool m_disableWifiReception;                              //!< forces this PHY to fail to sync on any signal
  TracedCallback<bool, uint32_t, double, Time> m_signalCb;  //!< Signal callback

};

} //namespace ns3

#endif /* SPECTRUM_WIFI_PHY_H */
