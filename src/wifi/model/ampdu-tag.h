/*
 * Copyright (c) 2013
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef AMPDU_TAG_H
#define AMPDU_TAG_H

#include "ns3/nstime.h"
#include "ns3/tag.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The aim of the AmpduTag is to provide means for a MAC to specify that a packet includes A-MPDU
 * since this is done in HT-SIG and there is no HT-SIG representation in ns-3
 */
class AmpduTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    /**
     * Create a AmpduTag with the default =0 no A-MPDU
     */
    AmpduTag();
    /**
     * @param nbOfMpdus the remaining number of MPDUs
     *
     * Set the remaining number of MPDUs in the A-MPDU.
     */
    void SetRemainingNbOfMpdus(uint8_t nbOfMpdus);
    /**
     * @param duration the remaining duration of the A-MPDU
     *
     * Set the remaining duration of the A-MPDU.
     */
    void SetRemainingAmpduDuration(Time duration);

    /**
     * @return the remaining number of MPDUs in an A-MPDU
     *
     * Returns the remaining number of MPDUs in an A-MPDU
     */
    uint8_t GetRemainingNbOfMpdus() const;
    /**
     * @return the remaining duration of an A-MPDU
     *
     * Returns the remaining duration of an A-MPDU
     */
    Time GetRemainingAmpduDuration() const;

  private:
    uint8_t m_nbOfMpdus; //!< Remaining number of MPDUs in the A-MPDU
    Time m_duration;     //!< Remaining duration of the A-MPDU
};

} // namespace ns3

#endif /* AMPDU_TAG_H */
