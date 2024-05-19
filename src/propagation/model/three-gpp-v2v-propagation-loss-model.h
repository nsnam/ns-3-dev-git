/*
 * Copyright (c) 2020 SIGNET Lab, Department of Information Engineering,
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

#ifndef THREE_GPP_V2V_PROPAGATION_LOSS_MODEL_H
#define THREE_GPP_V2V_PROPAGATION_LOSS_MODEL_H

#include "three-gpp-propagation-loss-model.h"

#include "ns3/deprecated.h"
#include <ns3/mobility-model.h>

namespace ns3
{

/**
 * \ingroup propagation
 *
 * \brief Implements the pathloss model defined in 3GPP TR 37.885, Table 6.2.1-1
 *        for the Urban scenario.
 */
class ThreeGppV2vUrbanPropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppV2vUrbanPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppV2vUrbanPropagationLossModel() override;

    ThreeGppV2vUrbanPropagationLossModel(const ThreeGppV2vUrbanPropagationLossModel&) = delete;
    ThreeGppV2vUrbanPropagationLossModel& operator=(const ThreeGppV2vUrbanPropagationLossModel&) =
        delete;

  private:
    /**
     * \brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * \brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     *
     *        TODO O2I car penetration loss (TR 38.901 7.4.3.2) not considered
     *
     * \return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dIn() const override;

    /**
     * \brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed by a vehicle
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \return pathloss value in dB
     */
    double GetLossNlosv(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * \brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed by a building
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \return pathloss value in dB
     */
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * \brief Computes the additional loss due to an obstruction caused by a vehicle
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \return pathloss value in dB
     */
    double GetAdditionalNlosvLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

    /**
     * \brief Returns the shadow fading standard deviation
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \param cond the LOS/NLOS channel condition
     * \return shadowing std in dB
     */
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;

    /**
     * \brief Returns the shadow fading correlation distance
     * \param cond the LOS/NLOS channel condition
     * \return shadowing correlation distance in meters
     */
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;

    int64_t DoAssignStreams(int64_t stream) override;

    double m_percType3Vehicles; //!< percentage of Type 3 vehicles in the scenario (i.e., trucks)
    Ptr<UniformRandomVariable> m_uniformVar;  //!< uniform random variable
    Ptr<LogNormalRandomVariable> m_logNorVar; //!< log normal random variable
};

/**
 * \ingroup propagation
 *
 * \brief Implements the pathloss model defined in 3GPP TR 37.885, Table 6.2.1-1
 *        for the Highway scenario.
 */
class ThreeGppV2vHighwayPropagationLossModel : public ThreeGppV2vUrbanPropagationLossModel
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppV2vHighwayPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppV2vHighwayPropagationLossModel() override;

  private:
    /**
     * \brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * \param a mobility model of one of the two communicating nodes
     * \param b mobility model of one of the two communicating nodes
     * \return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;
};

} // namespace ns3

#endif /* THREE_GPP_V2V_PROPAGATION_LOSS_MODEL_H */
