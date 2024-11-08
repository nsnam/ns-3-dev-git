/*
 * Copyright (c) 2005,2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 University of Surrey
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Konstantinos Katsaros <dinos.katsaros@gmail.com>
 */

#ifndef SNR_TAG_H
#define SNR_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class Tag;

class SnrTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create a SnrTag with the default SNR 0
     */
    SnrTag();

    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

    /**
     * Set the SNR to the given value.
     *
     * @param snr the value of the SNR to set in linear scale
     */
    void Set(double snr);
    /**
     * Return the SNR value.
     *
     * @return the SNR value in linear scale
     */
    double Get() const;

  private:
    double m_snr; //!< SNR value in linear scale
};

} // namespace ns3

#endif /* SNR_TAG_H */
