/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef ISOTROPIC_ANTENNA_MODEL_H
#define ISOTROPIC_ANTENNA_MODEL_H

#include "antenna-model.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup antenna
 *
 * @brief Isotropic antenna model
 *
 * This is the simplest antenna model. The gain of this antenna is the same in all directions.
 */
class IsotropicAntennaModel : public AntennaModel
{
  public:
    IsotropicAntennaModel();

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // inherited from AntennaModel
    double GetGainDb(Angles a) override;

  protected:
    /**
     * gain of the antenna in dB, in all directions
     *
     */
    double m_gainDb;
};

} // namespace ns3

#endif // ISOTROPIC_ANTENNA_MODEL_H
