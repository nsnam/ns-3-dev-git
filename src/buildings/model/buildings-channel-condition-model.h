/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York
 * University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef BUILDINGS_CHANNEL_CONDITION_MODEL_H
#define BUILDINGS_CHANNEL_CONDITION_MODEL_H

#include "ns3/channel-condition-model.h"

namespace ns3
{

class MobilityModel;

/**
 * @ingroup buildings
 * @ingroup propagation
 *
 * @brief Determines the channel condition based on the buildings deployed in the
 * scenario
 *
 * Code adapted from MmWave3gppBuildingsPropagationLossModel
 */
class BuildingsChannelConditionModel : public ChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the BuildingsChannelConditionModel class
     */
    BuildingsChannelConditionModel();

    /**
     * Destructor for the BuildingsChannelConditionModel class
     */
    ~BuildingsChannelConditionModel() override;

    /**
     * Computes the condition of the channel between a and b.
     *
     * @param a mobility model
     * @param b mobility model
     * @return the condition of the channel between a and b
     */
    Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                              Ptr<const MobilityModel> b) const override;

    /**
     * If this model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;

  private:
    /**
     * @brief Checks if the line of sight between position l1 and position l2 is
     *        blocked by a building.
     *
     * @param l1 position
     * @param l2 position
     * @return true if the line of sight is blocked, false otherwise
     */
    bool IsLineOfSightBlocked(const Vector& l1, const Vector& l2) const;
};

} // namespace ns3

#endif /* BUILDINGS_CHANNEL_CONDITION_MODEL_H */
