/*
 * Copyright (c) 2012 Telum (www.telum.ru)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@telum.ru>
 */
#ifndef JAKES_STATIONARY_LOSS_MODEL_H
#define JAKES_STATIONARY_LOSS_MODEL_H

#include "jakes-process.h"
#include "propagation-cache.h"
#include "propagation-loss-model.h"

namespace ns3
{
/**
 * @ingroup propagation
 *
 * @brief a  Jakes narrowband propagation model.
 * Symmetrical cache for JakesProcess
 */

class JakesPropagationLossModel : public PropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    JakesPropagationLossModel();
    ~JakesPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    JakesPropagationLossModel(const JakesPropagationLossModel&) = delete;
    JakesPropagationLossModel& operator=(const JakesPropagationLossModel&) = delete;

  protected:
    void DoDispose() override;

  private:
    friend class JakesProcess;

    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;

    int64_t DoAssignStreams(int64_t stream) override;

    /**
     * Get the underlying RNG stream
     * @return the RNG stream
     */
    Ptr<UniformRandomVariable> GetUniformRandomVariable() const;

    Ptr<UniformRandomVariable> m_uniformVariable;              //!< random stream
    mutable PropagationCache<JakesProcess> m_propagationCache; //!< Propagation cache
};

} // namespace ns3

#endif /* JAKES_STATIONARY_LOSS_MODEL_H */
