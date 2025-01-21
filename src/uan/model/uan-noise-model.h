/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_NOISE_MODEL_H
#define UAN_NOISE_MODEL_H

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup uan
 *
 * UAN Noise Model base class.
 */
class UanNoiseModel : public Object
{
  public:
    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Compute the noise power at a given frequency.
     *
     * @param fKhz Frequency in kHz.
     * @return Noise power in dB re 1uPa/Hz.
     */
    virtual double GetNoiseDbHz(double fKhz) const = 0;

    /** Clear all pointer references. */
    virtual void Clear();

    void DoDispose() override;
};

} // namespace ns3

#endif /* UAN_NOISE_MODEL_H */
