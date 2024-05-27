/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
 */

#include "three-gpp-propagation-loss-model.h"

#include "channel-condition-model.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include <cmath>

namespace
{
/**
 * The enumerator used for code clarity when performing parameter assignment in the GetLoss Methods
 */
enum SFCL_params
{
    S_LOS_sigF,
    S_NLOS_sigF,
    S_NLOS_CL,
    Ka_LOS_sigF,
    Ka_NLOS_sigF,
    Ka_NLOS_CL,
};

/**
 * The map containing the 3GPP value regarding Shadow Fading and Clutter Loss tables for the
 * NTN Dense Urban scenario
 */
const std::map<int, std::vector<float>> SFCL_DenseUrban{
    {10, {3.5, 15.5, 34.3, 2.9, 17.1, 44.3}},
    {20, {3.4, 13.9, 30.9, 2.4, 17.1, 39.9}},
    {30, {2.9, 12.4, 29.0, 2.7, 15.6, 37.5}},
    {40, {3.0, 11.7, 27.7, 2.4, 14.6, 35.8}},
    {50, {3.1, 10.6, 26.8, 2.4, 14.2, 34.6}},
    {60, {2.7, 10.5, 26.2, 2.7, 12.6, 33.8}},
    {70, {2.5, 10.1, 25.8, 2.6, 12.1, 33.3}},
    {80, {2.3, 9.2, 25.5, 2.8, 12.3, 33.0}},
    {90, {1.2, 9.2, 25.5, 0.6, 12.3, 32.9}},
};

/**
 * The map containing the 3GPP value regarding Shadow Fading and Clutter Loss tables for the
 * NTN Urban scenario
 */
const std::map<int, std::vector<float>> SFCL_Urban{
    {10, {4, 6, 34.3, 4, 6, 44.3}},
    {20, {4, 6, 30.9, 4, 6, 39.9}},
    {30, {4, 6, 29.0, 4, 6, 37.5}},
    {40, {4, 6, 27.7, 4, 6, 35.8}},
    {50, {4, 6, 26.8, 4, 6, 34.6}},
    {60, {4, 6, 26.2, 4, 6, 33.8}},
    {70, {4, 6, 25.8, 4, 6, 33.3}},
    {80, {4, 6, 25.5, 4, 6, 33.0}},
    {90, {4, 6, 25.5, 4, 6, 32.9}},
};

/**
 * The map containing the 3GPP value regarding Shadow Fading and Clutter Loss tables for the
 * NTN Suburban and Rural scenarios
 */
const std::map<int, std::vector<float>> SFCL_SuburbanRural{
    {10, {1.79, 8.93, 19.52, 1.9, 10.7, 29.5}},
    {20, {1.14, 9.08, 18.17, 1.6, 10.0, 24.6}},
    {30, {1.14, 8.78, 18.42, 1.9, 11.2, 21.9}},
    {40, {0.92, 10.25, 18.28, 2.3, 11.6, 20.0}},
    {50, {1.42, 10.56, 18.63, 2.7, 11.8, 18.7}},
    {60, {1.56, 10.74, 17.68, 3.1, 10.8, 17.8}},
    {70, {0.85, 10.17, 16.5, 3.0, 10.8, 17.2}},
    {80, {0.72, 11.52, 16.3, 3.6, 10.8, 16.9}},
    {90, {0.72, 11.52, 16.3, 0.4, 10.8, 16.8}},
};

/**
 * Array containing the attenuation given by atmospheric absorption. 100 samples are selected for
 * frequencies from 1GHz to 100GHz. In order to get the atmospheric absorption loss for a given
 * frequency f: 1- round f to the closest integer between 0 and 100. 2- use the obtained integer to
 * access the corresponding element in the array, that will give the attenuation at that frequency.
 * Data is obtained form ITU-R P.676 Figure 6.
 */
const double atmosphericAbsorption[101] = {
    0,        0.0300,   0.0350,  0.0380,  0.0390,  0.0410,  0.0420,  0.0450,  0.0480,   0.0500,
    0.0530,   0.0587,   0.0674,  0.0789,  0.0935,  0.1113,  0.1322,  0.1565,  0.1841,   0.2153,
    0.2500,   0.3362,   0.4581,  0.5200,  0.5200,  0.5000,  0.4500,  0.3850,  0.3200,   0.2700,
    0.2500,   0.2517,   0.2568,  0.2651,  0.2765,  0.2907,  0.3077,  0.3273,  0.3493,   0.3736,
    0.4000,   0.4375,   0.4966,  0.5795,  0.6881,  0.8247,  0.9912,  1.1900,  1.4229,   1.6922,
    2.0000,   4.2654,   10.1504, 19.2717, 31.2457, 45.6890, 62.2182, 80.4496, 100.0000, 140.0205,
    170.0000, 100.0000, 78.1682, 59.3955, 43.5434, 30.4733, 20.0465, 12.1244, 6.5683,   3.2397,
    2.0000,   1.7708,   1.5660,  1.3858,  1.2298,  1.0981,  0.9905,  0.9070,  0.8475,   0.8119,
    0.8000,   0.8000,   0.8000,  0.8000,  0.8000,  0.8000,  0.8000,  0.8000,  0.8000,   0.8000,
    0.8000,   0.8029,   0.8112,  0.8243,  0.8416,  0.8625,  0.8864,  0.9127,  0.9408,   0.9701,
    1.0000};

/**
 * Map containing the Tropospheric attenuation in dB with 99% probability at 20 GHz in Toulouse
 * used for tropospheric scintillation losses. From Table 6.6.6.2.1-1 of 3GPP TR 38.811.
 */
const std::map<int, float> troposphericScintillationLoss{
    {10, {1.08}},
    {20, {0.48}},
    {30, {0.30}},
    {40, {0.22}},
    {50, {0.17}},
    {60, {0.13}},
    {70, {0.12}},
    {80, {0.12}},
    {90, {0.12}},
};

/**
 * @brief Get the base station and user terminal relative distances and heights
 *
 * @param a the mobility model of terminal a
 * @param b the mobility model of terminal b
 *
 * @return The tuple [dist2D, dist3D, hBs, hUt], where dist2D and dist3D
 * are the 2D and 3D distances between a and b, respectively, hBs is the bigger
 * height and hUt the smallest.
 */
std::tuple<double, double, double, double>
GetBsUtDistancesAndHeights(ns3::Ptr<const ns3::MobilityModel> a,
                           ns3::Ptr<const ns3::MobilityModel> b)
{
    auto aPos = a->GetPosition();
    auto bPos = b->GetPosition();
    double distance2D = ns3::ThreeGppChannelConditionModel::Calculate2dDistance(aPos, bPos);
    double distance3D = ns3::CalculateDistance(aPos, bPos);
    double hBs = std::max(aPos.z, bPos.z);
    double hUt = std::min(aPos.z, bPos.z);
    return std::make_tuple(distance2D, distance3D, hBs, hUt);
};

/**
 * @brief Get the base station and user terminal heights for the UmiStreetCanyon scenario
 *
 * @param heightA the first height in meters
 * @param heightB the second height in meters
 *
 * @return The tuple [hBs, hUt], where hBs is assumed to be = 10 and hUt other height.
 */
std::tuple<double, double>
GetBsUtHeightsUmiStreetCanyon(double heightA, double heightB)
{
    double hBs = (heightA == 10) ? heightA : heightB;
    double hUt = (heightA == 10) ? heightB : heightA;
    return std::make_tuple(hBs, hUt);
};

/**
 * @brief Computes the free-space path loss using the formula described in 3GPP TR 38.811,
 * Table 6.6.2
 *
 * @param freq the operating frequency
 * @param dist3d the 3D distance between the communicating nodes
 *
 * @return the path loss for NTN scenarios
 */
double
ComputeNtnPathloss(double freq, double dist3d)
{
    return 32.45 + 20 * log10(freq / 1e9) + 20 * log10(dist3d);
};

/**
 * @brief Computes the atmospheric absorption loss using the formula described in 3GPP TR 38.811,
 * Sec 6.6.4
 *
 * @param freq the operating frequency
 * @param elevAngle the elevation angle between the communicating nodes
 *
 * @return the atmospheric absorption loss for NTN scenarios
 */
double
ComputeAtmosphericAbsorptionLoss(double freq, double elevAngle)
{
    double loss = 0;
    if ((elevAngle < 10 && freq > 1e9) || freq >= 10e9)
    {
        int roundedFreq = round(freq / 10e8);
        loss += atmosphericAbsorption[roundedFreq] / sin(elevAngle * (M_PI / 180));
    }

    return loss;
};

/**
 * @brief Computes the ionospheric plus tropospheric scintillation loss using the formulas
 * described in 3GPP TR 38.811, Sec 6.6.6.1-4 and 6.6.6.2, respectively.
 *
 * @param freq the operating frequency
 * @param elevAngleQuantized the quantized elevation angle between the communicating nodes
 *
 * @return the ionospheric plus tropospheric scintillation loss for NTN scenarios
 */
double
ComputeIonosphericPlusTroposphericScintillationLoss(double freq, double elevAngleQuantized)
{
    double loss = 0;
    if (freq < 6e9)
    {
        // Ionospheric
        loss = 6.22 / (pow(freq / 1e9, 1.5));
    }
    else
    {
        // Tropospheric
        loss = troposphericScintillationLoss.at(elevAngleQuantized);
    }
    return loss;
};

/**
 * @brief Computes the clutter loss using the formula
 * described in 3GPP TR 38.811, Sec 6.6.6.1-4 and 6.6.6.2, respectively.
 *
 * @param freq the operating frequency
 * @param elevAngleQuantized the quantized elevation angle between the communicating nodes
 * @param sfcl the nested map containing the Shadow Fading and
 *         Clutter Loss values for the NTN Suburban and Rural scenario
 *
 * @return the clutter loss for NTN scenarios
 */
double
ComputeClutterLoss(double freq,
                   const std::map<int, std::vector<float>>* sfcl,
                   double elevAngleQuantized)
{
    double loss = 0;
    if (freq < 13.0e9)
    {
        loss += (*sfcl).at(elevAngleQuantized)[SFCL_params::S_NLOS_CL]; // Get the Clutter Loss for
                                                                        // the S Band
    }
    else
    {
        loss += (*sfcl).at(elevAngleQuantized)[SFCL_params::Ka_NLOS_CL]; // Get the Clutter Loss for
                                                                         // the Ka Band
    }

    return loss;
};

constexpr double M_C = 3.0e8; //!< propagation velocity in free space

} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ThreeGppPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(ThreeGppPropagationLossModel);

