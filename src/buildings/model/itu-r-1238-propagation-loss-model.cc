/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */
#include "itu-r-1238-propagation-loss-model.h"

#include "mobility-building-info.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ItuR1238PropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(ItuR1238PropagationLossModel);

TypeId
ItuR1238PropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ItuR1238PropagationLossModel")

            .SetParent<PropagationLossModel>()
            .SetGroupName("Buildings")

            .AddAttribute("Frequency",
                          "The Frequency  (default is 2.106 GHz).",
                          DoubleValue(2160e6),
                          MakeDoubleAccessor(&ItuR1238PropagationLossModel::m_frequency),
                          MakeDoubleChecker<double>());

    return tid;
}

double
ItuR1238PropagationLossModel::GetLoss(Ptr<MobilityModel> a1, Ptr<MobilityModel> b1) const
{
    NS_LOG_FUNCTION(this << a1 << b1);
    Ptr<MobilityBuildingInfo> a = a1->GetObject<MobilityBuildingInfo>();
    Ptr<MobilityBuildingInfo> b = b1->GetObject<MobilityBuildingInfo>();
    NS_ASSERT_MSG(a && b, "ItuR1238PropagationLossModel only works with MobilityBuildingInfo");
    NS_ASSERT_MSG(a->GetBuilding()->GetId() == b->GetBuilding()->GetId(),
                  "ITU-R 1238 applies only to nodes that are in the same building");
    double N = 0.0;
    int n = std::abs(a->GetFloorNumber() - b->GetFloorNumber());
    NS_LOG_LOGIC(this << " A floor " << (uint16_t)a->GetFloorNumber() << " B floor "
                      << (uint16_t)b->GetFloorNumber() << " n " << n);
    double Lf = 0.0;
    Ptr<Building> aBuilding = a->GetBuilding();
    if (aBuilding->GetBuildingType() == Building::Residential)
    {
        N = 28;
        if (n >= 1)
        {
            Lf = 4 * n;
        }
        NS_LOG_LOGIC(this << " Residential ");
    }
    else if (aBuilding->GetBuildingType() == Building::Office)
    {
        N = 30;
        if (n >= 1)
        {
            Lf = 15 + (4 * (n - 1));
        }
        NS_LOG_LOGIC(this << " Office ");
    }
    else if (aBuilding->GetBuildingType() == Building::Commercial)
    {
        N = 22;
        if (n >= 1)
        {
            Lf = 6 + (3 * (n - 1));
        }
        NS_LOG_LOGIC(this << " Commercial ");
    }
    else
    {
        NS_LOG_ERROR(this << " Unkwnon Wall Type");
    }
    double loss = 20 * std::log10(m_frequency / 1e6 /*MHz*/) +
                  N * std::log10(a1->GetDistanceFrom(b1)) + Lf - 28.0;
    NS_LOG_INFO(this << " Node " << a1->GetPosition() << " <-> " << b1->GetPosition()
                     << " loss = " << loss << " dB");

    return loss;
}

double
ItuR1238PropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                            Ptr<MobilityModel> a,
                                            Ptr<MobilityModel> b) const
{
    return (txPowerDbm - GetLoss(a, b));
}

int64_t
ItuR1238PropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
