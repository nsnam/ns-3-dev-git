/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_NOISE_MODEL_DEFAULT_H
#define UAN_NOISE_MODEL_DEFAULT_H

#include "uan-noise-model.h"

#include "ns3/attribute.h"
#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup uan
 *
 * Standard ambient acoustic noise model.
 *
 * See attributes for parameters
 *
 * This class returns ambient noise by following the algorithm given in
 * Harris, A. F. and Zorzi, M. 2007. Modeling the underwater acoustic
 * channel in ns2. In Proceedings of the 2nd international Conference
 * on Performance Evaluation Methodologies and Tools (Nantes, France,
 * October 22 - 27, 2007). ValueTools, vol. 321. ICST (Institute for
 * Computer Sciences Social-Informatics and Telecommunications Engineering),
 * ICST, Brussels, Belgium, 1-8.
 *
 * Which uses the noise model also given in the book
 * "Principles of Underwater Sound" by Urick
 */
class UanNoiseModelDefault : public UanNoiseModel
{
  public:
    UanNoiseModelDefault();           //!< Default constructor.
    ~UanNoiseModelDefault() override; //!< Dummy destructor, DoDispose.

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    // Inherited methods
    double GetNoiseDbHz(double fKhz) const override;

  private:
    double m_wind;     //!< Wind speed in m/s.
    double m_shipping; //!< Shipping contribution to noise between 0 and 1.
};

} // namespace ns3

#endif /* UAN_NOISE_MODEL_DEFAULT_H */
