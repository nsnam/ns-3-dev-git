/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */

#ifndef ITU_R_1238_PROPAGATION_LOSS_MODEL_H
#define ITU_R_1238_PROPAGATION_LOSS_MODEL_H

#include "ns3/propagation-environment.h"
#include "ns3/propagation-loss-model.h"

namespace ns3
{

/**
 * @ingroup buildings
 * @ingroup propagation
 *
 * This class implements the ITU-R 1238 propagation loss model.
 *
 */
class ItuR1238PropagationLossModel : public PropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     *
     *
     * @param a the first mobility model
     * @param b the second mobility model
     *
     * @return the loss in dBm for the propagation between
     * the two given mobility models
     */
    double GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

  private:
    // inherited from PropagationLossModel
    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;
    int64_t DoAssignStreams(int64_t stream) override;

    double m_frequency; ///< frequency in MHz
};

} // namespace ns3

#endif // ITU_R_1238_PROPAGATION_LOSS_MODEL_H