TypeId
ThreeGppPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppPropagationLossModel")
            .SetParent<PropagationLossModel>()
            .SetGroupName("Propagation")
            .AddAttribute("Frequency",
                          "The centre frequency in Hz.",
                          DoubleValue(500.0e6),
                          MakeDoubleAccessor(&ThreeGppPropagationLossModel::SetFrequency,
                                             &ThreeGppPropagationLossModel::GetFrequency),
                          MakeDoubleChecker<double>())
            .AddAttribute("ShadowingEnabled",
                          "Enable/disable shadowing.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ThreeGppPropagationLossModel::m_shadowingEnabled),
                          MakeBooleanChecker())
            .AddAttribute(
                "ChannelConditionModel",
                "Pointer to the channel condition model.",
                PointerValue(),
                MakePointerAccessor(&ThreeGppPropagationLossModel::SetChannelConditionModel,
                                    &ThreeGppPropagationLossModel::GetChannelConditionModel),
                MakePointerChecker<ChannelConditionModel>())
            .AddAttribute("EnforceParameterRanges",
                          "Whether to strictly enforce TR38.901 applicability ranges",
                          BooleanValue(false),
                          MakeBooleanAccessor(&ThreeGppPropagationLossModel::m_enforceRanges),
                          MakeBooleanChecker())
            .AddAttribute(
                "BuildingPenetrationLossesEnabled",
                "Enable/disable Building Penetration Losses.",
                BooleanValue(true),
                MakeBooleanAccessor(&ThreeGppPropagationLossModel::m_buildingPenLossesEnabled),
                MakeBooleanChecker());
    return tid;
}

