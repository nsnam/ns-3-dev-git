/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
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
 *
 */

#include "ns3/log.h"
#include "three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/net-device.h"
#include "ns3/three-gpp-antenna-array-model.h"
#include "ns3/node.h"
#include "ns3/channel-condition-model.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ThreeGppSpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (ThreeGppSpectrumPropagationLossModel);

ThreeGppSpectrumPropagationLossModel::ThreeGppSpectrumPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
  m_uniformRv = CreateObject<UniformRandomVariable> ();
}

ThreeGppSpectrumPropagationLossModel::~ThreeGppSpectrumPropagationLossModel ()
{
  NS_LOG_FUNCTION (this);
}

void
ThreeGppSpectrumPropagationLossModel::DoDispose ()
{
  m_deviceAntennaMap.clear ();
  m_longTermMap.clear ();
  m_channelModel->Dispose ();
  m_channelModel = nullptr;
}

TypeId
ThreeGppSpectrumPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ThreeGppSpectrumPropagationLossModel")
    .SetParent<SpectrumPropagationLossModel> ()
    .SetGroupName ("Spectrum")
    .AddConstructor<ThreeGppSpectrumPropagationLossModel> ()
    .AddAttribute("ChannelModel", 
                  "The channel model. It needs to implement the MatrixBasedChannelModel interface",
                  StringValue("ns3::ThreeGppChannelModel"),
                  MakePointerAccessor (&ThreeGppSpectrumPropagationLossModel::SetChannelModel,
                                       &ThreeGppSpectrumPropagationLossModel::GetChannelModel),
                                       MakePointerChecker<MatrixBasedChannelModel> ())
    .AddAttribute ("vScatt",
                   "Maximum speed of the vehicle in the layout (see 3GPP TR 37.885 v15.3.0, Sec. 6.2.3)."
                   "Used to compute the additional contribution for the Doppler of" 
                   "delayed (reflected) paths",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&ThreeGppSpectrumPropagationLossModel::m_vScatt),
                   MakeDoubleChecker<double> (0.0))
    ;
  return tid;
}

void
ThreeGppSpectrumPropagationLossModel::SetChannelModel (Ptr<MatrixBasedChannelModel> channel)
{
  m_channelModel = channel;
}

Ptr<MatrixBasedChannelModel>
ThreeGppSpectrumPropagationLossModel::GetChannelModel () const
{
  return m_channelModel;
}

void
ThreeGppSpectrumPropagationLossModel::AddDevice (Ptr<NetDevice> n, Ptr<const ThreeGppAntennaArrayModel> a)
{
  NS_ASSERT_MSG (m_deviceAntennaMap.find (n->GetNode ()->GetId ()) == m_deviceAntennaMap.end (), "Device is already present in the map");
  m_deviceAntennaMap.insert (std::make_pair (n->GetNode ()->GetId (), a));
}

double
ThreeGppSpectrumPropagationLossModel::GetFrequency () const
{
  DoubleValue freq;
  m_channelModel->GetAttribute ("Frequency", freq);
  return freq.Get ();
}

void
ThreeGppSpectrumPropagationLossModel::SetChannelModelAttribute (const std::string &name, const AttributeValue &value)
{
  m_channelModel->SetAttribute (name, value);
}

void
ThreeGppSpectrumPropagationLossModel::GetChannelModelAttribute (const std::string &name, AttributeValue &value) const
{
  m_channelModel->GetAttribute (name, value);
}

ThreeGppAntennaArrayModel::ComplexVector
ThreeGppSpectrumPropagationLossModel::CalcLongTerm (Ptr<const MatrixBasedChannelModel::ChannelMatrix> params,
                                                    const ThreeGppAntennaArrayModel::ComplexVector &sW,
                                                    const ThreeGppAntennaArrayModel::ComplexVector &uW) const
{
  NS_LOG_FUNCTION (this);

  uint16_t sAntenna = static_cast<uint16_t> (sW.size ());
  uint16_t uAntenna = static_cast<uint16_t> (uW.size ());

  NS_LOG_DEBUG ("CalcLongTerm with sAntenna " << sAntenna << " uAntenna " << uAntenna);
  //store the long term part to reduce computation load
  //only the small scale fading needs to be updated if the large scale parameters and antenna weights remain unchanged.
  ThreeGppAntennaArrayModel::ComplexVector longTerm;
  uint8_t numCluster = static_cast<uint8_t> (params->m_channel[0][0].size ());

  for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
    {
      std::complex<double> txSum (0,0);
      for (uint16_t sIndex = 0; sIndex < sAntenna; sIndex++)
        {
          std::complex<double> rxSum (0,0);
          for (uint16_t uIndex = 0; uIndex < uAntenna; uIndex++)
            {
              rxSum = rxSum + uW[uIndex] * params->m_channel[uIndex][sIndex][cIndex];
            }
          txSum = txSum + sW[sIndex] * rxSum;
        }
      longTerm.push_back (txSum);
    }
  return longTerm;
}

