/*
 * Copyright (c) 2009 CTTC
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
 */

#ifndef WIFI_SPECTRUM_PHY_INTERFACE_H
#define WIFI_SPECTRUM_PHY_INTERFACE_H

#include "spectrum-wifi-phy.h"

#include "ns3/he-phy.h"
#include "ns3/spectrum-phy.h"

namespace ns3
{

class SpectrumWifiPhy;

/**
 * \ingroup wifi
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
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Constructor
     *
     * \param freqRange the frequency range covered by the interface
     */
    WifiSpectrumPhyInterface(FrequencyRange freqRange);
    /**
     * Connect SpectrumWifiPhy object
     * \param phy SpectrumWifiPhy object to be connected to this object
     */
    void SetSpectrumWifiPhy(const Ptr<SpectrumWifiPhy> phy);

    /**
     * Get SpectrumWifiPhy object
     * \return Pointer to SpectrumWifiPhy object
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
     * \return the spectrum channel this interface is attached to
     */
    Ptr<SpectrumChannel> GetChannel() const;

    /**
     * Get the frequency range covered by the spectrum channel this interface is attached to
     *
     * \return the frequency range covered by the spectrum channel this interface is attached to
     */
    const FrequencyRange& GetFrequencyRange() const;

    /**
     * Get the center frequency in MHz of the the spectrum channel this interface is attached to
     *
     * \return the center frequency in MHz of the the spectrum channel this interface is attached to
     * to
     */
    uint16_t GetCenterFrequency() const;

    /**
     * Get the channel width in MHz covered by the spectrum channel this interface is attached to
     *
     * \return the channel width in MHz covered by the spectrum channel this interface is attached
     * to to
     */
    uint16_t GetChannelWidth() const;

    /**
     * Start transmission over the spectrum channel
     *
     * \param params the parameters of the signal to transmit
     */
    void StartTx(Ptr<SpectrumSignalParameters> params);

    /**
     * Set the RX spectrum model
     *
     * \param centerFrequency the center frequency in MHz
     * \param channelWidth the channel width in MHz
     * \param bandBandwidth the width of each band in Hz
     * \param guardBandwidth the width of the guard band in MHz
     */
    void SetRxSpectrumModel(uint32_t centerFrequency,
                            uint16_t channelWidth,
                            uint32_t bandBandwidth,
                            uint16_t guardBandwidth);

    /// vector of spectrum bands handled by this interface
    using WifiSpectrumBands = std::vector<WifiSpectrumBandInfo>;

    /**
     * Set the vector of spectrum bands handled by this interface
     *
     * \param bands vector of spectrum bands
     */
    void SetBands(WifiSpectrumBands&& bands);
    /**
     * Get the vector of spectrum bands handled by this interface
     *
     * \return the vector of spectrum bands
     */
    const WifiSpectrumBands& GetBands() const;

    /**
     * Set the HE RU spectrum bands handled by this interface (if any)
     *
     * \param ruBands the HE RU spectrum bands
     */
    void SetRuBands(HePhy::RuBands&& ruBands);
    /**
     * Get the HE RU spectrum bands handled by this interface
     *
     * \return the HE RU spectrum bands
     */
    const HePhy::RuBands& GetRuBands() const;

  private:
    void DoDispose() override;

    FrequencyRange m_frequencyRange;            ///< frequency range
    Ptr<SpectrumWifiPhy> m_spectrumWifiPhy;     ///< spectrum PHY
    Ptr<NetDevice> m_netDevice;                 ///< the device
    Ptr<SpectrumChannel> m_channel;             ///< spectrum channel
    uint16_t m_centerFrequency;                 ///< center frequency in MHz
    uint16_t m_channelWidth;                    ///< channel width in MHz
    Ptr<const SpectrumModel> m_rxSpectrumModel; ///< receive spectrum model

    WifiSpectrumBands
        m_bands; /**< Store all the distinct spectrum bands associated with every channels widths */
    HePhy::RuBands m_ruBands; /**< Store all the distinct spectrum bands associated with every RU */
};

} // namespace ns3

#endif /* WIFI_SPECTRUM_PHY_INTERFACE_H */
