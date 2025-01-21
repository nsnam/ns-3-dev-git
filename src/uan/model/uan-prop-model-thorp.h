/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_PROP_MODEL_THORP_H
#define UAN_PROP_MODEL_THORP_H

#include "uan-prop-model.h"

namespace ns3
{

class UanTxMode;

/**
 * @ingroup uan
 *
 * Uses Thorp's approximation to compute pathloss.  Assumes implulse PDP.
 */
class UanPropModelThorp : public UanPropModel
{
  public:
    /** Default constructor. */
    UanPropModelThorp();
    /** Destructor */
    ~UanPropModelThorp() override;

    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // Inherited methods
    double GetPathLossDb(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;
    UanPdp GetPdp(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;
    Time GetDelay(Ptr<MobilityModel> a, Ptr<MobilityModel> b, UanTxMode mode) override;

  private:
    /**
     * Get the attenuation in dB / 1000 yards.
     * @param freqKhz The channel center frequency, in kHz.
     * @return The attenuation, in dB / 1000 yards.
     */
    double GetAttenDbKyd(double freqKhz);
    /**
     * Get the attenuation in dB / km.
     * @param freqKhz The channel center frequency, in kHz.
     * @return The attenuation, in dB/km.
     */
    double GetAttenDbKm(double freqKhz);

    double m_SpreadCoef; //!< Spreading coefficient used in calculation of Thorp's approximation.
};

} // namespace ns3

#endif /* UAN_PROP_MODEL_THORP_H */
