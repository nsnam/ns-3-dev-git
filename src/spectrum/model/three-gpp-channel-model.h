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
#include <unordered_map>
#include <ns3/channel-condition-model.h>
#include <ns3/matrix-based-channel-model.h>

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
class ThreeGppChannelModel : public MatrixBasedChannelModel
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
  
  void DoDispose () override;

  /**
   * Get the type ID
   * \return the object TypeId
   */
  static TypeId GetTypeId ();


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
   * \param aMob mobility model of the a device
   * \param bMob mobility model of the b device
   * \param aAntenna antenna of the a device
   * \param bAntenna antenna of the b device
   * \return the channel matrix
   */
  Ptr<const ChannelMatrix> GetChannel (Ptr<const MobilityModel> aMob,
                                       Ptr<const MobilityModel> bMob,
                                       Ptr<const ThreeGppAntennaArrayModel> aAntenna,
                                       Ptr<const ThreeGppAntennaArrayModel> bAntenna) override;
  /**
   * \brief Assign a fixed random variable stream number to the random variables
   * used by this model.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);
  
private:
  /**
   * \brief Shuffle the elements of a simple sequence container of type double
   * \param first Pointer to the first element among the elements to be shuffled
   * \param last Pointer to the last element among the elements to be shuffled
   */
  void Shuffle (double * first, double * last) const;
  /**
   * Extends the struct ChannelMatrix by including information that are used 
   * within the class ThreeGppChannelModel
   */
  struct ThreeGppChannelMatrix : public MatrixBasedChannelModel::ChannelMatrix
  {
    Ptr<const ChannelCondition> m_channelCondition; //!< the channel condition
    
    // TODO these are not currently used, they have to be correctly set when including the spatial consistent update procedure
    /*The following parameters are stored for spatial consistent updating. The notation is 
    that of 3GPP technical reports, but it can apply also to other channel realizations*/
    MatrixBasedChannelModel::Double2DVector m_nonSelfBlocking; //!< store the blockages
    Vector m_preLocUT; //!< location of UT when generating the previous channel
    Vector m_locUT; //!< location of UT
    MatrixBasedChannelModel::Double2DVector m_norRvAngles; //!< stores the normal variable for random angles angle[cluster][id] generated for equation (7.6-11)-(7.6-14), where id = 0(aoa),1(zoa),2(aod),3(zod)
    double m_DS; //!< delay spread
    double m_K; //!< K factor
    uint8_t m_numCluster; //!< reduced cluster number;
    MatrixBasedChannelModel::Double3DVector m_clusterPhase; //!< the initial random phases
    Vector m_speed; //!< velocity
    double m_dis2D; //!< 2D distance between tx and rx
    double m_dis3D; //!< 3D distance between tx and rx
  };

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
   * \param channelCondition the channel condition
   * \param hBS the height of the BS
   * \param hUT the height of the UT
   * \param distance2D the 2D distance between tx and rx
   * \return the parameters table
   */
  virtual Ptr<const ParamsTable> GetThreeGppTable (Ptr<const ChannelCondition> channelCondition, double hBS, double hUT, double distance2D) const;

  /**
   * Compute the channel matrix between two devices using the procedure
   * described in 3GPP TR 38.901
   * \param locUT the location of the UT
   * \param channelCondition the channel condition
   * \param sAntenna the s node antenna array
   * \param uAntenna the u node antenna array
   * \param uAngle the u node angle
   * \param sAngle the s node angle
   * \param dis2D the 2D distance between tx and rx
   * \param hBS the height of the BS
   * \param hUT the height of the UT
   * \return the channel realization
   */
  Ptr<ThreeGppChannelMatrix> GetNewChannel (Vector locUT, Ptr<const ChannelCondition> channelCondition,
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
   * \param channelCondition the channel condition
   * \return true if the channel matrix has to be updated, false otherwise
   */
  bool ChannelMatrixNeedsUpdate (Ptr<const ThreeGppChannelMatrix> channelMatrix, Ptr<const ChannelCondition> channelCondition) const;

  std::unordered_map<uint32_t, Ptr<ThreeGppChannelMatrix> > m_channelMap; //!< map containing the channel realizations
  Time m_updatePeriod; //!< the channel update period
  double m_frequency; //!< the operating frequency
  std::string m_scenario; //!< the 3GPP scenario
  Ptr<ChannelConditionModel> m_channelConditionModel; //!< the channel condition model
  Ptr<UniformRandomVariable> m_uniformRv; //!< uniform random variable
  Ptr<NormalRandomVariable> m_normalRv; //!< normal random variable
  Ptr<UniformRandomVariable> m_uniformRvShuffle; //!< uniform random variable used to shuffle array in GetNewChannel

  // parameters for the blockage model
  bool m_blockage; //!< enables the blockage model A
  uint16_t m_numNonSelfBlocking; //!< number of non-self-blocking regions
  bool m_portraitMode; //!< true if potrait mode, false if landscape
  double m_blockerSpeed; //!< the blocker speed

  static const uint8_t PHI_INDEX = 0; //!< index of the PHI value in the m_nonSelfBlocking array
  static const uint8_t X_INDEX = 1; //!< index of the X value in the m_nonSelfBlocking array
  static const uint8_t THETA_INDEX = 2; //!< index of the THETA value in the m_nonSelfBlocking array
  static const uint8_t Y_INDEX = 3; //!< index of the Y value in the m_nonSelfBlocking array
  static const uint8_t R_INDEX = 4; //!< index of the R value in the m_nonSelfBlocking array
};
} // namespace ns3

#endif /* THREE_GPP_CHANNEL_H */
