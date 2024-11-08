/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef FRIIS_SPECTRUM_PROPAGATION_LOSS_H
#define FRIIS_SPECTRUM_PROPAGATION_LOSS_H

#include "spectrum-propagation-loss-model.h"

namespace ns3
{

class MobilityModel;

/**
 * @ingroup spectrum
 * @brief Friis spectrum propagation loss model
 *
 * The propagation loss is calculated according to a simplified version of Friis'
 * formula in which antenna gains are unitary:
 *
 * \f$ L = \frac{4 \pi * d * f}{C^2}\f$
 *
 * where C = 3e8 m/s is the light speed in the vacuum. The intended
 * use is to calculate Prx = Ptx * G
 */
class FriisSpectrumPropagationLossModel : public SpectrumPropagationLossModel
{
  public:
    FriisSpectrumPropagationLossModel();
    ~FriisSpectrumPropagationLossModel() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                    Ptr<const MobilityModel> a,
                                                    Ptr<const MobilityModel> b) const override;

    /**
     * Return the propagation loss L according to a simplified version of Friis'
     * formula in which antenna gains are unitary
     *
     * @param f frequency in Hz
     * @param d distance in m
     *
     * @return if Prx < Ptx then return Prx; else return Ptx
     */
    double CalculateLoss(double f, double d) const;

  protected:
    int64_t DoAssignStreams(int64_t stream) override;
};

} // namespace ns3

#endif /* FRIIS_SPECTRUM_PROPAGATION_LOSS_MODEL_H */
