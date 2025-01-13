/*
 * Copyright (c) 2014 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Benjamin Cizdziel <ben.cizdziel@gmail.com>
 */

#ifndef TV_SPECTRUM_TRANSMITTER_H
#define TV_SPECTRUM_TRANSMITTER_H

#include "spectrum-channel.h"
#include "spectrum-phy.h"
#include "spectrum-signal-parameters.h"
#include "spectrum-value.h"

#include "ns3/antenna-model.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *
 * SpectrumPhy implementation that creates a customizable TV transmitter which
 * transmits a PSD spectrum specified by user-set attributes.
 *
 *
 * This PHY model supports a single antenna model instance which is
 * used for transmission (this PHY model never receives).
 */
class TvSpectrumTransmitter : public SpectrumPhy
{
  public:
    /**
     * types of TV transmitters: analog, digital 8-VSB, or digital COFDM
     */
    enum TvType
    {
        TVTYPE_ANALOG,
        TVTYPE_8VSB,
        TVTYPE_COFDM
    };

    TvSpectrumTransmitter();
    ~TvSpectrumTransmitter() override;

    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited from SpectrumPhy
    void SetChannel(Ptr<SpectrumChannel> c) override;
    void SetMobility(Ptr<MobilityModel> m) override;
    void SetDevice(Ptr<NetDevice> d) override;
    Ptr<MobilityModel> GetMobility() const override;
    Ptr<NetDevice> GetDevice() const override;
    // device does not use Rx but these pure virtual methods must be implemented
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override;
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Get the spectrum channel
     *
     * @return a pointer to the assigned spectrum channel
     */
    Ptr<SpectrumChannel> GetChannel() const;

    /**
     * Creates power spectral density (PSD) spectrum of the TV transmitter and
     * sets it for transmission.
     * Before calling this method, must set attributes and parameters that are
     * applicable to your transmitter.
     *
     * 8-VSB PSD approximated from Figure 3 of the following article:
     * Baron, Stanley. "First-Hand:Digital Television: The Digital Terrestrial
     * Television Broadcasting (DTTB) Standard." IEEE Global History Network.
     * <http://www.ieeeghn.org/wiki/index.php/First-Hand:Digital_Television:_The_
     * Digital_Terrestrial_Television_Broadcasting_(DTTB)_Standard>.
     *
     * COFDM PSD approximated from Figure 12 (8k mode) of the following article:
     * Kopp, Carlo. "High Definition Television." High Definition Television. Air
     * Power Australia. <http://www.ausairpower.net/AC-1100.html>.
     *
     * Analog PSD approximated from Figure 4 of the following paper:
     * Stephen Shellhammer, Ahmed Sadek, and Wenyi Zhang. "Technical Challenges
     * for Cognitive Radio in the TV White Space Spectrum." Qualcomm Incorporated.
     */
    virtual void CreateTvPsd();

    /**
     * Get the power spectral density of the TV transmitter's signal
     *
     * @return a pointer to the PSD
     */
    Ptr<SpectrumValue> GetTxPsd() const;

    /**
     * Starts the TV Transmitter's transmission on the spectrum channel
     */
    virtual void Start();

    /**
     * Stops the TV Transmitter's transmission on the spectrum channel
     */
    virtual void Stop();

  private:
    Ptr<MobilityModel> m_mobility;  //!< Pointer to mobility model object
    Ptr<AntennaModel> m_antenna;    //!< Pointer to antenna model object
    Ptr<NetDevice> m_netDevice;     //!< Pointer to net device object
    Ptr<SpectrumChannel> m_channel; //!< Pointer to spectrum channel object

    /** Sets up signal to be transmitted */
    virtual void SetupTx();

    TvType m_tvType;           //!< Type of TV transmitter
    double m_startFrequency;   //!< Start frequency (in Hz) of TV transmitter's signal
    double m_channelBandwidth; //!< Bandwidth (in Hz) of TV transmitter's signal
    double m_basePsd; //!< Base power spectral density value (in dBm/Hz) of TV transmitter's signal
    Ptr<SpectrumValue> m_txPsd; //!< Pointer to power spectral density of TV transmitter's signal
    Time m_startingTime;     //!< Timepoint after simulation begins that TV transmitter will begin
                             //!< transmitting
    Time m_transmitDuration; //!< Length of time that TV transmitter will transmit for
    bool m_active;           //!< True if TV transmitter is transmitting
};

} // namespace ns3

#endif /* TV_SPECTRUM_TRANSMITTER_H */