ThreeGppPropagationLossModel::ThreeGppPropagationLossModel()
    : PropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    // initialize the normal random variables
    m_normRandomVariable = CreateObject<NormalRandomVariable>();
    m_normRandomVariable->SetAttribute("Mean", DoubleValue(0));
    m_normRandomVariable->SetAttribute("Variance", DoubleValue(1));

    m_randomO2iVar1 = CreateObject<UniformRandomVariable>();
    m_randomO2iVar2 = CreateObject<UniformRandomVariable>();

    m_normalO2iLowLossVar = CreateObject<NormalRandomVariable>();
    m_normalO2iLowLossVar->SetAttribute("Mean", DoubleValue(0));
    m_normalO2iLowLossVar->SetAttribute("Variance", DoubleValue(4.4));

    m_normalO2iHighLossVar = CreateObject<NormalRandomVariable>();
    m_normalO2iHighLossVar->SetAttribute("Mean", DoubleValue(0));
    m_normalO2iHighLossVar->SetAttribute("Variance", DoubleValue(6.5));
}

ThreeGppPropagationLossModel::~ThreeGppPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

void
ThreeGppPropagationLossModel::DoDispose()
{
    m_channelConditionModel->Dispose();
    m_channelConditionModel = nullptr;
    m_shadowingMap.clear();
}

void
ThreeGppPropagationLossModel::SetChannelConditionModel(Ptr<ChannelConditionModel> model)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = model;
}

Ptr<ChannelConditionModel>
ThreeGppPropagationLossModel::GetChannelConditionModel() const
{
    NS_LOG_FUNCTION(this);
    return m_channelConditionModel;
}

void
ThreeGppPropagationLossModel::SetFrequency(double f)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(f >= 500.0e6 && f <= 100.0e9,
                  "Frequency should be between 0.5 and 100 GHz but is " << f);
    m_frequency = f;
}

double
ThreeGppPropagationLossModel::GetFrequency() const
{
    NS_LOG_FUNCTION(this);
    return m_frequency;
}

bool
ThreeGppPropagationLossModel::IsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond) const
{
    return DoIsO2iLowPenetrationLoss(cond);
}

double
ThreeGppPropagationLossModel::DoCalcRxPower(double txPowerDbm,
                                            Ptr<MobilityModel> a,
                                            Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    // check if the model is initialized
    NS_ASSERT_MSG(m_frequency != 0.0, "First set the centre frequency");

    // retrieve the channel condition
    NS_ASSERT_MSG(m_channelConditionModel, "First set the channel condition model");
    Ptr<ChannelCondition> cond = m_channelConditionModel->GetChannelCondition(a, b);

    double rxPow = txPowerDbm;
    rxPow -= GetLoss(cond, a, b);

    if (m_shadowingEnabled)
    {
        rxPow -= GetShadowing(a, b, cond->GetLosCondition());
    }

    // get o2i losses
    if (m_buildingPenLossesEnabled &&
        ((cond->GetO2iCondition() == ChannelCondition::O2iConditionValue::O2I) ||
         (cond->GetO2iCondition() == ChannelCondition::O2iConditionValue::I2I &&
          cond->GetLosCondition() == ChannelCondition::LosConditionValue::NLOS)))
    {
        if (IsO2iLowPenetrationLoss(cond))
        {
            rxPow -= GetO2iLowPenetrationLoss(a, b, cond->GetLosCondition());
        }
        else
        {
            rxPow -= GetO2iHighPenetrationLoss(a, b, cond->GetLosCondition());
        }
    }

    return rxPow;
}

double
ThreeGppPropagationLossModel::GetLoss(Ptr<ChannelCondition> cond,
                                      Ptr<MobilityModel> a,
                                      Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    double loss = 0;
    switch (cond->GetLosCondition())
    {
    case ChannelCondition::LosConditionValue::LOS:
        loss = GetLossLos(a, b);
        break;
    case ChannelCondition::LosConditionValue::NLOSv:
        loss = GetLossNlosv(a, b);
        break;
    case ChannelCondition::LosConditionValue::NLOS:
        loss = GetLossNlos(a, b);
        break;
    default:
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return loss;
}

double
ThreeGppPropagationLossModel::GetO2iLowPenetrationLoss(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);

    double o2iLossValue = 0;
    double lowLossTw = 0;
    double lossIn = 0;
    double lowlossNormalVariate = 0;
    double lGlass = 0;
    double lConcrete = 0;

    // compute the channel key
    uint32_t key = GetKey(a, b);

    bool notFound = false;     // indicates if the o2iLoss value has not been computed yet
    bool newCondition = false; // indicates if the channel condition has changed

    auto it = m_o2iLossMap.end(); // the o2iLoss map iterator
    if (m_o2iLossMap.find(key) != m_o2iLossMap.end())
    {
        // found the o2iLoss value in the map
        it = m_o2iLossMap.find(key);
        newCondition = (it->second.m_condition != cond); // true if the condition changed
    }
    else
    {
        notFound = true;
        // add a new entry in the map and update the iterator
        O2iLossMapItem newItem;
        it = m_o2iLossMap.insert(it, std::make_pair(key, newItem));
    }

    if (notFound || newCondition)
    {
        // distance2dIn is minimum of two independently generated uniformly distributed
        // variables between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and
        // 10 m for RMa. 2D−in d shall be UT-specifically generated.
        double distance2dIn = GetO2iDistance2dIn();

        // calculate material penetration losses, see TR 38.901 Table 7.4.3-1
        lGlass = 2 + 0.2 * m_frequency / 1e9; // m_frequency is operation frequency in Hz
        lConcrete = 5 + 4 * m_frequency / 1e9;

        lowLossTw =
            5 - 10 * log10(0.3 * std::pow(10, -lGlass / 10) + 0.7 * std::pow(10, -lConcrete / 10));

        // calculate indoor loss
        lossIn = 0.5 * distance2dIn;

        // calculate low loss standard deviation
        lowlossNormalVariate = m_normalO2iLowLossVar->GetValue();

        o2iLossValue = lowLossTw + lossIn + lowlossNormalVariate;
    }
    else
    {
        o2iLossValue = it->second.m_o2iLoss;
    }

    // update the entry in the map
    it->second.m_o2iLoss = o2iLossValue;
    it->second.m_condition = cond;

    return o2iLossValue;
}