Ptr<SpectrumValue>
ThreeGppSpectrumPropagationLossModel::CalcBeamformingGain (Ptr<SpectrumValue> txPsd,
                                                           ThreeGppAntennaArrayModel::ComplexVector longTerm,
                                                           Ptr<const MatrixBasedChannelModel::ChannelMatrix> params,
                                                           const ns3::Vector &sSpeed, const ns3::Vector &uSpeed) const
{
  NS_LOG_FUNCTION (this);

  Ptr<SpectrumValue> tempPsd = Copy<SpectrumValue> (txPsd);

  //channel[rx][tx][cluster]
  uint8_t numCluster = static_cast<uint8_t> (params->m_channel[0][0].size ());

  // compute the doppler term
  // NOTE the update of Doppler is simplified by only taking the center angle of
  // each cluster in to consideration.
  double slotTime = Simulator::Now ().GetSeconds ();
  ThreeGppAntennaArrayModel::ComplexVector doppler;
  for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
    {
      // Compute alpha and D as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3
      // These terms account for an additional Doppler contribution due to the 
      // presence of moving objects in the sorrounding environment, such as in 
      // vehicular scenarios.
      // This contribution is applied only to the delayed (reflected) paths and 
      // must be properly configured by setting the value of 
      // m_vScatt, which is defined as "maximum speed of the vehicle in the 
      // layout". 
      // By default, m_vScatt is set to 0, so there is no additional Doppler 
      // contribution.
      double alpha = 0; 
      double D = 0; 
      if (cIndex != 0)
      {
        alpha = m_uniformRv->GetValue (-1, 1);
        D = m_uniformRv->GetValue (-m_vScatt, m_vScatt);
      }
      
      //cluster angle angle[direction][n],where, direction = 0(aoa), 1(zoa).
      double temp_doppler = 2 * M_PI * ((sin (params->m_angle[MatrixBasedChannelModel::ZOA_INDEX][cIndex] * M_PI / 180) * cos (params->m_angle[MatrixBasedChannelModel::AOA_INDEX][cIndex] * M_PI / 180) * uSpeed.x
                                         + sin (params->m_angle[MatrixBasedChannelModel::ZOA_INDEX][cIndex] * M_PI / 180) * sin (params->m_angle[MatrixBasedChannelModel::AOA_INDEX][cIndex] * M_PI / 180) * uSpeed.y
                                         + cos (params->m_angle[MatrixBasedChannelModel::ZOA_INDEX][cIndex] * M_PI / 180) * uSpeed.z)
                                         + (sin (params->m_angle[MatrixBasedChannelModel::ZOD_INDEX][cIndex] * M_PI / 180) * cos (params->m_angle[MatrixBasedChannelModel::AOD_INDEX][cIndex] * M_PI / 180) * sSpeed.x
                                         + sin (params->m_angle[MatrixBasedChannelModel::ZOD_INDEX][cIndex] * M_PI / 180) * sin (params->m_angle[MatrixBasedChannelModel::AOD_INDEX][cIndex] * M_PI / 180) * sSpeed.y
                                         + cos (params->m_angle[MatrixBasedChannelModel::ZOD_INDEX][cIndex] * M_PI / 180) * sSpeed.z) + 2 * alpha * D)
                           * slotTime * GetFrequency () / 3e8;
      doppler.push_back (exp (std::complex<double> (0, temp_doppler)));
    }

  // apply the doppler term and the propagation delay to the long term component
  // to obtain the beamforming gain
  auto vit = tempPsd->ValuesBegin (); // psd iterator
  auto sbit = tempPsd->ConstBandsBegin(); // band iterator
  while (vit != tempPsd->ValuesEnd ())
    {
      std::complex<double> subsbandGain (0.0,0.0);
      if ((*vit) != 0.00)
        {
          double fsb = (*sbit).fc; // center frequency of the sub-band
          for (uint8_t cIndex = 0; cIndex < numCluster; cIndex++)
            {
              double delay = -2 * M_PI * fsb * (params->m_delay[cIndex]);
              subsbandGain = subsbandGain + longTerm[cIndex] * doppler[cIndex] * exp (std::complex<double> (0, delay));
            }
          *vit = (*vit) * (norm (subsbandGain));
        }
      vit++;
      sbit++;
    }
  return tempPsd;
}

