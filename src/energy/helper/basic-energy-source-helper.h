/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * \ingroup energy
 * \brief Creates a BasicEnergySource object.
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
