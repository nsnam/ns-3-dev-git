/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *
 */

#ifndef OH_BUILDINGS_PROPAGATION_LOSS_MODEL_H_
#define OH_BUILDINGS_PROPAGATION_LOSS_MODEL_H_

#include "buildings-propagation-loss-model.h"

namespace ns3
{

class OkumuraHataPropagationLossModel;

/**
 * @ingroup buildings
 * @ingroup propagation
 *
 *  this model combines the OkumuraHata model with the BuildingsPropagationLossModel
 *
 *  @warning This model works with MobilityBuildingInfo only
 *
 */
class OhBuildingsPropagationLossModel : public BuildingsPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    OhBuildingsPropagationLossModel();
    ~OhBuildingsPropagationLossModel() override;

    /**
     * @param a the mobility model of the source
     * @param b the mobility model of the destination
     * @returns the propagation loss (in dBm)
     */
    double GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

  private:
    Ptr<OkumuraHataPropagationLossModel> m_okumuraHata; //!< OkumuraHata Propagation Loss Model
};

} // namespace ns3

#endif /* OH_BUILDINGS_PROPAGATION_LOSS_MODEL_H_ */
