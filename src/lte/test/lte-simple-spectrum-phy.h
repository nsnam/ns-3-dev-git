/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#ifndef LTE_SIMPLE_SPECTRUM_PHY_H
#define LTE_SIMPLE_SPECTRUM_PHY_H

#include "ns3/event-id.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-phy.h"
#include "ns3/spectrum-value.h"
#include "ns3/traced-callback.h"

namespace ns3
{

/**
 * @ingroup lte
 *
 * The LteSimpleSpectrumPhy models the physical layer of LTE
 * This class is used to test Frequency Reuse Algorithms,
 * it allow to get SpectrumValue from channel and pass it to
 * test script by trace mechanism.
 * When m_cellId is 0, all received signals will be traced,
 * if m_cellId > 0, only signals from specified Cell will be traced.
 *
 */

class LteSimpleSpectrumPhy : public SpectrumPhy
{
  public:
    LteSimpleSpectrumPhy();
    ~LteSimpleSpectrumPhy() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    // inherited from Object
    void DoDispose() override;

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
     * @brief Set receive spectrum model.
     * @param model the spectrum model
     */
    void SetRxSpectrumModel(Ptr<const SpectrumModel> model);

    /**
     * @brief Set cell ID.
     * @param cellId the cell ID
     */
    void SetCellId(uint16_t cellId);

  private:
    Ptr<MobilityModel> m_mobility;              ///< the mobility model
    Ptr<AntennaModel> m_antenna;                ///< the antenna model
    Ptr<NetDevice> m_device;                    ///< the device
    Ptr<SpectrumChannel> m_channel;             ///< the channel
    Ptr<const SpectrumModel> m_rxSpectrumModel; ///< the spectrum model

    uint16_t m_cellId; ///< the cell ID

    TracedCallback<Ptr<const SpectrumValue>> m_rxStart; ///< receive start trace callback function
};

} // namespace ns3

#endif /* LTE_SIMPLE_SPECTRUM_PHY_H */
