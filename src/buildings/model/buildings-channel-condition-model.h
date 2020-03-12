/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York
 * University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef BUILDINGS_CHANNEL_CONDITION_MODEL_H
#define BUILDINGS_CHANNEL_CONDITION_MODEL_H

#include "ns3/channel-condition-model.h"

namespace ns3 {

class MobilityModel;

/**
 * \ingroup buildings
 *
 * \brief Determines the channel condition based on the buildings deployed in the
 * scenario
 *
 * Code adapted from MmWave3gppBuildingsPropagationLossModel
 */
class BuildingsChannelConditionModel : public ChannelConditionModel
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);


  /**
   * Constructor for the BuildingsChannelConditionModel class
   */
  BuildingsChannelConditionModel ();

  /**
   * Destructor for the BuildingsChannelConditionModel class
   */
  virtual ~BuildingsChannelConditionModel () override;

  /**
   * Computes the condition of the channel between a and b.
   *
   * \param a mobility model
   * \param b mobility model
   * \return the condition of the channel between a and b
   */
  virtual Ptr<ChannelCondition> GetChannelCondition (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;

  /**
   * If this model uses objects of type RandomVariableStream,
   * set the stream numbers to the integers starting with the offset
   * 'stream'. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream) override;

private:
  /**
   * Checks if the is a building in between position L1 and position L2.
   * This function was developed by NYU Wireless and is based on the algorithm
   * described here http://www.3dkingdoms.com/weekly/weekly.php?a=21.
   * Reference: Menglei Zhang, Michele Polese, Marco Mezzavilla, Sundeep Rangan,
   * Michele Zorzi. "ns-3 Implementation of the 3GPP MIMO Channel Model for
   * Frequency Spectrum above 6 GHz". In Proceedings of the Workshop on ns-3
   * (WNS3 '17). 2017.
   *
   * \param l1 position
   * \param l2 position
   * \return true if there is a building in between L1 and L2, false otherwise
   */
  bool IsWithinLineOfSight (const Vector &l1, const Vector &l2) const;
};

} // end ns3 namespace

#endif /* BUILDINGS_CHANNEL_CONDITION_MODEL_H */
