/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#ifndef RV_BATTERY_MODEL_HELPER_H
#define RV_BATTERY_MODEL_HELPER_H

#include "energy-model-helper.h"

#include "ns3/node.h"

namespace ns3
{

/**
 * @ingroup energy
 * @brief Creates a RvBatteryModel object.
 *
 */
class RvBatteryModelHelper : public EnergySourceHelper
{
  public:
    RvBatteryModelHelper();
    ~RvBatteryModelHelper() override;

    void Set(std::string name, const AttributeValue& v) override;

  private:
    Ptr<energy::EnergySource> DoInstall(Ptr<Node> node) const override;

  private:
    ObjectFactory m_rvBatteryModel; //!< RV Battery factory
};

} // namespace ns3

#endif /* RV_BATTERY_MODEL_HELPER_H */