double
ThreeGppPropagationLossModel::GetO2iHighPenetrationLoss(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);

    double o2iLossValue = 0;
    double highLossTw = 0;
    double lossIn = 0;
    double highlossNormalVariate = 0;
    double lIIRGlass = 0;
    double lConcrete = 0;

    // compute the channel key
    uint32_t key = GetKey(a, b);

    bool notFound = false;     // indicates if the o2iLoss value has not been computed yet
    bool newCondition = false; // indicates if the channel condition has changed

    auto it = m_o2iLossMap.end(); // the o2iLoss map iterator
    if (m_o2iLossMap.find(key) != m_o2iLossMap.end())
    {
        // found the o2iLoss value in the map
        it = m_o2iLossMap.find(key);
        newCondition = (it->second.m_condition != cond); // true if the condition changed
    }
    else
    {
        notFound = true;
        // add a new entry in the map and update the iterator
        O2iLossMapItem newItem;
        it = m_o2iLossMap.insert(it, std::make_pair(key, newItem));
    }

    if (notFound || newCondition)
    {
        // generate a new independent realization

        // distance2dIn is minimum of two independently generated uniformly distributed
        // variables between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and
        // 10 m for RMa. 2D−in d shall be UT-specifically generated.
        double distance2dIn = GetO2iDistance2dIn();

        // calculate material penetration losses, see TR 38.901 Table 7.4.3-1
        lIIRGlass = 23 + 0.3 * m_frequency / 1e9;
        lConcrete = 5 + 4 * m_frequency / 1e9;

        highLossTw = 5 - 10 * log10(0.7 * std::pow(10, -lIIRGlass / 10) +
                                    0.3 * std::pow(10, -lConcrete / 10));

        // calculate indoor loss
        lossIn = 0.5 * distance2dIn;

        // calculate low loss standard deviation
        highlossNormalVariate = m_normalO2iHighLossVar->GetValue();

        o2iLossValue = highLossTw + lossIn + highlossNormalVariate;
    }
    else
    {
        o2iLossValue = it->second.m_o2iLoss;
    }

    // update the entry in the map
    it->second.m_o2iLoss = o2iLossValue;
    it->second.m_condition = cond;

    return o2iLossValue;
}

bool
ThreeGppPropagationLossModel::DoIsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond) const
{
    if (cond->GetO2iLowHighCondition() == ChannelCondition::O2iLowHighConditionValue::LOW)
    {
        return true;
    }
    else if (cond->GetO2iLowHighCondition() == ChannelCondition::O2iLowHighConditionValue::HIGH)
    {
        return false;
    }
    else
    {
        NS_ABORT_MSG("If we have set the O2I condition, we shouldn't be here");
    }
}

double
ThreeGppPropagationLossModel::GetLossNlosv(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_FATAL_ERROR("Unsupported channel condition (NLOSv)");
    return 0;
}

double
ThreeGppPropagationLossModel::GetShadowing(Ptr<MobilityModel> a,
                                           Ptr<MobilityModel> b,
                                           ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);

    double shadowingValue;

    // compute the channel key
    uint32_t key = GetKey(a, b);

    bool notFound = false;          // indicates if the shadowing value has not been computed yet
    bool newCondition = false;      // indicates if the channel condition has changed
    Vector newDistance;             // the distance vector, that is not a distance but a difference
    auto it = m_shadowingMap.end(); // the shadowing map iterator
    if (m_shadowingMap.find(key) != m_shadowingMap.end())
    {
        // found the shadowing value in the map
        it = m_shadowingMap.find(key);
        newDistance = GetVectorDifference(a, b);
        newCondition = (it->second.m_condition != cond); // true if the condition changed
    }
    else
    {
        notFound = true;

        // add a new entry in the map and update the iterator
        ShadowingMapItem newItem;
        it = m_shadowingMap.insert(it, std::make_pair(key, newItem));
    }

    if (notFound || newCondition)
    {
        // generate a new independent realization
        shadowingValue = m_normRandomVariable->GetValue() * GetShadowingStd(a, b, cond);
    }
    else
    {
        // compute a new correlated shadowing loss
        Vector2D displacement(newDistance.x - it->second.m_distance.x,
                              newDistance.y - it->second.m_distance.y);
        double R = exp(-1 * displacement.GetLength() / GetShadowingCorrelationDistance(cond));
        shadowingValue = R * it->second.m_shadowing + sqrt(1 - R * R) *
                                                          m_normRandomVariable->GetValue() *
                                                          GetShadowingStd(a, b, cond);
    }

    // update the entry in the map
    it->second.m_shadowing = shadowingValue;
    it->second.m_distance = newDistance; // Save the (0,0,0) vector in case it's the first time we
                                         // are calculating this value
    it->second.m_condition = cond;

    return shadowingValue;
}

int64_t
ThreeGppPropagationLossModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this);

    m_normRandomVariable->SetStream(stream);
    m_randomO2iVar1->SetStream(stream + 1);
    m_randomO2iVar2->SetStream(stream + 2);
    m_normalO2iLowLossVar->SetStream(stream + 3);
    m_normalO2iHighLossVar->SetStream(stream + 4);

    return 5;
}

double
ThreeGppPropagationLossModel::Calculate2dDistance(Vector a, Vector b)
{
    double x = a.x - b.x;
    double y = a.y - b.y;
    double distance2D = sqrt(x * x + y * y);

    return distance2D;
}

uint32_t
ThreeGppPropagationLossModel::GetKey(Ptr<MobilityModel> a, Ptr<MobilityModel> b)
{
    // use the nodes ids to obtain an unique key for the channel between a and b
    // sort the nodes ids so that the key is reciprocal
    uint32_t x1 = std::min(a->GetObject<Node>()->GetId(), b->GetObject<Node>()->GetId());
    uint32_t x2 = std::max(a->GetObject<Node>()->GetId(), b->GetObject<Node>()->GetId());

    // use the cantor function to obtain the key
    uint32_t key = (((x1 + x2) * (x1 + x2 + 1)) / 2) + x2;

    return key;
}

