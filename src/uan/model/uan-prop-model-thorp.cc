/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */
#include "uan-prop-model-thorp.h"

#include "uan-tx-mode.h"

#include "ns3/double.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UanPropModelThorp");

NS_OBJECT_ENSURE_REGISTERED(UanPropModelThorp);

UanPropModelThorp::UanPropModelThorp()
{
}

UanPropModelThorp::~UanPropModelThorp()
{
}

TypeId
UanPropModelThorp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UanPropModelThorp")
            .SetParent<UanPropModel>()
            .SetGroupName("Uan")
            .AddConstructor<UanPropModelThorp>()
            .AddAttribute("SpreadCoef",
                          "Spreading coefficient used in calculation of Thorp's approximation.",
                          DoubleValue(1.5),
                          MakeDoubleAccessor(&UanPropModelThorp::m_SpreadCoef),
                          MakeDoubleChecker<double>());
    return tid;
}

double
UanPropModelThorp::GetPathLossDb(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    double dist = a->GetDistanceFrom(b);

    return m_SpreadCoef * 10.0 * std::log10(dist) +
           (dist / 1000.0) * GetAttenDbKm(mode.GetCenterFreqHz() / 1000.0);
}

UanPdp
UanPropModelThorp::GetPdp(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    return UanPdp::CreateImpulsePdp();
}

Time
UanPropModelThorp::GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode)
{
    return Seconds(a->GetDistanceFrom(b) / 1500.0);
}

double
UanPropModelThorp::GetAttenDbKyd(double freqKhz)
{
    return GetAttenDbKm(freqKhz) / 1.093613298;
}

double
UanPropModelThorp::GetAttenDbKm(double freqKhz)
{
    double fsq = freqKhz * freqKhz;
    double atten;

    if (freqKhz >= 0.4)
    {
        atten = 0.11 * fsq / (1 + fsq) + 44 * fsq / (4100 + fsq) + 2.75 * 0.0001 * fsq + 0.003;
    }
    else
    {
        atten = 0.002 + 0.11 * (fsq / (1 + fsq)) + 0.011 * fsq;
    }

    return atten;
}

} // namespace ns3
