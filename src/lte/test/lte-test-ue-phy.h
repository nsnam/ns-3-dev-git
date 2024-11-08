/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 * (Based on lte-ue-phy code)
 */

#ifndef LTE_TEST_UE_PHY_H
#define LTE_TEST_UE_PHY_H

#include "ns3/lte-control-messages.h"
#include "ns3/lte-phy.h"

namespace ns3
{

/**
 * @ingroup lte-test
 *
 * @brief Defines a simplified LtePhy class that is used for testing purposes
 * of downlink and uplink SINR generation. Used in LteDownlinkDataSinrTestCase
 * and LteUplinkDataSinrTestCase as simplified LTE PHY.
 */
class LteTestUePhy : public LtePhy
{
  public:
    /**
     * @warning the default constructor should not be used
     */
    LteTestUePhy();

    /**
     * @param dlPhy the downlink LteSpectrumPhy instance
     * @param ulPhy the uplink LteSpectrumPhy instance
     */
    LteTestUePhy(Ptr<LteSpectrumPhy> dlPhy, Ptr<LteSpectrumPhy> ulPhy);

    ~LteTestUePhy() override;

    void DoDispose() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Queue the MAC PDU to be sent
     * @param p the MAC PDU to sent
     */
    void DoSendMacPdu(Ptr<Packet> p) override;

    /**
     * @brief Create the PSD for the TX
     * @return the pointer to the PSD
     */
    Ptr<SpectrumValue> CreateTxPowerSpectralDensity() override;

    void GenerateCtrlCqiReport(const SpectrumValue& sinr) override;

    void GenerateDataCqiReport(const SpectrumValue& sinr) override;

    void ReportInterference(const SpectrumValue& interf) override;

    void ReportRsReceivedPower(const SpectrumValue& power) override;

    /**
     * @brief Reeive LTE Control Message
     * @param msg the control message
     */
    virtual void ReceiveLteControlMessage(Ptr<LteControlMessage> msg);

    /**
     * @brief Get the SINR
     * @return the SINR
     */
    SpectrumValue GetSinr();

  private:
    SpectrumValue m_sinr; ///< the SINR
};

} // namespace ns3

#endif /* LTE_TEST_UE_PHY_H */
