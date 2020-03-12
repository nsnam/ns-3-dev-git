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

#include "ns3/buildings-channel-condition-model.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-building-info.h"
#include "ns3/building-list.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BuildingsChannelConditionModel");

NS_OBJECT_ENSURE_REGISTERED (BuildingsChannelConditionModel);

TypeId
BuildingsChannelConditionModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BuildingsChannelConditionModel")
    .SetParent<ChannelConditionModel> ()
    .SetGroupName ("Buildings")
    .AddConstructor<BuildingsChannelConditionModel> ()
  ;
  return tid;
}

BuildingsChannelConditionModel::BuildingsChannelConditionModel ()
  : ChannelConditionModel ()
{
}

BuildingsChannelConditionModel::~BuildingsChannelConditionModel ()
{
}

Ptr<ChannelCondition>
BuildingsChannelConditionModel::GetChannelCondition (Ptr<const MobilityModel> a,
                                                     Ptr<const MobilityModel> b) const
{
  Ptr<MobilityBuildingInfo> a1 = a->GetObject<MobilityBuildingInfo> ();
  Ptr<MobilityBuildingInfo> b1 = b->GetObject<MobilityBuildingInfo> ();
  NS_ASSERT_MSG ((a1 != nullptr) && (b1 != nullptr),
                 "BuildingsChannelConditionModel only works with MobilityBuildingInfo");

  Ptr<ChannelCondition> cond = CreateObject<ChannelCondition> ();

  bool isAIndoor = a1->IsIndoor ();
  bool isBIndoor = b1->IsIndoor ();


  if (!isAIndoor && !isBIndoor) // a and b are outdoor
    {
      // The outdoor case, determine LOS/NLOS
      // The channel condition should be NLOS if the line intersect one of the buildings, otherwise LOS
      bool intersect = IsWithinLineOfSight (a->GetPosition (), b->GetPosition ());
      if (!intersect)
        {
          cond->SetLosCondition (ChannelCondition::LosConditionValue::LOS);
        }
      else
        {
          cond->SetLosCondition (ChannelCondition::LosConditionValue::NLOS);
        }

    }
  else if (isAIndoor && isBIndoor) // a and b are indoor
    {
      // Indoor case, determine is the two nodes are inside the same building
      // or not
      if (a1->GetBuilding () == b1->GetBuilding ())
        {
          cond->SetLosCondition (ChannelCondition::LosConditionValue::LOS);
        }
      else
        {
          cond->SetLosCondition (ChannelCondition::LosConditionValue::NLOS);
        }
    }
  else //outdoor to indoor case
    {
      cond->SetLosCondition (ChannelCondition::LosConditionValue::NLOS);
    }

  return cond;
}

bool
BuildingsChannelConditionModel::IsWithinLineOfSight (const ns3::Vector &l1, const ns3::Vector &l2) const
{
  for (BuildingList::Iterator bit = BuildingList::Begin (); bit != BuildingList::End (); ++bit)
    {
      Box boundaries = (*bit)->GetBoundaries ();

      Vector boxSize (0.5 * (boundaries.xMax - boundaries.xMin),
                      0.5 * (boundaries.yMax - boundaries.yMin),
                      0.5 * (boundaries.zMax - boundaries.zMin));
      Vector boxCenter (boundaries.xMin + boxSize.x,
                        boundaries.yMin + boxSize.y,
                        boundaries.zMin + boxSize.z);

      // Put line in box space
      Vector lB1 (l1.x - boxCenter.x, l1.y - boxCenter.y, l1.z - boxCenter.z);
      Vector lB2 (l2.x - boxCenter.x, l2.y - boxCenter.y, l2.z - boxCenter.z);

      // Get line midpoint and extent
      Vector lMid (0.5 * (lB1.x + lB2.x), 0.5 * (lB1.y + lB2.y), 0.5 * (lB1.z + lB2.z));
      Vector l (lB1.x - lMid.x, lB1.y - lMid.y, lB1.z - lMid.z);
      Vector lExt (std::abs (l.x), std::abs (l.y), std::abs (l.z));

      // Use Separating Axis Test
      // Separation vector from box center to line center is lMid, since the line is in box space
      // If the line did not intersect this building, jump to the next building.
      if (std::abs(lMid.x) > boxSize.x + lExt.x)
        {
          continue;
        }
      if (std::abs(lMid.y) > boxSize.y + lExt.y)
        {
          continue;
        }
      if (std::abs(lMid.z) > boxSize.z + lExt.z)
        {
          continue;
        }
      // Crossproducts of line and each axis
      if (std::abs(lMid.y * l.z - lMid.z * l.y)  >  (boxSize.y * lExt.z + boxSize.z * lExt.y))
        {
          continue;
        }
      if (std::abs(lMid.x * l.z - lMid.z * l.x)  >  (boxSize.x * lExt.z + boxSize.z * lExt.x))
        {
          continue;
        }
      if (std::abs(lMid.x * l.y - lMid.y * l.x)  >  (boxSize.x * lExt.y + boxSize.y * lExt.x))
        {
          continue;
        }

      // No separating axis, the line intersects
      // If the line intersect this building, return true.
      return true;
    }
  return false;
}

int64_t
BuildingsChannelConditionModel::AssignStreams (int64_t stream)
{
  NS_UNUSED (stream);
  return 0;
}

} // end namespace ns3