Vector
ThreeGppPropagationLossModel::GetVectorDifference(Ptr<MobilityModel> a, Ptr<MobilityModel> b)
{
    uint32_t x1 = a->GetObject<Node>()->GetId();
    uint32_t x2 = b->GetObject<Node>()->GetId();

    if (x1 < x2)
    {
        return b->GetPosition() - a->GetPosition();
    }
    else
    {
        return a->GetPosition() - b->GetPosition();
    }
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppRmaPropagationLossModel);

TypeId
ThreeGppRmaPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppRmaPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppRmaPropagationLossModel>()
                            .AddAttribute("AvgBuildingHeight",
                                          "The average building height in meters.",
                                          DoubleValue(5.0),
                                          MakeDoubleAccessor(&ThreeGppRmaPropagationLossModel::m_h),
                                          MakeDoubleChecker<double>(5.0, 50.0))
                            .AddAttribute("AvgStreetWidth",
                                          "The average street width in meters.",
                                          DoubleValue(20.0),
                                          MakeDoubleAccessor(&ThreeGppRmaPropagationLossModel::m_w),
                                          MakeDoubleChecker<double>(5.0, 50.0));
    return tid;
}

ThreeGppRmaPropagationLossModel::ThreeGppRmaPropagationLossModel()
    : ThreeGppPropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    // set a default channel condition model
    m_channelConditionModel = CreateObject<ThreeGppRmaChannelConditionModel>();
}

ThreeGppRmaPropagationLossModel::~ThreeGppRmaPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppRmaPropagationLossModel::GetO2iDistance2dIn() const
{
    // distance2dIn is minimum of two independently generated uniformly distributed variables
    // between 0 and 10 m for RMa. 2D−in d shall be UT-specifically generated.
    return std::min(m_randomO2iVar1->GetValue(0, 10), m_randomO2iVar2->GetValue(0, 10));
}

bool
ThreeGppRmaPropagationLossModel::DoIsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond
                                                           [[maybe_unused]]) const
{
    // Based on 3GPP 38.901 7.4.3.1 in RMa only low losses are applied.
    // Therefore enforce low losses.
    return true;
}

double
ThreeGppRmaPropagationLossModel::GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 30.0e9,
                  "RMa scenario is valid for frequencies between 0.5 and 30 GHz.");

    auto [distance2D, distance3D, hBs, hUt] = GetBsUtDistancesAndHeights(a, b);

    // check if hBS and hUT are within the specified validity range
    if (hUt < 1.0 || hUt > 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Rma UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1 and 10 m (see TR 38.901, Table 7.4.1-1)");
    }

    if (hBs < 10.0 || hBs > 150.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Rma BS height out of range");
        NS_LOG_WARN(
            "The height of the BS should be between 10 and 150 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    double distanceBp = GetBpDistance(m_frequency, hBs, hUt);
    NS_LOG_DEBUG("breakpoint distance " << distanceBp);
    NS_ABORT_MSG_UNLESS(
        distanceBp > 0,
        "Breakpoint distance is zero (divide-by-zero below); are either hBs or hUt = 0?");

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 10.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Rma distance2D out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss (see 3GPP TR 38.901, Table 7.4.1-1)
    double loss = 0;
    if (distance2D <= distanceBp)
    {
        // use PL1
        loss = Pl1(m_frequency, distance3D, m_h, m_w);
    }
    else
    {
        // use PL2
        loss = Pl1(m_frequency, distanceBp, m_h, m_w) + 40 * log10(distance3D / distanceBp);
    }

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppRmaPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 30.0e9,
                  "RMa scenario is valid for frequencies between 0.5 and 30 GHz.");

    auto [distance2D, distance3D, hBs, hUt] = GetBsUtDistancesAndHeights(a, b);

    // check if hBs and hUt are within the validity range
    if (hUt < 1.0 || hUt > 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Rma UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1 and 10 m (see TR 38.901, Table 7.4.1-1)");
    }

    if (hBs < 10.0 || hBs > 150.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Rma BS height out of range");
        NS_LOG_WARN(
            "The height of the BS should be between 10 and 150 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 5.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "distance2D out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss
    double plNlos = 161.04 - 7.1 * log10(m_w) + 7.5 * log10(m_h) -
                    (24.37 - 3.7 * pow((m_h / hBs), 2)) * log10(hBs) +
                    (43.42 - 3.1 * log10(hBs)) * (log10(distance3D) - 3.0) +
                    20.0 * log10(m_frequency / 1e9) - (3.2 * pow(log10(11.75 * hUt), 2) - 4.97);

    double loss = std::max(GetLossLos(a, b), plNlos);

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppRmaPropagationLossModel::GetShadowingStd(Ptr<MobilityModel> a,
                                                 Ptr<MobilityModel> b,
                                                 ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        // compute the 2D distance between the two nodes
        double distance2d = Calculate2dDistance(a->GetPosition(), b->GetPosition());

        // compute the breakpoint distance (see 3GPP TR 38.901, Table 7.4.1-1, note 5)
        double distanceBp = GetBpDistance(m_frequency, a->GetPosition().z, b->GetPosition().z);

        if (distance2d <= distanceBp)
        {
            shadowingStd = 4.0;
        }
        else
        {
            shadowingStd = 6.0;
        }
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        shadowingStd = 8.0;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppRmaPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.901, Table 7.5-6
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 120;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

double
ThreeGppRmaPropagationLossModel::Pl1(double frequency, double distance3D, double h, double /* w */)
{
    double loss = 20.0 * log10(40.0 * M_PI * distance3D * frequency / 1e9 / 3.0) +
                  std::min(0.03 * pow(h, 1.72), 10.0) * log10(distance3D) -
                  std::min(0.044 * pow(h, 1.72), 14.77) + 0.002 * log10(h) * distance3D;
    return loss;
}

double
ThreeGppRmaPropagationLossModel::GetBpDistance(double frequency, double hA, double hB)
{
    double distanceBp = 2.0 * M_PI * hA * hB * frequency / M_C;
    return distanceBp;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppUmaPropagationLossModel);

TypeId
ThreeGppUmaPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppUmaPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppUmaPropagationLossModel>();
    return tid;
}

ThreeGppUmaPropagationLossModel::ThreeGppUmaPropagationLossModel()
    : ThreeGppPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
    m_uniformVar = CreateObject<UniformRandomVariable>();
    // set a default channel condition model
    m_channelConditionModel = CreateObject<ThreeGppUmaChannelConditionModel>();
}

ThreeGppUmaPropagationLossModel::~ThreeGppUmaPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppUmaPropagationLossModel::GetBpDistance(double hUt, double hBs, double distance2D) const
{
    NS_LOG_FUNCTION(this);

    // compute g (d2D) (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double g = 0.0;
    if (distance2D > 18.0)
    {
        g = 5.0 / 4.0 * pow(distance2D / 100.0, 3) * exp(-distance2D / 150.0);
    }

    // compute C (hUt, d2D) (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double c = 0.0;
    if (hUt >= 13.0)
    {
        c = pow((hUt - 13.0) / 10.0, 1.5) * g;
    }

    // compute hE (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double prob = 1.0 / (1.0 + c);
    double hE = 0.0;
    if (m_uniformVar->GetValue() < prob)
    {
        hE = 1.0;
    }
    else
    {
        int random = m_uniformVar->GetInteger(12, std::max(12, (int)(hUt - 1.5)));
        hE = (double)floor(random / 3.0) * 3.0;
    }

    // compute dBP' (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double distanceBp = 4 * (hBs - hE) * (hUt - hE) * m_frequency / M_C;

    return distanceBp;
}

double
ThreeGppUmaPropagationLossModel::GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    auto [distance2D, distance3D, hBs, hUt] = GetBsUtDistancesAndHeights(a, b);

    // check if hBS and hUT are within the validity range
    if (hUt < 1.5 || hUt > 22.5)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1)");
    }

    if (hBs != 25.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma BS height out of range");
        NS_LOG_WARN("The height of the BS should be equal to 25 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    // compute the breakpoint distance (see 3GPP TR 38.901, Table 7.4.1-1, note 1)
    double distanceBp = GetBpDistance(hUt, hBs, distance2D);
    NS_LOG_DEBUG("breakpoint distance " << distanceBp);

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 5.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma 2D distance out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss (see 3GPP TR 38.901, Table 7.4.1-1)
    double loss = 0;
    if (distance2D <= distanceBp)
    {
        // use PL1
        loss = 28.0 + 22.0 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9);
    }
    else
    {
        // use PL2
        loss = 28.0 + 40.0 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9) -
               9.0 * log10(pow(distanceBp, 2) + pow(hBs - hUt, 2));
    }

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppUmaPropagationLossModel::GetO2iDistance2dIn() const
{
    // distance2dIn is minimum of two independently generated uniformly distributed variables
    // between 0 and 25 m for UMa and UMi-Street Canyon. 2D−in d shall be UT-specifically generated.
    return std::min(m_randomO2iVar1->GetValue(0, 25), m_randomO2iVar2->GetValue(0, 25));
}

