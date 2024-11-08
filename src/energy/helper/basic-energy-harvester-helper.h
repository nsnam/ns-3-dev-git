/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Cristiano Tapparello <cristiano.tapparello@rochester.edu>
 */

#ifndef BASIC_ENERGY_HARVESTER_HELPER_H
#define BASIC_ENERGY_HARVESTER_HELPER_H

#include "energy-harvester-helper.h"

#include "ns3/energy-source.h"
#include "ns3/node.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Creates a BasicEnergyHarvester object.
 */
class BasicEnergyHarvesterHelper : public EnergyHarvesterHelper
{
  public:
    BasicEnergyHarvesterHelper();
    ~BasicEnergyHarvesterHelper() override;

    void Set(std::string name, const AttributeValue& v) override;

  private:
    Ptr<energy::EnergyHarvester> DoInstall(Ptr<energy::EnergySource> source) const override;

  private:
    ObjectFactory m_basicEnergyHarvester; //!< Energy source factory
};

} // namespace ns3

#endif /* defined(BASIC_ENERGY_HARVESTER_HELPER_H) */
