/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#ifndef WRAPAROUNDMODEL_H
#define WRAPAROUNDMODEL_H

#include "ns3/mobility-model.h"

namespace ns3
{
class WraparoundModel : public Object
{
  public:
    /**
     * @brief Default constructor
     */
    WraparoundModel() = default;

    /**
     * Register this type with the TypeId system.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a disposable virtual mobility model for tx based on rx distance and
     * a wraparound model
     * @param tx Transmitter mobility model
     * @param rx Receiver Mobility model
     * @return virtual mobility model for transmitter
     */
    Ptr<MobilityModel> GetVirtualMobilityModel(Ptr<const MobilityModel> tx,
                                               Ptr<const MobilityModel> rx) const;
    /**
     * @brief Get virtual position of txPos with respect to rxPos. Each wraparound model
     * should override this function with their own implementation
     * @param tx Transmitter position
     * @param rx Receiver position
     * @return virtual position of transmitter in respect to receiver position
     */
    virtual Vector3D GetVirtualPosition(const Vector3D tx, const Vector3D rx) const;
};
} // namespace ns3

#endif // WRAPAROUNDMODEL_H