double
ThreeGppUmaPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    auto [distance2D, distance3D, hBs, hUt] = GetBsUtDistancesAndHeights(a, b);

    // check if hBS and hUT are within the validity range
    if (hUt < 1.5 || hUt > 22.5)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1)");
    }

    if (hBs != 25.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma BS height out of range");
        NS_LOG_WARN("The height of the BS should be equal to 25 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 5.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "Uma 2D distance out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss
    double plNlos =
        13.54 + 39.08 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9) - 0.6 * (hUt - 1.5);
    double loss = std::max(GetLossLos(a, b), plNlos);
    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppUmaPropagationLossModel::GetShadowingStd(Ptr<MobilityModel> /* a */,
                                                 Ptr<MobilityModel> /* b */,
                                                 ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        shadowingStd = 4.0;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        shadowingStd = 6.0;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppUmaPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.901, Table 7.5-6
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 50;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

int64_t
ThreeGppUmaPropagationLossModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this);

    m_normRandomVariable->SetStream(stream);
    m_uniformVar->SetStream(stream + 1);
    return 2;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppUmiStreetCanyonPropagationLossModel);

TypeId
ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppUmiStreetCanyonPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppUmiStreetCanyonPropagationLossModel>();
    return tid;
}

ThreeGppUmiStreetCanyonPropagationLossModel::ThreeGppUmiStreetCanyonPropagationLossModel()
    : ThreeGppPropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    // set a default channel condition model
    m_channelConditionModel = CreateObject<ThreeGppUmiStreetCanyonChannelConditionModel>();
}

