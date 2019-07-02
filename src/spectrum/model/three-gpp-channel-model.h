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
 */

#ifndef THREE_GPP_CHANNEL_H
#define THREE_GPP_CHANNEL_H

#include  <complex.h>
#include "ns3/angles.h"
#include <ns3/object.h>
#include <ns3/nstime.h>
#include <ns3/random-variable-stream.h>
#include <ns3/boolean.h>
#include <ns3/three-gpp-antenna-array-model.h>
#include <unordered_map>
#include <ns3/channel-condition-model.h>

namespace ns3 {

class MobilityModel;

/**
 * \ingroup spectrum
 * \brief Channel Matrix Generation following 3GPP TR 38.901
 *
 * The class implements the channel matrix generation procedure
 * described in 3GPP TR 38.901.
 *
 * \see GetChannel
 */
class ThreeGppChannelModel : public Object
{
public:
  /**
   * Constructor
   */
  ThreeGppChannelModel ();

  /**
   * Destructor
   */
  ~ThreeGppChannelModel ();

  /**
   * Get the type ID
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  typedef std::vector<double> DoubleVector; //!< type definition for vectors of doubles
  typedef std::vector<DoubleVector> Double2DVector; //!< type definition for matrices of doubles
  typedef std::vector<Double2DVector> Double3DVector; //!< type definition for 3D matrices of doubles
  typedef std::vector<ThreeGppAntennaArrayModel::ComplexVector> Complex2DVector; //!< type definition for complex matrices
  typedef std::vector<Complex2DVector> Complex3DVector; //!< type definition for complex 3D matrices

  /**
   * Data structure that stores a channel realization
   */
  struct ThreeGppChannelMatrix : public SimpleRefCount<ThreeGppChannelMatrix>
  {
    Complex3DVector    m_channel; //!< channel matrix H[u][s][n].
    DoubleVector       m_delay; //!< cluster delay.
    Double2DVector     m_angle; //!< cluster angle angle[direction][n], where direction = 0(aoa), 1(zoa), 2(aod), 3(zod) in degree.
    Double2DVector     m_nonSelfBlocking; //!< store the blockages
    Time               m_generatedTime; //!< generation time
    std::pair<uint32_t, uint32_t> m_nodeIds; //!< the first element is the s-node ID, the second element is the u-node ID
    bool m_los; //!< true if LOS, false if NLOS

    // TODO these are not currently used, they have to be correctly set when including the spatial consistent update procedure
    /*The following parameters are stored for spatial consistent updating*/
    Vector m_preLocUT; //!< location of UT when generating the previous channel
    Vector m_locUT; //!< location of UT
    Double2DVector m_norRvAngles; //!< stores the normal variable for random angles angle[cluster][id] generated for equation (7.6-11)-(7.6-14), where id = 0(aoa),1(zoa),2(aod),3(zod)
    double m_DS; //!< delay spread
    double m_K; //!< K factor
    uint8_t m_numCluster; //!< reduced cluster number;
    Double3DVector m_clusterPhase; //!< the initial random phases
    bool m_o2i; //!< true if O2I
    Vector m_speed; //!< velocity
    double m_dis2D; //!< 2D distance between tx and rx
    double m_dis3D; //!< 3D distance between tx and rx

    /**
     * Returns true if the ThreeGppChannelMatrix object was generated
     * considering node b as transmitter and node a as receiver.
     * \param aid id of the a node
     * \param bid id of the b node
     * \return true if b is the rx and a is the tx, false otherwise
     */
    bool IsReverse (const uint32_t aId, const uint32_t bId) const
    {
      uint32_t sId, uId;
      std::tie (sId, uId) = m_nodeIds;
      NS_ASSERT_MSG ((sId == aId && uId == bId) || (sId == bId && uId == aId),
                      "This matrix represents the channel between " << sId << " and " << uId);
      return (sId == bId && uId == aId);
    }
  };

