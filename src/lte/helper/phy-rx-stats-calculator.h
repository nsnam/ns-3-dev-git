/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jnin@cttc.es>
 * modified by: Marco Miozzo <mmiozzo@cttc.es>
 *        Convert MacStatsCalculator in PhyRxStatsCalculator
 */

#ifndef PHY_RX_STATS_CALCULATOR_H_
#define PHY_RX_STATS_CALCULATOR_H_

#include "lte-stats-calculator.h"

#include "ns3/lte-common.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"

#include <fstream>
#include <string>

namespace ns3
{

/**
 * @ingroup lte
 *
 * Takes care of storing the information generated at PHY layer regarding
 * reception. Metrics saved are:
 *
 *   - Timestamp (in seconds)
 *   - Frame index
 *   - Subframe index
 *   - C-RNTI
 *   - MCS for transport block 1
 *   - Size of transport block 1
 *   - MCS for transport block 2 (0 if not used)
 *   - Size of transport block 2 (0 if not used)
 */
class PhyRxStatsCalculator : public LteStatsCalculator
{
  public:
    /**
     * Constructor
     */
    PhyRxStatsCalculator();

    /**
     * Destructor
     */
    ~PhyRxStatsCalculator() override;

    // Inherited from ns3::Object
    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the name of the file where the UL Rx PHY statistics will be stored.
     *
     * @param outputFilename string with the name of the file
     */
    void SetUlRxOutputFilename(std::string outputFilename);

    /**
     * Get the name of the file where the UL RX PHY statistics will be stored.
     * @return the name of the file where the UL RX PHY statistics will be stored
     */
    std::string GetUlRxOutputFilename();

    /**
     * Set the name of the file where the DL RX PHY statistics will be stored.
     *
     * @param outputFilename string with the name of the file
     */
    void SetDlRxOutputFilename(std::string outputFilename);

    /**
     * Get the name of the file where the DL RX PHY statistics will be stored.
     * @return the name of the file where the DL RX PHY statistics will be stored
     */
    std::string GetDlRxOutputFilename();

    /**
     * Notifies the stats calculator that an downlink reception has occurred.
     * @param params Trace information regarding PHY reception stats
     */
    void DlPhyReception(PhyReceptionStatParameters params);

    /**
     * Notifies the stats calculator that an uplink reception has occurred.
     * @param params Trace information regarding PHY reception stats
     */
    void UlPhyReception(PhyReceptionStatParameters params);

    /**
     * trace sink
     *
     * @param phyRxStats
     * @param path
     * @param params
     */
    static void DlPhyReceptionCallback(Ptr<PhyRxStatsCalculator> phyRxStats,
                                       std::string path,
                                       PhyReceptionStatParameters params);

    /**
     * trace sink
     *
     * @param phyRxStats
     * @param path
     * @param params
     */
    static void UlPhyReceptionCallback(Ptr<PhyRxStatsCalculator> phyRxStats,
                                       std::string path,
                                       PhyReceptionStatParameters params);

  private:
    /**
     * When writing DL RX PHY statistics first time to file,
     * columns description is added. Then next lines are
     * appended to file. This value is true if output
     * files have not been opened yet
     */
    bool m_dlRxFirstWrite;

    /**
     * When writing UL RX PHY statistics first time to file,
     * columns description is added. Then next lines are
     * appended to file. This value is true if output
     * files have not been opened yet
     */
    bool m_ulRxFirstWrite;

    /**
     * DL RX PHY output trace file
     */
    std::ofstream m_dlRxOutFile;

    /**
     * UL RX PHY output trace file
     */
    std::ofstream m_ulRxOutFile;
};

} // namespace ns3

#endif /* PHY_RX_STATS_CALCULATOR_H_ */