ThreeGppUmiStreetCanyonPropagationLossModel::~ThreeGppUmiStreetCanyonPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetBpDistance(double hUt,
                                                           double hBs,
                                                           double /* distance2D */) const
{
    NS_LOG_FUNCTION(this);

    // compute hE (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double hE = 1.0;

    // compute dBP' (see 3GPP TR 38.901, Table 7.4.1-1, Note 1)
    double distanceBp = 4 * (hBs - hE) * (hUt - hE) * m_frequency / M_C;

    return distanceBp;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetO2iDistance2dIn() const
{
    // distance2dIn is minimum of two independently generated uniformly distributed variables
    // between 0 and 25 m for UMa and UMi-Street Canyon. 2D−in d shall be UT-specifically generated.
    return std::min(m_randomO2iVar1->GetValue(0, 25), m_randomO2iVar2->GetValue(0, 25));
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetLossLos(Ptr<MobilityModel> a,
                                                        Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    double distance2D = Calculate2dDistance(a->GetPosition(), b->GetPosition());
    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [hBs, hUt] = GetBsUtHeightsUmiStreetCanyon(a->GetPosition().z, b->GetPosition().z);

    // check if hBS and hUT are within the validity range
    if (hUt < 1.5 || hUt >= 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1). "
            "We further assume hUT < hBS, then hUT is upper bounded by hBS, which should be 10 m");
    }

    if (hBs != 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon BS height out of range");
        NS_LOG_WARN("The height of the BS should be equal to 10 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    // compute the breakpoint distance (see 3GPP TR 38.901, Table 7.4.1-1, note 1)
    double distanceBp = GetBpDistance(hUt, hBs, distance2D);
    NS_LOG_DEBUG("breakpoint distance " << distanceBp);

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 5.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon 2D distance out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss (see 3GPP TR 38.901, Table 7.4.1-1)
    double loss = 0;
    if (distance2D <= distanceBp)
    {
        // use PL1
        loss = 32.4 + 21.0 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9);
    }
    else
    {
        // use PL2
        loss = 32.4 + 40.0 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9) -
               9.5 * log10(pow(distanceBp, 2) + pow(hBs - hUt, 2));
    }

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a,
                                                         Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    double distance2D = Calculate2dDistance(a->GetPosition(), b->GetPosition());
    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [hBs, hUt] = GetBsUtHeightsUmiStreetCanyon(a->GetPosition().z, b->GetPosition().z);

    // check if hBS and hUT are within the validity range
    if (hUt < 1.5 || hUt >= 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon UT height out of range");
        NS_LOG_WARN(
            "The height of the UT should be between 1.5 and 22.5 m (see TR 38.901, Table 7.4.1-1). "
            "We further assume hUT < hBS, then hUT is upper bounded by hBS, which should be 10 m");
    }

    if (hBs != 10.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon BS height out of range");
        NS_LOG_WARN("The height of the BS should be equal to 10 m (see TR 38.901, Table 7.4.1-1)");
    }

    // NOTE The model is intended to be used for BS-UT links, however we may need to
    // compute the pathloss between two BSs or UTs, e.g., to evaluate the
    // interference. In order to apply the model, we need to retrieve the values of
    // hBS and hUT, but in these cases one of the two falls outside the validity
    // range and the warning message is printed (hBS for the UT-UT case and hUT
    // for the BS-BS case).

    // check if the distance is outside the validity range
    if (distance2D < 10.0 || distance2D > 5.0e3)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "UmiStreetCanyon 2D distance out of range");
        NS_LOG_WARN("The 2D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss
    double plNlos =
        22.4 + 35.3 * log10(distance3D) + 21.3 * log10(m_frequency / 1e9) - 0.3 * (hUt - 1.5);
    double loss = std::max(GetLossLos(a, b), plNlos);
    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> /* a */,
    Ptr<MobilityModel> /* b */,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        shadowingStd = 4.0;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        shadowingStd = 7.82;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppUmiStreetCanyonPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.901, Table 7.5-6
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 10;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 13;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppIndoorOfficePropagationLossModel);

TypeId
ThreeGppIndoorOfficePropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppIndoorOfficePropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppIndoorOfficePropagationLossModel>();
    return tid;
}

ThreeGppIndoorOfficePropagationLossModel::ThreeGppIndoorOfficePropagationLossModel()
    : ThreeGppPropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    // set a default channel condition model
    m_channelConditionModel = CreateObject<ThreeGppIndoorOpenOfficeChannelConditionModel>();
}

ThreeGppIndoorOfficePropagationLossModel::~ThreeGppIndoorOfficePropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppIndoorOfficePropagationLossModel::GetO2iDistance2dIn() const
{
    return 0;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetLossLos(Ptr<MobilityModel> a,
                                                     Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());

    // check if the distance is outside the validity range
    if (distance3D < 1.0 || distance3D > 150.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "IndoorOffice 3D distance out of range");
        NS_LOG_WARN("The 3D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss (see 3GPP TR 38.901, Table 7.4.1-1)
    double loss = 32.4 + 17.3 * log10(distance3D) + 20.0 * log10(m_frequency / 1e9);

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetLossNlos(Ptr<MobilityModel> a,
                                                      Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());

    // check if the distance is outside the validity range
    if (distance3D < 1.0 || distance3D > 150.0)
    {
        NS_ABORT_MSG_IF(m_enforceRanges, "IndoorOffice 3D distance out of range");
        NS_LOG_WARN("The 3D distance is outside the validity range, the pathloss value may not be "
                    "accurate");
    }

    // compute the pathloss
    double plNlos = 17.3 + 38.3 * log10(distance3D) + 24.9 * log10(m_frequency / 1e9);
    double loss = std::max(GetLossLos(a, b), plNlos);

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> /* a */,
    Ptr<MobilityModel> /* b */,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        shadowingStd = 3.0;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        shadowingStd = 8.03;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppIndoorOfficePropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);

    // See 3GPP TR 38.901, Table 7.5-6
    double correlationDistance;

    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 10;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 6;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

NS_OBJECT_ENSURE_REGISTERED(ThreeGppNTNDenseUrbanPropagationLossModel);

TypeId
ThreeGppNTNDenseUrbanPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppNTNDenseUrbanPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppNTNDenseUrbanPropagationLossModel>();
    return tid;
}

ThreeGppNTNDenseUrbanPropagationLossModel::ThreeGppNTNDenseUrbanPropagationLossModel()
    : ThreeGppPropagationLossModel(),
      m_SFCL_DenseUrban(&SFCL_DenseUrban)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = CreateObject<ThreeGppNTNDenseUrbanChannelConditionModel>();
}

ThreeGppNTNDenseUrbanPropagationLossModel::~ThreeGppNTNDenseUrbanPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppNTNDenseUrbanPropagationLossModel::GetO2iDistance2dIn() const
{
    abort();
}

double
ThreeGppNTNDenseUrbanPropagationLossModel::GetLossLos(Ptr<MobilityModel> a,
                                                      Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNDenseUrbanPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a,
                                                       Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Clutter Loss
    loss += ComputeClutterLoss(m_frequency, m_SFCL_DenseUrban, elevAngleQuantized);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNDenseUrbanPropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    std::string freqBand = (m_frequency < 13.0e9) ? "S" : "Ka";
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // Assign Shadowing Standard Deviation according to table 6.6.2-1
    if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_DenseUrban).at(elevAngleQuantized)[SFCL_params::S_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_DenseUrban).at(elevAngleQuantized)[SFCL_params::Ka_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_DenseUrban).at(elevAngleQuantized)[SFCL_params::S_NLOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_DenseUrban).at(elevAngleQuantized)[SFCL_params::Ka_NLOS_sigF];
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppNTNDenseUrbanPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.811, Table 6.7.2-1a/b and Table 6.7.2-2a/b
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 50;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppNTNUrbanPropagationLossModel);

TypeId
ThreeGppNTNUrbanPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppNTNUrbanPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppNTNUrbanPropagationLossModel>();
    return tid;
}

