/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-prop-model-ideal.h"

#include "uan-tx-mode.h"

#include "ns3/mobility-model.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UanPropModelIdeal);

UanPropModelIdeal::UanPropModelIdeal()
{
}

UanPropModelIdeal::~UanPropModelIdeal()
{
}

TypeId
UanPropModelIdeal::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UanPropModelIdeal")
                            .SetParent<UanPropModel>()
                            .SetGroupName("Uan")
                            .AddConstructor<UanPropModelIdeal>();
    return tid;
}

double
UanPropModelIdeal::GetPathLossDb(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    return 0;
}

UanPdp
UanPropModelIdeal::GetPdp(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    return UanPdp::CreateImpulsePdp();
}

Time
UanPropModelIdeal::GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    return Seconds(a->GetDistanceFrom(b) / 1500.0);
}

} // namespace ns3
