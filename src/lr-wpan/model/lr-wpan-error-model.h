/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */
#ifndef LR_WPAN_ERROR_MODEL_H
#define LR_WPAN_ERROR_MODEL_H

#include "ns3/object.h"

namespace ns3
{
namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * Model the error rate for IEEE 802.15.4 2.4 GHz AWGN channel for OQPSK
 * the model description can be found in IEEE Std 802.15.4-2006, section
 * E.4.1.7
 */
class LrWpanErrorModel : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    LrWpanErrorModel();

    /**
     * Return chunk success rate for given SNR.
     *
     * @return success rate (i.e. 1 - chunk error rate)
     * @param snr SNR expressed as a power ratio (i.e. not in dB)
     * @param nbits number of bits in the chunk
     */
    double GetChunkSuccessRate(double snr, uint32_t nbits) const;

  private:
    /**
     * Array of precalculated binomial coefficients.
     */
    double m_binomialCoefficients[17];
};
} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_ERROR_MODEL_H */