ThreeGppNTNUrbanPropagationLossModel::ThreeGppNTNUrbanPropagationLossModel()
    : ThreeGppPropagationLossModel(),
      m_SFCL_Urban(&SFCL_Urban)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = CreateObject<ThreeGppNTNUrbanChannelConditionModel>();
}

ThreeGppNTNUrbanPropagationLossModel::~ThreeGppNTNUrbanPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppNTNUrbanPropagationLossModel::GetO2iDistance2dIn() const
{
    abort();
}

double
ThreeGppNTNUrbanPropagationLossModel::GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNUrbanPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Clutter Loss
    loss += ComputeClutterLoss(m_frequency, m_SFCL_Urban, elevAngleQuantized);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNUrbanPropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    std::string freqBand = (m_frequency < 13.0e9) ? "S" : "Ka";
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // Assign Shadowing Standard Deviation according to table 6.6.2-1
    if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_Urban).at(elevAngleQuantized)[SFCL_params::S_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_Urban).at(elevAngleQuantized)[SFCL_params::Ka_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_Urban).at(elevAngleQuantized)[SFCL_params::S_NLOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_Urban).at(elevAngleQuantized)[SFCL_params::Ka_NLOS_sigF];
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppNTNUrbanPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.811, Table 6.7.2-3a/b and Table 6.7.2-3a/b
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 50;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppNTNSuburbanPropagationLossModel);

TypeId
ThreeGppNTNSuburbanPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppNTNSuburbanPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppNTNSuburbanPropagationLossModel>();
    return tid;
}

ThreeGppNTNSuburbanPropagationLossModel::ThreeGppNTNSuburbanPropagationLossModel()
    : ThreeGppPropagationLossModel(),
      m_SFCL_SuburbanRural(&SFCL_SuburbanRural)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = CreateObject<ThreeGppNTNSuburbanChannelConditionModel>();
}

ThreeGppNTNSuburbanPropagationLossModel::~ThreeGppNTNSuburbanPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppNTNSuburbanPropagationLossModel::GetO2iDistance2dIn() const
{
    abort();
}

double
ThreeGppNTNSuburbanPropagationLossModel::GetLossLos(Ptr<MobilityModel> a,
                                                    Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);

    return loss;
}

double
ThreeGppNTNSuburbanPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a,
                                                     Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Clutter Loss
    loss += ComputeClutterLoss(m_frequency, m_SFCL_SuburbanRural, elevAngleQuantized);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNSuburbanPropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    std::string freqBand = (m_frequency < 13.0e9) ? "S" : "Ka";
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // Assign Shadowing Standard Deviation according to table 6.6.2-1
    if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::S_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::Ka_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::S_NLOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::Ka_NLOS_sigF];
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppNTNSuburbanPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.811, Table 6.7.2-5a/b and Table 6.7.2-6a/b
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 50;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED(ThreeGppNTNRuralPropagationLossModel);

TypeId
ThreeGppNTNRuralPropagationLossModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ThreeGppNTNRuralPropagationLossModel")
                            .SetParent<ThreeGppPropagationLossModel>()
                            .SetGroupName("Propagation")
                            .AddConstructor<ThreeGppNTNRuralPropagationLossModel>();
    return tid;
}

ThreeGppNTNRuralPropagationLossModel::ThreeGppNTNRuralPropagationLossModel()
    : ThreeGppPropagationLossModel(),
      m_SFCL_SuburbanRural(&SFCL_SuburbanRural)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = CreateObject<ThreeGppNTNRuralChannelConditionModel>();
}

ThreeGppNTNRuralPropagationLossModel::~ThreeGppNTNRuralPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

double
ThreeGppNTNRuralPropagationLossModel::GetO2iDistance2dIn() const
{
    abort();
}

double
ThreeGppNTNRuralPropagationLossModel::GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNRuralPropagationLossModel::GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_frequency <= 100.0e9,
                  "NTN communications are valid for frequencies between 0.5 and 100 GHz.");

    double distance3D = CalculateDistance(a->GetPosition(), b->GetPosition());
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // compute the pathloss (see 3GPP TR 38.811, Table 6.6.2)
    double loss = ComputeNtnPathloss(m_frequency, distance3D);

    // Apply Clutter Loss
    loss += ComputeClutterLoss(m_frequency, m_SFCL_SuburbanRural, elevAngleQuantized);

    // Apply Atmospheric Absorption Loss 3GPP 38.811 6.6.4
    loss += ComputeAtmosphericAbsorptionLoss(m_frequency, elevAngle);

    // Apply Ionospheric plus Tropospheric Scintillation Loss
    loss += ComputeIonosphericPlusTroposphericScintillationLoss(m_frequency, elevAngleQuantized);

    NS_LOG_DEBUG("Loss " << loss);
    return loss;
}

double
ThreeGppNTNRuralPropagationLossModel::GetShadowingStd(
    Ptr<MobilityModel> a,
    Ptr<MobilityModel> b,
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double shadowingStd;

    std::string freqBand = (m_frequency < 13.0e9) ? "S" : "Ka";
    auto [elevAngle, elevAngleQuantized] =
        ThreeGppChannelConditionModel::GetQuantizedElevationAngle(a, b);

    // Assign Shadowing Standard Deviation according to table 6.6.2-1
    if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::S_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::LOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::Ka_LOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "S")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::S_NLOS_sigF];
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS && freqBand == "Ka")
    {
        shadowingStd = (*m_SFCL_SuburbanRural).at(elevAngleQuantized)[SFCL_params::Ka_NLOS_sigF];
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return shadowingStd;
}

double
ThreeGppNTNRuralPropagationLossModel::GetShadowingCorrelationDistance(
    ChannelCondition::LosConditionValue cond) const
{
    NS_LOG_FUNCTION(this);
    double correlationDistance;

    // See 3GPP TR 38.811, Table 6.7.2-7a/b and Table 6.7.2-8a/b
    if (cond == ChannelCondition::LosConditionValue::LOS)
    {
        correlationDistance = 37;
    }
    else if (cond == ChannelCondition::LosConditionValue::NLOS)
    {
        correlationDistance = 120;
    }
    else
    {
        NS_FATAL_ERROR("Unknown channel condition");
    }

    return correlationDistance;
}

} // namespace ns3
