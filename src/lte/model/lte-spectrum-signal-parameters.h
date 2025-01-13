/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified by Marco Miozzo <mmiozzo@cttc.es> (add data and ctrl diversity)
 */

#ifndef LTE_SPECTRUM_SIGNAL_PARAMETERS_H
#define LTE_SPECTRUM_SIGNAL_PARAMETERS_H

#include "ns3/spectrum-signal-parameters.h"

#include <list>

namespace ns3
{

class PacketBurst;
class LteControlMessage;

/**
 * @ingroup lte
 *
 * Signal parameters for Lte
 */
struct LteSpectrumSignalParameters : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    LteSpectrumSignalParameters();

    /**
     * copy constructor
     * @param p the LteSpectrumSignalParameters to copy
     */
    LteSpectrumSignalParameters(const LteSpectrumSignalParameters& p);

    /**
     * The packet burst being transmitted with this signal
     */
    Ptr<PacketBurst> packetBurst;
};

/**
 * @ingroup lte
 *
 * Signal parameters for Lte Data Frame (PDSCH), and eventually after some
 * control messages through other control channel embedded in PDSCH
 * (i.e. PBCH)
 */
struct LteSpectrumSignalParametersDataFrame : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    LteSpectrumSignalParametersDataFrame();

    /**
     * copy constructor
     * @param p the LteSpectrumSignalParametersDataFrame to copy
     */
    LteSpectrumSignalParametersDataFrame(const LteSpectrumSignalParametersDataFrame& p);

    /**
     * The packet burst being transmitted with this signal
     */
    Ptr<PacketBurst> packetBurst;

    std::list<Ptr<LteControlMessage>> ctrlMsgList; ///< the control message list

    uint16_t cellId; ///< cell ID
};

/**
 * @ingroup lte
 *
 * Signal parameters for Lte DL Ctrl Frame (RS, PCFICH and PDCCH)
 */
struct LteSpectrumSignalParametersDlCtrlFrame : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    LteSpectrumSignalParametersDlCtrlFrame();

    /**
     * copy constructor
     * @param p the LteSpectrumSignalParametersDlCtrlFrame to copy
     */
    LteSpectrumSignalParametersDlCtrlFrame(const LteSpectrumSignalParametersDlCtrlFrame& p);

    std::list<Ptr<LteControlMessage>> ctrlMsgList; ///< control message list

    uint16_t cellId; ///< cell ID
    bool pss;        ///< primary synchronization signal
};

/**
 * @ingroup lte
 *
 * Signal parameters for Lte SRS Frame
 */
struct LteSpectrumSignalParametersUlSrsFrame : public SpectrumSignalParameters
{
    Ptr<SpectrumSignalParameters> Copy() const override;

    /**
     * default constructor
     */
    LteSpectrumSignalParametersUlSrsFrame();

    /**
     * copy constructor
     * @param p the LteSpectrumSignalParametersUlSrsFrame to copy
     */
    LteSpectrumSignalParametersUlSrsFrame(const LteSpectrumSignalParametersUlSrsFrame& p);

    uint16_t cellId; ///< cell ID
};

} // namespace ns3

#endif /* LTE_SPECTRUM_SIGNAL_PARAMETERS_H */
