/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_PROP_MODEL_IDEAL_H
#define UAN_PROP_MODEL_IDEAL_H

#include "uan-prop-model.h"

#include "ns3/mobility-model.h"
#include "ns3/nstime.h"

namespace ns3
{

/**
 * @ingroup uan
 *
 * Ideal propagation model (no pathloss, impulse PDP).
 */
class UanPropModelIdeal : public UanPropModel
{
  public:
    /** Default constructor. */
    UanPropModelIdeal();
    /** Destructor */
    ~UanPropModelIdeal() override;

    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // Inherited methods
    double GetPathLossDb(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;
    UanPdp GetPdp(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;
    Time GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;
};

} // namespace ns3

#endif /* UAN_PROP_MODEL_IDEAL_H */
