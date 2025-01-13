/*
 * Copyright (c) 2012 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef REM_SPECTRUM_PHY_H
#define REM_SPECTRUM_PHY_H

#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-phy.h"
#include "ns3/spectrum-value.h"

#include <fstream>
#include <string>

namespace ns3
{

/**
 *
 * This minimal SpectrumPhy implementation calculates the SINR with
 * respect to the strongest signal for a given point. The original
 * purpose of this class is to be used to generate a
 * Radio Environment Map (REM) by locating several instances in a grid
 * fashion, and connecting them to the channel only for a very short
 * amount of time.
 *
 * The assumption on which this class works is that the system
 * being considered is an infrastructure radio access network using
 * FDD, hence all signals will be transmitted simultaneously.
 */
class RemSpectrumPhy : public SpectrumPhy
{
  public:
    RemSpectrumPhy();
    ~RemSpectrumPhy() override;

    // inherited from Object
    void DoDispose() override;
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
     * set the RX spectrum model to be used
     *
     * @param m
     */
    void SetRxSpectrumModel(Ptr<const SpectrumModel> m);

    /**
     *
     * @param noisePower the noise power
     * @return the Signal to Noise Ratio calculated
     */
    double GetSinr(double noisePower) const;

    /**
     * make StartRx a no-op from now on, and mark instance as inactive
     *
     */
    void Deactivate();

    /**
     *
     * @return true if active
     */
    bool IsActive() const;

    /**
     * Reset the SINR calculator
     *
     */
    void Reset();

    /**
     * set usage of DataChannel
     *
     * @param value if true, data channel signal will be processed, control signal otherwise
     */
    void SetUseDataChannel(bool value);

    /**
     * set RB Id
     *
     * @param rbId Resource Block Id which will be processed
     */
    void SetRbId(int32_t rbId);

  private:
    Ptr<MobilityModel> m_mobility;              ///< the mobility model
    Ptr<const SpectrumModel> m_rxSpectrumModel; ///< receive spectrum model

    double m_referenceSignalPower; ///< reference signal power
    double m_sumPower;             ///< sum power

    bool m_active; ///< is active?

    bool m_useDataChannel; ///< use data channel
    int32_t m_rbId;        ///< RBID
};

} // namespace ns3

#endif /* REM_SPECTRUM_PHY_H */
