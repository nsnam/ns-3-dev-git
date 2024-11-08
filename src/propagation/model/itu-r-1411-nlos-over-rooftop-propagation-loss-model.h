/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *
 */

#ifndef ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H
#define ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H

#include "propagation-environment.h"
#include "propagation-loss-model.h"

namespace ns3
{

/**
 * @ingroup propagation
 *
 * @brief the ITU-R 1411 NLOS over rooftop propagation model
 *
 * This class implements the ITU-R 1411 LOS propagation model for
 * Non-Line-of-Sight (NLoS) short range outdoor communication over
 * rooftops in the frequency range 300 MHz to 100 GHz.
 * For more information about the model, please see
 * the propagation module documentation in .rst format.
 */
class ItuR1411NlosOverRooftopPropagationLossModel : public PropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    ItuR1411NlosOverRooftopPropagationLossModel();
    ~ItuR1411NlosOverRooftopPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ItuR1411NlosOverRooftopPropagationLossModel(
        const ItuR1411NlosOverRooftopPropagationLossModel&) = delete;
    ItuR1411NlosOverRooftopPropagationLossModel& operator=(
        const ItuR1411NlosOverRooftopPropagationLossModel&) = delete;

    /**
     * Set the operating frequency
     *
     * @param freq the frequency in Hz
     */
    void SetFrequency(double freq);

    /**
     *
     *
     * @param a the first mobility model
     * @param b the second mobility model
     *
     * @return the loss in dBm for the propagation between
     * the two given mobility models
     */
    double GetLoss(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

  private:
    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;
    int64_t DoAssignStreams(int64_t stream) override;

    double m_frequency;            //!< frequency in MHz
    double m_lambda;               //!< wavelength
    EnvironmentType m_environment; //!< Environment Scenario
    CitySize m_citySize;           //!< Dimension of the city
    double m_rooftopHeight;        //!< in meters
    double m_streetsOrientation;   //!< in degrees [0,90]
    double m_streetsWidth;         //!< in meters
    double m_buildingsExtend;      //!< in meters
    double m_buildingSeparation;   //!< in meters
};

} // namespace ns3

#endif // ITU_R_1411_NLOS_OVER_ROOFTOP_PROPAGATION_LOSS_MODEL_H
