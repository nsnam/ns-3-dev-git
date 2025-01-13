/*
 * Copyright (c) 2011,2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_GLOBAL_PATHLOSS_DATABASE_H
#define LTE_GLOBAL_PATHLOSS_DATABASE_H

#include "ns3/log.h"
#include "ns3/ptr.h"

#include <map>
#include <string>

namespace ns3
{

class SpectrumPhy;

/**
 * @ingroup lte
 *
 * Store the last pathloss value for each TX-RX pair. This is an
 * example of how the PathlossTrace (provided by some SpectrumChannel
 * implementations) work.
 *
 */
class LteGlobalPathlossDatabase
{
  public:
    virtual ~LteGlobalPathlossDatabase();

    /**
     * update the pathloss value
     *
     * @param context
     * @param txPhy the transmitting PHY
     * @param rxPhy the receiving PHY
     * @param lossDb the loss in dB
     */
    virtual void UpdatePathloss(std::string context,
                                Ptr<const SpectrumPhy> txPhy,
                                Ptr<const SpectrumPhy> rxPhy,
                                double lossDb) = 0;

    /**
     *
     *
     * @param cellId the id of the eNB
     * @param imsi the id of the UE
     *
     * @return the pathloss value between the UE and the eNB
     */
    double GetPathloss(uint16_t cellId, uint64_t imsi);

    /**
     * print the stored pathloss values to standard output
     *
     */
    void Print();

  protected:
    /**
     * List of the last pathloss value for each UE by CellId.
     * ( CELL ID,  ( IMSI,PATHLOSS ))
     */
    std::map<uint16_t, std::map<uint64_t, double>> m_pathlossMap;
};

/**
 * @ingroup lte
 * Store the last pathloss value for each TX-RX pair for downlink
 */
class DownlinkLteGlobalPathlossDatabase : public LteGlobalPathlossDatabase
{
  public:
    // inherited from LteGlobalPathlossDatabase
    void UpdatePathloss(std::string context,
                        Ptr<const SpectrumPhy> txPhy,
                        Ptr<const SpectrumPhy> rxPhy,
                        double lossDb) override;
};

/**
 * @ingroup lte
 * Store the last pathloss value for each TX-RX pair for uplink
 */
class UplinkLteGlobalPathlossDatabase : public LteGlobalPathlossDatabase
{
  public:
    // inherited from LteGlobalPathlossDatabase
    void UpdatePathloss(std::string context,
                        Ptr<const SpectrumPhy> txPhy,
                        Ptr<const SpectrumPhy> rxPhy,
                        double lossDb) override;
};

} // namespace ns3

#endif // LTE_GLOBAL_PATHLOSS_DATABASE_H
