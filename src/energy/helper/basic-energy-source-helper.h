/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#ifndef BASIC_ENERGY_SOURCE_HELPER_H
#define BASIC_ENERGY_SOURCE_HELPER_H

#include "energy-model-helper.h"

#include "ns3/node.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Creates a BasicEnergySource object.
 *
 */
class BasicEnergySourceHelper : public EnergySourceHelper
{
  public:
    BasicEnergySourceHelper();
    ~BasicEnergySourceHelper() override;

    void Set(std::string name, const AttributeValue& v) override;

  private:
    Ptr<energy::EnergySource> DoInstall(Ptr<Node> node) const override;

  private:
    ObjectFactory m_basicEnergySource; //!< Energy source factory
};

} // namespace ns3

#endif /* BASIC_ENERGY_SOURCE_HELPER_H */
