/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef CONSTANT_OBSS_PD_ALGORITHM_H
#define CONSTANT_OBSS_PD_ALGORITHM_H

#include "obss-pd-algorithm.h"

namespace ns3
{

/**
 * @brief Constant OBSS PD algorithm
 * @ingroup wifi
 *
 * This constant OBSS_PD algorithm is a simple OBSS_PD algorithm which evaluates if a receiving
 * signal should be accepted or rejected based on a constant threshold.
 *
 * Once a HE-SIG-A has been received by the PHY, the ReceiveHeSigA method is
 * triggered. The algorithm then checks whether this is an OBSS frame by comparing its own BSS
 * color with the BSS color of the received preamble. If this is an OBSS frame, it compares the
 * received RSSI with its configured OBSS_PD level value. The PHY then gets reset to IDLE state
 * in case the received RSSI is lower than that constant OBSS PD level value, and is informed
 * about TX power restrictions that might be applied to the next transmission.
 */
class ConstantObssPdAlgorithm : public ObssPdAlgorithm
{
  public:
    ConstantObssPdAlgorithm();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    void ConnectWifiNetDevice(const Ptr<WifiNetDevice> device) override;
    void ReceiveHeSigA(HeSigAParameters params) override;
};

} // namespace ns3

#endif /* CONSTANT_OBSS_PD_ALGORITHM_H */
