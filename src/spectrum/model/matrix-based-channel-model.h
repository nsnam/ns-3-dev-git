/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 * Copyright (c) 2020 Institute for the Wireless Internet of Things,
 * Northeastern University
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

#ifndef MATRIX_BASED_CHANNEL_H
#define MATRIX_BASED_CHANNEL_H

#include <complex.h>
#include <ns3/object.h>
#include <ns3/nstime.h>
#include <ns3/vector.h>
#include <ns3/phased-array-model.h>
#include <tuple>

namespace ns3 {

class MobilityModel;

/**
 * \ingroup spectrum
 *
 * This is an interface for a channel model that can be described
 * by a channel matrix, e.g., the 3GPP Spatial Channel Models,
 * which is generally used in combination with antenna arrays
 */
class MatrixBasedChannelModel : public Object
{
public:
  /**
   * Destructor for MatrixBasedChannelModel
   */
  virtual ~MatrixBasedChannelModel ();

  typedef std::vector<double> DoubleVector; //!< type definition for vectors of doubles
  typedef std::vector<DoubleVector> Double2DVector; //!< type definition for matrices of doubles
  typedef std::vector<Double2DVector> Double3DVector; //!< type definition for 3D matrices of doubles
  typedef std::vector<PhasedArrayModel::ComplexVector> Complex2DVector; //!< type definition for complex matrices
  typedef std::vector<Complex2DVector> Complex3DVector; //!< type definition for complex 3D matrices
  typedef std::pair<Ptr<const PhasedArrayModel>, Ptr<const PhasedArrayModel>> PhasedAntennaPair; //!< the antenna pair for which is generated the specific instance of the channel

  /**
   * Data structure that implements the hash function for the key made of the pair of Ptrs
   */
  struct PhasedAntennaPairHashXor
  {
    /**
     * Operator () definition to support the call of the hash function
     * \param pair the pair of pointers to PhasedArrayModel that will be used to calculate the hash value
     * \return returns the hash function value of the PhasedAntennaPair
     */
     std::size_t operator() (const PhasedAntennaPair &pair) const
     {
       return std::hash<const PhasedArrayModel*>()(PeekPointer (pair.first)) ^ std::hash<const PhasedArrayModel*>()(PeekPointer (pair.second));
     }
  };
  /**
   * Data structure that stores a channel realization
   */
  struct ChannelMatrix : public SimpleRefCount<ChannelMatrix>
  {
    Complex3DVector    m_channel; //!< channel matrix H[u][s][n].
    Time               m_generatedTime; //!< generation time
    PhasedAntennaPair  m_antennaPair; //!< the  first element is the antenna of the s-node antenna (the antenna of the transmitter when the channel was generated), the second element is the u-node antenna (the antenna of the receiver when the channel was generated)

    /**
     * Destructor for ChannelMatrix
     */
    virtual ~ChannelMatrix () = default;
    
    /**
     * Returns true if the ChannelMatrix object was generated
     * considering node b as transmitter and node a as receiver.
     * \param aAntennaModel the antenna array of the a node
     * \param bAntennaModel the antenna array of the b node
     * \return true if b is the rx and a is the tx, false otherwise
     */
    bool IsReverse (const Ptr<const PhasedArrayModel> aAntennaModel, const Ptr<const PhasedArrayModel> bAntennaModel) const
    {
      Ptr<const PhasedArrayModel> sAntennaModel, uAntennaModel;
      std::tie (sAntennaModel, uAntennaModel) = m_antennaPair;
      NS_ASSERT_MSG ((sAntennaModel == aAntennaModel && uAntennaModel == bAntennaModel) || (sAntennaModel == bAntennaModel && uAntennaModel == aAntennaModel),
                      "This channel matrix does not represent the channel among the provided antenna arrays.");
      return (sAntennaModel == bAntennaModel && uAntennaModel == aAntennaModel);
    }
  };


  /**
   * Data structure that stores channel parameters
   */
  struct ChannelParams : public SimpleRefCount<ChannelParams>
  {
    Time               m_generatedTime; //!< generation time
    DoubleVector       m_delay; //!< cluster delay in nanoseconds.
    Double2DVector     m_angle; //!< cluster angle angle[direction][n], where direction = 0(AOA), 1(ZOA), 2(AOD), 3(ZOD) in degree.
    /**
     * Destructor for ChannelParams
     */
    virtual ~ChannelParams () = default;
  };

  /**
   * Returns a matrix with a realization of the channel between
   * the nodes with mobility objects passed as input parameters.
   *
   * We assume channel reciprocity between each node pair (i.e., H_ab = H_ba^T),
   * therefore GetChannel (a, b) and GetChannel (b, a) will return the same
   * ChannelMatrix object.
   * To understand if the channel matrix corresponds to H_ab or H_ba, we provide
   * the method ChannelMatrix::IsReverse. For instance, if the channel
   * matrix corresponds to H_ab, a call to IsReverse (idA, idB) will return
   * false, conversely, IsReverse (idB, idA) will return true.
   *
   * \param aMob mobility model of the a device
   * \param bMob mobility model of the b device
   * \param aAntenna antenna of the a device
   * \param bAntenna antenna of the b device
   * \return the channel matrix
   */
  virtual Ptr<const ChannelMatrix> GetChannel (Ptr<const MobilityModel> aMob,
                                               Ptr<const MobilityModel> bMob,
                                               Ptr<const PhasedArrayModel> aAntenna,
                                               Ptr<const PhasedArrayModel> bAntenna) = 0;
  
  /**
   * Returns a channel parameters structure used to obtain the channel between
   * the nodes with mobility objects passed as input parameters.
   *
   * \param aMob mobility model of the a device
   * \param bMob mobility model of the b device
   * \return the channel matrix
   */
  virtual Ptr<const ChannelParams> GetParams (Ptr<const MobilityModel> aMob,
                                              Ptr<const MobilityModel> bMob) const = 0;

  /**
   * Generate a unique value for the pair of unsigned integer of 32 bits,
   * where the order does not matter, i.e., the same value will be returned for (a,b) and (b,a).
   * \param a the first value
   * \param b the second value
   * \return return an integer representing a unique value for the pair of values
   */
  static uint64_t GetKey (uint32_t a, uint32_t b)
  {
   return (uint64_t) std::min (a, b) << 32 | std::max (a, b);
  }

  /**
   * Put in ascending order the provided pair, if already in ascending order it returns it.
   * \param pair the pair to be sorted in ascending order
   * \return the pair in ascending order
   */
  static PhasedAntennaPair GetOrderedPhasedAntennaPair (const PhasedAntennaPair &pair)
  {
    return (pair.first < pair.second) ? pair : PhasedAntennaPair (pair.second, pair.first);
  }

  static const uint8_t AOA_INDEX = 0; //!< index of the AOA value in the m_angle array
  static const uint8_t ZOA_INDEX = 1; //!< index of the ZOA value in the m_angle array
  static const uint8_t AOD_INDEX = 2; //!< index of the AOD value in the m_angle array
  static const uint8_t ZOD_INDEX = 3; //!< index of the ZOD value in the m_angle array

};

}; // namespace ns3

#endif // MATRIX_BASED_CHANNEL_H
