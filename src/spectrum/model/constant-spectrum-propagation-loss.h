/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef CONSTANT_SPECTRUM_PROPAGATION_LOSS_H
#define CONSTANT_SPECTRUM_PROPAGATION_LOSS_H

#include "spectrum-propagation-loss-model.h"

namespace ns3
{

/**
 * @ingroup spectrum
 *
 * A Constant (fixed) propagation loss. The loss is not dependent on the distance.
 */
class ConstantSpectrumPropagationLossModel : public SpectrumPropagationLossModel
{
  public:
    ConstantSpectrumPropagationLossModel();
    ~ConstantSpectrumPropagationLossModel() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                    Ptr<const MobilityModel> a,
                                                    Ptr<const MobilityModel> b) const override;
    /**
     * Set the propagation loss
     * @param lossDb the propagation loss [dB]
     */
    void SetLossDb(double lossDb);
    /**
     * Get the propagation loss
     * @returns the propagation loss [dB]
     */
    double GetLossDb() const;

  protected:
    int64_t DoAssignStreams(int64_t stream) override;
    double m_lossDb;     //!< Propagation loss [dB]
    double m_lossLinear; //!< Propagation loss (linear)

  private:
};

} // namespace ns3

#endif /* CONSTANT_SPECTRUM_PROPAGATION_LOSS_MODEL_H */
