/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Hoda Ayatollahi <hayatoll@ur.rochester.edu>
 *          Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#ifndef LI_ION_ENERGY_SOURCE_HELPER_H_
#define LI_ION_ENERGY_SOURCE_HELPER_H_

#include "energy-model-helper.h"

#include "ns3/node.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Creates a LiIonEnergySource  object.
 *
 */
class LiIonEnergySourceHelper : public EnergySourceHelper
{
  public:
    LiIonEnergySourceHelper();
    ~LiIonEnergySourceHelper() override;

    void Set(std::string name, const AttributeValue& v) override;

  private:
    Ptr<energy::EnergySource> DoInstall(Ptr<Node> node) const override;

  private:
    ObjectFactory m_liIonEnergySource; //!< LiIon Battery factory
};

} // namespace ns3

#endif /* LI_ION_ENERGY_SOURCE_HELPER_H_ */