  /**
   * Set the channel condition model
   * \param a pointer to the ChannelConditionModel object
   */
  void SetChannelConditionModel (Ptr<ChannelConditionModel> model);

  /**
   * Get the associated channel condition model
   * \return a pointer to the ChannelConditionModel object
   */
  Ptr<ChannelConditionModel> GetChannelConditionModel () const;

  /**
   * Sets the center frequency of the model
   * \param f the center frequency in Hz
   */
  void SetFrequency (double f);

  /**
   * Returns the center frequency
   * \return the center frequency in Hz
   */
  double GetFrequency (void) const;

  /**
   * Sets the propagation scenario
   * \param scenario the propagation scenario
   */
  void SetScenario (const std::string &scenario);

  /**
   * Returns the propagation scenario
   * \return the propagation scenario
   */
  std::string GetScenario (void) const;

  /**
   * Looks for the channel matrix associated to the aMob and bMob pair in m_channelMap.
   * If found, it checks if it has to be updated. If not found or if it has to
   * be updated, it generates a new uncorrelated channel matrix using the
   * method GetNewChannel and updates m_channelMap.
   *
   * We assume channel reciprocity between each node pair (i.e., H_ab = H_ba^T),
   * therefore GetChannel (a, b) and GetChannel (b, a) will return the same
   * ThreeGppChannelMatrix object.
   * To understand if the channel matrix corresponds to H_ab or H_ba, we provide
   * the method ThreeGppChannelMatrix::IsReverse. For instance, if the channel
   * matrix corresponds to H_ab, a call to IsReverse (idA, idB) will return
   * false, conversely, IsReverse (idB, idA) will return true.
   *
   * \param aMob mobility model of the a device
   * \param bMob mobility model of the b device
   * \param aAntenna antenna of the a device
   * \param bAntenna antenna of the b device
   * \return the channel matrix
   */
  Ptr<const ThreeGppChannelMatrix> GetChannel (Ptr<const MobilityModel> aMob,
                                               Ptr<const MobilityModel> bMob,
                                               Ptr<const ThreeGppAntennaArrayModel> aAntenna,
                                               Ptr<const ThreeGppAntennaArrayModel> bAntenna);
  /**
   * \brief Assign a fixed random variable stream number to the random variables
   * used by this model.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

  static const uint8_t AOA_INDEX = 0; //!< index of the AOA value in the m_angle array
  static const uint8_t ZOA_INDEX = 1; //!< index of the ZOA value in the m_angle array
  static const uint8_t AOD_INDEX = 2; //!< index of the AOD value in the m_angle array
  static const uint8_t ZOD_INDEX = 3; //!< index of the ZOD value in the m_angle array

  static const uint8_t PHI_INDEX = 0; //!< index of the PHI value in the m_nonSelfBlocking array
  static const uint8_t X_INDEX = 1; //!< index of the X value in the m_nonSelfBlocking array
  static const uint8_t THETA_INDEX = 2; //!< index of the THETA value in the m_nonSelfBlocking array
  static const uint8_t Y_INDEX = 3; //!< index of the Y value in the m_nonSelfBlocking array
  static const uint8_t R_INDEX = 4; //!< index of the R value in the m_nonSelfBlocking array

 /**
  * Calculate the channel key using the Cantor function
  * \param x1 first value
  * \param x2 second value
  * \return \f$ (((x1 + x2) * (x1 + x2 + 1))/2) + x2; \f$
  */
 static constexpr uint32_t GetKey (uint32_t x1, uint32_t x2)
 {
   return (((x1 + x2) * (x1 + x2 + 1)) / 2) + x2;
 }

private:
  /**
   * Data structure that stores the parameters of 3GPP TR 38.901, Table 7.5-6,
   * for a certain scenario
   */
  struct ParamsTable : public SimpleRefCount<ParamsTable>
  {
    uint8_t m_numOfCluster = 0;
    uint8_t m_raysPerCluster = 0;
    double m_uLgDS = 0;
    double m_sigLgDS = 0;
    double m_uLgASD = 0;
    double m_sigLgASD = 0;
    double m_uLgASA = 0;
    double m_sigLgASA = 0;
    double m_uLgZSA = 0;
    double m_sigLgZSA = 0;
    double m_uLgZSD = 0;
    double m_sigLgZSD = 0;
    double m_offsetZOD = 0;
    double m_cDS = 0;
    double m_cASD = 0;
    double m_cASA = 0;
    double m_cZSA = 0;
    double m_uK = 0;
    double m_sigK = 0;
    double m_rTau = 0;
    double m_uXpr = 0;
    double m_sigXpr = 0;
    double m_perClusterShadowingStd = 0;