ThreeGppAntennaArrayModel::ComplexVector
ThreeGppSpectrumPropagationLossModel::GetLongTerm (uint32_t aId, uint32_t bId,
                                                   Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
                                                   const ThreeGppAntennaArrayModel::ComplexVector &aW,
                                                   const ThreeGppAntennaArrayModel::ComplexVector &bW) const
{
  ThreeGppAntennaArrayModel::ComplexVector longTerm; // vector containing the long term component for each cluster

  // check if the channel matrix was generated considering a as the s-node and
  // b as the u-node or viceversa
  ThreeGppAntennaArrayModel::ComplexVector sW, uW;
  if (!channelMatrix->IsReverse (aId, bId))
  {
    sW = aW;
    uW = bW;
  }
  else
  {
    sW = bW;
    uW = aW;
  }

  // compute the long term key, the key is unique for each tx-rx pair
  uint32_t x1 = std::min (aId, bId);
  uint32_t x2 = std::max (aId, bId);
  uint32_t longTermId = MatrixBasedChannelModel::GetKey (x1, x2);

  bool update = false; // indicates whether the long term has to be updated
  bool notFound = false; // indicates if the long term has not been computed yet

  // look for the long term in the map and check if it is valid
  if (m_longTermMap.find (longTermId) != m_longTermMap.end ())
  {
    NS_LOG_DEBUG ("found the long term component in the map");
    longTerm = m_longTermMap[longTermId]->m_longTerm;

    // check if the channel matrix has been updated
    // or the s beam has been changed
    // or the u beam has been changed
    update = (m_longTermMap[longTermId]->m_channel->m_generatedTime != channelMatrix->m_generatedTime
              || m_longTermMap[longTermId]->m_sW != sW
              || m_longTermMap[longTermId]->m_uW != uW);

  }
  else
  {
    NS_LOG_DEBUG ("long term component NOT found");
    notFound = true;
  }

  if (update || notFound)
    {
      NS_LOG_DEBUG ("compute the long term");
      // compute the long term component
      longTerm = CalcLongTerm (channelMatrix, sW, uW);

      // store the long term
      Ptr<LongTerm> longTermItem = Create<LongTerm> ();
      longTermItem->m_longTerm = longTerm;
      longTermItem->m_channel = channelMatrix;
      longTermItem->m_sW = sW;
      longTermItem->m_uW = uW;

      m_longTermMap[longTermId] = longTermItem;
    }

  return longTerm;
}

Ptr<SpectrumValue>
ThreeGppSpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                                    Ptr<const MobilityModel> a,
                                                                    Ptr<const MobilityModel> b) const
{
  NS_LOG_FUNCTION (this);
  uint32_t aId = a->GetObject<Node> ()->GetId (); // id of the node a
  uint32_t bId = b->GetObject<Node> ()->GetId (); // id of the node b

  NS_ASSERT (aId != bId);
  NS_ASSERT_MSG (a->GetDistanceFrom (b) > 0.0, "The position of a and b devices cannot be the same");

  Ptr<SpectrumValue> rxPsd = Copy<SpectrumValue> (txPsd);

  // retrieve the antenna of device a
  NS_ASSERT_MSG (m_deviceAntennaMap.find (aId) != m_deviceAntennaMap.end (), "Antenna not found for node " << aId);
  Ptr<const ThreeGppAntennaArrayModel> aAntenna = m_deviceAntennaMap.at (aId);
  NS_LOG_DEBUG ("a node " << a->GetObject<Node> () << " antenna " << aAntenna);

  // retrieve the antenna of the device b
  NS_ASSERT_MSG (m_deviceAntennaMap.find (bId) != m_deviceAntennaMap.end (), "Antenna not found for device " << bId);
  Ptr<const ThreeGppAntennaArrayModel> bAntenna = m_deviceAntennaMap.at (bId);
  NS_LOG_DEBUG ("b node " << bId << " antenna " << bAntenna);

  if (aAntenna->IsOmniTx () || bAntenna->IsOmniTx () )
    {
      NS_LOG_LOGIC ("Omni transmission, do nothing.");
      return rxPsd;
    }

  Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix = m_channelModel->GetChannel (a, b, aAntenna, bAntenna);

  // get the precoding and combining vectors
  ThreeGppAntennaArrayModel::ComplexVector aW = aAntenna->GetBeamformingVector ();
  ThreeGppAntennaArrayModel::ComplexVector bW = bAntenna->GetBeamformingVector ();

  // retrieve the long term component
  ThreeGppAntennaArrayModel::ComplexVector longTerm = GetLongTerm (aId, bId, channelMatrix, aW, bW);

  // apply the beamforming gain
  rxPsd = CalcBeamformingGain (rxPsd, longTerm, channelMatrix, a->GetVelocity (), b->GetVelocity ());

  return rxPsd;
}


}  // namespace ns3
