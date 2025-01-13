/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SPECTRUM_ANALYZER_H
#define SPECTRUM_ANALYZER_H

#include "spectrum-channel.h"
#include "spectrum-phy.h"
#include "spectrum-value.h"

#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

#include <fstream>
#include <string>

namespace ns3
{

/**
 * @ingroup spectrum
 *
 * Simple SpectrumPhy implementation that averages the spectrum power
 * density of incoming transmissions to produce a spectrogram.
 *
 *
 * This PHY model supports a single antenna model instance which is
 * used for reception (this PHY model never transmits).
 */
class SpectrumAnalyzer : public SpectrumPhy
{
  public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from SpectrumPhy
    void SetChannel(Ptr<SpectrumChannel> c) override;
    void SetMobility(Ptr<MobilityModel> m) override;
    void SetDevice(Ptr<NetDevice> d) override;
    Ptr<MobilityModel> GetMobility() const override;
    Ptr<NetDevice> GetDevice() const override;
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override;
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Set the spectrum model used by the SpectrumAnalyzer to represent incoming signals
     *
     * @param m the Rx Spectrum model
     */
    void SetRxSpectrumModel(Ptr<SpectrumModel> m);

    /**
     * Set the AntennaModel to be used
     *
     * @param a the Antenna Model
     */
    void SetAntenna(Ptr<AntennaModel> a);

    /**
     * Start the spectrum analyzer
     *
     */
    virtual void Start();

    /**
     * Stop the spectrum analyzer
     *
     */
    virtual void Stop();

  protected:
    void DoDispose() override;

  private:
    Ptr<MobilityModel> m_mobility;  //!< Pointer to the mobility model
    Ptr<AntennaModel> m_antenna;    //!< Pointer to the Antenna model
    Ptr<NetDevice> m_netDevice;     //!< Pointer to the NetDevice using this object
    Ptr<SpectrumChannel> m_channel; //!< Pointer to the channel to be analyzed

    /**
     * Generates a report of the data collected so far.
     *
     * This function is called periodically.
     */
    virtual void GenerateReport();

    /**
     * Adds a signal to the data collected.
     * @param psd signal to add
     */
    void AddSignal(Ptr<const SpectrumValue> psd);
    /**
     * Removes a signal to the data collected.
     * @param psd signal to subtract
     */
    void SubtractSignal(Ptr<const SpectrumValue> psd);
    /**
     * Updates the data about the received Energy
     */
    void UpdateEnergyReceivedSoFar();

    Ptr<SpectrumModel> m_spectrumModel;           //!< Spectrum model
    Ptr<SpectrumValue> m_sumPowerSpectralDensity; //!< Sum of the received PSD
    Ptr<SpectrumValue> m_energySpectralDensity;   //!< Energy spectral density
    double m_noisePowerSpectralDensity;           //!< Noise power spectral density
    Time m_resolution;                            //!< Time resolution
    Time m_lastChangeTime;                        //!< When the last update happened
    bool m_active;                                //!< True if the analyzer is active

    /// TracedCallback - average power spectral density report.
    TracedCallback<Ptr<const SpectrumValue>> m_averagePowerSpectralDensityReportTrace;
};

} // namespace ns3

#endif /* SPECTRUM_ANALYZER_H */