    double m_sqrtC[7][7];
  };

  /**
   * Get the parameters needed to apply the channel generation procedure
   * \param los the LOS/NLOS condition
   * \param o2i whether if it is an outdoor to indoor transmission
   * \param hBS the height of the BS
   * \param hUT the height of the UT
   * \param distance2D the 2D distance between tx and rx
   * \return the parameters table
   */
  Ptr<const ParamsTable> GetThreeGppTable (bool los, bool o2i, double hBS, double hUT, double distance2D) const;

  /**
   * Compute the channel matrix between two devices using the procedure
   * described in 3GPP TR 38.901
   * \param locUT the location of the UT
   * \param los the LOS/NLOS condition
   * \param o2i whether if it is an outdoor to indoor transmission
   * \param sAntenna the s node antenna array
   * \param uAntenna the u node antenna array
   * \param uAngle the u node angle
   * \param sAngle the s node angle
   * \param dis2D the 2D distance between tx and rx
   * \param hBS the height of the BS
   * \param hUT the height of the UT
   * \return the channel realization
   */
  Ptr<ThreeGppChannelMatrix> GetNewChannel (Vector locUT, bool los, bool o2i,
                                            Ptr<const ThreeGppAntennaArrayModel> sAntenna,
                                            Ptr<const ThreeGppAntennaArrayModel> uAntenna,
                                            Angles &uAngle, Angles &sAngle,
                                            double dis2D, double hBS, double hUT) const;

  /**
   * Applies the blockage model A described in 3GPP TR 38.901
   * \param params the channel matrix
   * \param clusterAOA vector containing the azimuth angle of arrival for each cluster
   * \param clusterZOA vector containing the zenith angle of arrival for each cluster
   * \return vector containing the power attenuation for each cluster
   */
  DoubleVector CalcAttenuationOfBlockage (Ptr<ThreeGppChannelMatrix> params,
                                          const DoubleVector &clusterAOA,
                                          const DoubleVector &clusterZOA) const;

  /**
   * Check if the channel matrix has to be updated
   * \param channelMatrix channel matrix
   * \param isLos the current los condition
   * \return true if the channel matrix has to be updated, false otherwise
   */
  bool ChannelMatrixNeedsUpdate (Ptr<const ThreeGppChannelMatrix> channelMatrix, bool isLos) const;

  std::unordered_map<uint32_t, Ptr<ThreeGppChannelMatrix> > m_channelMap; //!< map containing the channel realizations
  Time m_updatePeriod; //!< the channel update period
  double m_frequency; //!< the operating frequency
  std::string m_scenario; //!< the 3GPP scenario
  Ptr<ChannelConditionModel> m_channelConditionModel; //!< the channel condition model
  Ptr<UniformRandomVariable> m_uniformRv; //!< uniform random variable
  Ptr<NormalRandomVariable> m_normalRv; //!< normal random variable

  // parameters for the blockage model
  bool m_blockage; //!< enables the blockage model A
  uint16_t m_numNonSelfBlocking; //!< number of non-self-blocking regions
  bool m_portraitMode; //!< true if potrait mode, false if landscape
  double m_blockerSpeed; //!< the blocker speed
};
} // namespace ns3

#endif /* THREE_GPP_CHANNEL_H */
