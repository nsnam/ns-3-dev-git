/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#include "cost231-propagation-loss-model.h"

#include "propagation-loss-model.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/pointer.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Cost231PropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(Cost231PropagationLossModel);

TypeId
Cost231PropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Cost231PropagationLossModel")
            .SetParent<PropagationLossModel>()
            .SetGroupName("Propagation")
            .AddConstructor<Cost231PropagationLossModel>()
            .AddAttribute("Lambda",
                          "The wavelength  (default is 2.3 GHz at 300 000 km/s).",
                          DoubleValue(300000000.0 / 2.3e9),
                          MakeDoubleAccessor(&Cost231PropagationLossModel::m_lambda),
                          MakeDoubleChecker<double>())
            .AddAttribute("Frequency",
                          "The Frequency  (default is 2.3 GHz).",
                          DoubleValue(2.3e9),
                          MakeDoubleAccessor(&Cost231PropagationLossModel::m_frequency),
                          MakeDoubleChecker<double>())
            .AddAttribute("BSAntennaHeight",
                          "BS Antenna Height (default is 50m).",
                          DoubleValue(50.0),
                          MakeDoubleAccessor(&Cost231PropagationLossModel::m_BSAntennaHeight),
                          MakeDoubleChecker<double>())
            .AddAttribute("SSAntennaHeight",
                          "SS Antenna Height (default is 3m).",
                          DoubleValue(3),
                          MakeDoubleAccessor(&Cost231PropagationLossModel::m_SSAntennaHeight),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "MinDistance",
                "The distance under which the propagation model refuses to give results (m).",
                DoubleValue(0.5),
                MakeDoubleAccessor(&Cost231PropagationLossModel::SetMinDistance,
                                   &Cost231PropagationLossModel::GetMinDistance),
                MakeDoubleChecker<double>());
    return tid;
}

Cost231PropagationLossModel::Cost231PropagationLossModel()
{
    m_shadowing = 10;
}

void
Cost231PropagationLossModel::SetLambda(double frequency, double speed)
{
    m_lambda = speed / frequency;
    m_frequency = frequency;
}

double
Cost231PropagationLossModel::GetShadowing() const
{
    return m_shadowing;
}

void
Cost231PropagationLossModel::SetShadowing(double shadowing)
{
    m_shadowing = shadowing;
}

void
Cost231PropagationLossModel::SetLambda(double lambda)
{
    m_lambda = lambda;
    m_frequency = 300000000 / lambda;
}

double
Cost231PropagationLossModel::GetLambda() const
{
    return m_lambda;
}

void
Cost231PropagationLossModel::SetMinDistance(double minDistance)
{
    m_minDistance = minDistance;
}

double
Cost231PropagationLossModel::GetMinDistance() const
{
    return m_minDistance;
}

void
Cost231PropagationLossModel::SetBSAntennaHeight(double height)
{
    m_BSAntennaHeight = height;
}

double
Cost231PropagationLossModel::GetBSAntennaHeight() const
{
    return m_BSAntennaHeight;
}

void
Cost231PropagationLossModel::SetSSAntennaHeight(double height)
{
    m_SSAntennaHeight = height;
}

double
Cost231PropagationLossModel::GetSSAntennaHeight() const
{
    return m_SSAntennaHeight;
}

double
Cost231PropagationLossModel::GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    double distance = a->GetDistanceFrom(b);
    if (distance <= m_minDistance)
    {
        return 0.0;
    }

    double logFrequencyMhz = std::log10(m_frequency * 1e-6);
    double logDistanceKm = std::log10(distance * 1e-3);
    double logBSAntennaHeight = std::log10(m_BSAntennaHeight);

    double C_H =
        0.8 + ((1.11 * logFrequencyMhz) - 0.7) * m_SSAntennaHeight - (1.56 * logFrequencyMhz);

    // from the COST231 wiki entry
    // See also http://www.lx.it.pt/cost231/final_report.htm
    // Ch. 4, eq. 4.4.3, pg. 135

    double loss_in_db = 46.3 + (33.9 * logFrequencyMhz) - (13.82 * logBSAntennaHeight) - C_H +
                        ((44.9 - 6.55 * logBSAntennaHeight) * logDistanceKm) + m_shadowing;

    NS_LOG_DEBUG("dist =" << distance << ", Path Loss = " << loss_in_db);

    return (0 - loss_in_db);
}

double
Cost231PropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                           Ptr<MobilityModel> a,
                                           Ptr<MobilityModel> b) const
{
    return txPowerDbm + GetLoss(a, b);
}

int64_t
Cost231PropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
