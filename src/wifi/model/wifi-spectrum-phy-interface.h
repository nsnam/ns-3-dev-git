/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef WIFI_SPECTRUM_PHY_INTERFACE_H
#define WIFI_SPECTRUM_PHY_INTERFACE_H

#include "spectrum-wifi-phy.h"

#include "ns3/he-phy.h"
#include "ns3/spectrum-phy.h"

#include <vector>

namespace ns3
{

class SpectrumWifiPhy;

/**
 * @ingroup wifi
 *
 * This class is an adaptor between class SpectrumWifiPhy (which inherits
 * from WifiPhy) and class SpectrumChannel (which expects objects derived
 * from class SpectrumPhy to be connected to it).
 *
 * The adaptor is used only in the receive direction; in the transmit
 * direction, the class SpectrumWifiPhy constructs signal parameters
 * and directly accesses the SpectrumChannel
 */
class WifiSpectrumPhyInterface : public SpectrumPhy
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Constructor
     *
     * @param freqRange the frequency range covered by the interface
     */
    WifiSpectrumPhyInterface(FrequencyRange freqRange);
    /**
     * Connect SpectrumWifiPhy object
     * @param phy SpectrumWifiPhy object to be connected to this object
     */
    void SetSpectrumWifiPhy(const Ptr<SpectrumWifiPhy> phy);

    /**
     * Get SpectrumWifiPhy object
     * @return Pointer to SpectrumWifiPhy object
     */
    Ptr<const SpectrumWifiPhy> GetSpectrumWifiPhy() const;

    Ptr<NetDevice> GetDevice() const override;
    void SetDevice(const Ptr<NetDevice> d) override;
    void SetMobility(const Ptr<MobilityModel> m) override;
    Ptr<MobilityModel> GetMobility() const override;
    void SetChannel(const Ptr<SpectrumChannel> c) override;
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override;
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Get the spectrum channel this interface is attached to
     *
     * @return the spectrum channel this interface is attached to
     */
    Ptr<SpectrumChannel> GetChannel() const;

    /**
     * Get the frequency range covered by the spectrum channel this interface is attached to
     *
     * @return the frequency range covered by the spectrum channel this interface is attached to
     */
    const FrequencyRange& GetFrequencyRange() const;

    /**
     * Get the center frequency for each segment of the the spectrum channel this interface is
     * attached to
     *
     * @return the center frequency for each segment of the the spectrum channel this interface is
     * attached to to
     */
    const std::vector<MHz_u>& GetCenterFrequencies() const;

    /**
     * Get the channel width covered by the spectrum channel this interface is attached to
     *
     * @return the channel width covered by the spectrum channel this interface is attached
     * to to
     */
    MHz_u GetChannelWidth() const;

    /**
     * Start transmission over the spectrum channel
     *
     * @param params the parameters of the signal to transmit
     */
    void StartTx(Ptr<SpectrumSignalParameters> params);

    /**
     * Set the RX spectrum model
     *
     * @param centerFrequencies the center frequency for each segment
     * @param channelWidth the total channel width
     * @param bandBandwidth the width of each band
     * @param guardBandwidth the total width of the guard band
     */
    void SetRxSpectrumModel(const std::vector<MHz_u>& centerFrequencies,
                            MHz_u channelWidth,
                            Hz_u bandBandwidth,
                            MHz_u guardBandwidth);

    /**
     * Set the vector of spectrum bands handled by this interface
     *
     * @param bands vector of spectrum bands
     */
    void SetBands(WifiSpectrumBands&& bands);
    /**
     * Get the vector of spectrum bands handled by this interface
     *
     * @return the vector of spectrum bands
     */
    const WifiSpectrumBands& GetBands() const;

    /**
     * Set the HE RU spectrum bands handled by this interface (if any)
     *
     * @param heRuBands the HE RU spectrum bands
     */
    void SetHeRuBands(HeRuBands&& heRuBands);
    /**
     * Get the HE RU spectrum bands handled by this interface
     *
     * @return the HE RU spectrum bands
     */
    const HeRuBands& GetHeRuBands() const;

  private:
    void DoDispose() override;

    FrequencyRange m_frequencyRange;            ///< frequency range
    Ptr<SpectrumWifiPhy> m_spectrumWifiPhy;     ///< spectrum PHY
    Ptr<NetDevice> m_netDevice;                 ///< the device
    Ptr<SpectrumChannel> m_channel;             ///< spectrum channel
    std::vector<MHz_u> m_centerFrequencies;     ///< center frequency per segment
    MHz_u m_channelWidth;                       ///< channel width
    Ptr<const SpectrumModel> m_rxSpectrumModel; ///< receive spectrum model

    WifiSpectrumBands
        m_bands; /**< Store all the distinct spectrum bands associated with every channels widths */
    HeRuBands m_heRuBands; /**< Store all the distinct spectrum bands associated with every RU */
};

} // namespace ns3

#endif /* WIFI_SPECTRUM_PHY_INTERFACE_H */
