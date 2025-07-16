/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef THREE_GPP_CHANNEL_H
#define THREE_GPP_CHANNEL_H

#include "matrix-based-channel-model.h"

#include "ns3/angles.h"
#include "ns3/boolean.h"
#include "ns3/channel-condition-model.h"
#include "ns3/deprecated.h"

#include <complex.h>
#include <unordered_map>

namespace ns3
{

class MobilityModel;

/**
 * ThreeGppChannelModel is a MatrixBasedChannelModel extension implementing a
 * 3GPP TR 38.901 channel model for wireless propagation, supporting channel
 * matrices, channel conditions, and propagation scenarios.
 */
class ThreeGppChannelModel : public MatrixBasedChannelModel
{
  public:
    /**
     * Constructor
     */
    ThreeGppChannelModel();

    /**
     * Destructor
     */
    ~ThreeGppChannelModel() override;

    void DoDispose() override;

    /**
     * Get the type ID
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Set the channel condition model
     * @param model a pointer to the ChannelConditionModel object
     */
    void SetChannelConditionModel(Ptr<ChannelConditionModel> model);

    /**
     * Get the associated channel condition model
     * @return a pointer to the ChannelConditionModel object
     */
    Ptr<ChannelConditionModel> GetChannelConditionModel() const;

    /**
     * Sets the center frequency of the model
     * @param f the center frequency in Hz
     */
    void SetFrequency(double f);

    /**
     * Returns the center frequency
     * @return the center frequency in Hz
     */
    double GetFrequency() const;

    /**
     * Sets the propagation scenario
     * @param scenario the propagation scenario
     */
    void SetScenario(const std::string& scenario);

    /**
     * Returns the propagation scenario
     * @return the propagation scenario
     */
    std::string GetScenario() const;

    /**
     * Looks for the channel matrix associated to the aMob and bMob pair in m_channelMatrixMap.
     * If found, it checks if it has to be updated. If not found or if it has to
     * be updated, it generates a new uncorrelated channel matrix using the
     * method GetNewChannel and updates m_channelMap.
     *
     * @param aMob mobility model of the a device
     * @param bMob mobility model of the b device
     * @param aAntenna antenna of the a device
     * @param bAntenna antenna of the b device
     * @return the channel matrix
     */
    Ptr<const ChannelMatrix> GetChannel(Ptr<const MobilityModel> aMob,
                                        Ptr<const MobilityModel> bMob,
                                        Ptr<const PhasedArrayModel> aAntenna,
                                        Ptr<const PhasedArrayModel> bAntenna) override;

    /**
     * Looks for the channel params associated to the aMob and bMob pair in
     * m_channelParamsMap. If not found it will return a nullptr.
     *
     * @param aMob mobility model of the a device
     * @param bMob mobility model of the b device
     * @return the channel params
     */
    Ptr<const ChannelParams> GetParams(Ptr<const MobilityModel> aMob,
                                       Ptr<const MobilityModel> bMob) const override;
    /**
     * @brief Assign a fixed random variable stream number to the random variables
     * used by this model.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    /**
     * Wrap an (azimuth, inclination) angle pair in a valid range.
     * Specifically, inclination must be in [0, M_PI] and azimuth in [0, 2*M_PI).
     * If the inclination angle is outside of its range, the azimuth angle is
     * rotated by M_PI.
     * This methods aims specifically at solving the problem of generating angles at
     * the boundaries of the angle domain, specifically, generating angle distributions
     * close to inclinationRad=0 and inclinationRad=M_PI.
     *
     * @param azimuthRad the azimuth angle in radians
     * @param inclinationRad the inclination angle in radians
     * @return the wrapped (azimuth, inclination) angle pair in radians
     */
    static std::pair<double, double> WrapAngles(double azimuthRad, double inclinationRad);

    /**
     * Extends the struct ChannelParams by including information that is used
     * within the ThreeGppChannelModel class
     */
    struct ThreeGppChannelParams : public MatrixBasedChannelModel::ChannelParams
    {
        ChannelCondition::LosConditionValue
            m_losCondition; //!< contains the information about the LOS state of the channel
        ChannelCondition::O2iConditionValue
            m_o2iCondition; //!< contains the information about the O2I state of the channel
        // TODO these are not currently used, they have to be correctly set when including the
        // spatial consistent update procedure
        /*The following parameters are stored for spatial consistent updating. The notation is
        that of 3GPP technical reports, but it can apply also to other channel realizations*/
        MatrixBasedChannelModel::Double2DVector m_nonSelfBlocking; //!< store the blockages
        Vector m_preLocUT; //!< location of UT when generating the previous channel
        Vector m_locUT;    //!< location of UT
        MatrixBasedChannelModel::Double2DVector
            m_norRvAngles; //!< stores the normal variable for random angles angle[cluster][id]
                           //!< generated for equation (7.6-11)-(7.6-14), where id =
                           //!< 0(aoa),1(zoa),2(aod),3(zod)
        double m_DS;       //!< delay spread
        double m_K_factor; //!< K factor
        uint8_t m_reducedClusterNumber; //!< reduced cluster number;
        MatrixBasedChannelModel::Double2DVector
            m_rayAodRadian; //!< the vector containing AOD angles
        MatrixBasedChannelModel::Double2DVector
            m_rayAoaRadian; //!< the vector containing AOA angles
        MatrixBasedChannelModel::Double2DVector
            m_rayZodRadian; //!< the vector containing ZOD angles
        MatrixBasedChannelModel::Double2DVector
            m_rayZoaRadian; //!< the vector containing ZOA angles
        MatrixBasedChannelModel::Double3DVector m_clusterPhase; //!< the initial random phases
        MatrixBasedChannelModel::Double2DVector
            m_crossPolarizationPowerRatios; //!< cross polarization power ratios
        Vector m_speed;                     //!< velocity
        double m_dis2D;                     //!< 2D distance between tx and rx
        double m_dis3D;                     //!< 3D distance between tx and rx
        DoubleVector m_clusterPowers;       //!< cluster powers
        DoubleVector m_attenuation_dB;      //!< vector that stores the attenuation of the blockage
        uint8_t m_cluster1st;               //!< index of the first strongest cluster
        uint8_t m_cluster2nd;               //!< index of the second strongest cluster
        Vector m_txSpeed;                   //!< TX velocity
        Vector m_rxSpeed;                   //!< RX velocity
        DoubleVector m_delayConsistency;    //!< cluster delay for consistency update
        bool m_newChannel; //!< true if the channel was generated with GetNewChannel
    };

    /**
     * Data structure that stores the parameters of 3GPP TR 38.901, Table 7.5-6,
     * for a certain scenario
     */
    struct ParamsTable : public SimpleRefCount<ParamsTable>
    {
        uint8_t m_numOfCluster = 0;   //!< Number of clusters
        uint8_t m_raysPerCluster = 0; //!< Number of rays per cluster
        double m_uLgDS = 0;           //!< Mean value of 10-base logarithm of delay spread
        double m_sigLgDS = 0; //!< Standard deviation value of 10-base logarithm of delay spread
        double m_uLgASD =
            0; //!< Mean value of 10-base logarithm of azimuth angle spread of departure
        double m_sigLgASD =
            0; //!< Standard deviation of 10-base logarithm of azimuth angle spread of departure
        double m_uLgASA = 0; //!< Mean value of 10-base logarithm of azimuth angle spread of arrival
        double m_sigLgASA =
            0; //!< Standard deviation of 10-base logarithm of azimuth angle spread of arrival
        double m_uLgZSA = 0; //!< Mean value of 10-base logarithm of zenith angle spread of arrival
        double m_sigLgZSA =
            0; //!< Standard deviation of 10-base logarithm of zenith angle spread of arrival
        double m_uLgZSD =
            0; //!< Mean value of 10-base logarithm of zenith angle spread of departure
        double m_sigLgZSD =
            0; //!< Standard deviation of 10-base logarithm of zenith angle spread of departure
        double m_offsetZOD = 0;              //!< Offset of zenith angle of departure
        double m_cDS = 0;                    //!< Cluster DS
        double m_cASD = 0;                   //!< Cluster ASD (Azimuth angle Spread of Departure)
        double m_cASA = 0;                   //!< Cluster ASA (Azimuth angle Spread of Arrival)
        double m_cZSA = 0;                   //!< Cluster ZSA (Zenith angle Spread of Arrival)
        double m_uK = 0;                     //!< Mean of K-factor
        double m_sigK = 0;                   //!< Standard deviation of K-factor
        double m_rTau = 0;                   //!< Delay scaling parameter
        double m_uXpr = 0;                   //!< Mean of Cross-Polarization Ratio
        double m_sigXpr = 0;                 //!< Standard deviation of Cross-Polarization Ratio
        double m_perClusterShadowingStd = 0; //!< Per cluster shadowing standard deviation
        /**
         * For LOS, LSP is following the order of [SF,K,DS,ASD,ASA,ZSD,ZSA].
         * For NLOS, LSP is following the order of [SF,DS,ASD,ASA,ZSD,ZSA].
         * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
         */
        double m_sqrtC[7][7];
    };

    /**
     * Get the parameters needed to apply the channel generation procedure
     * @param aMob the mobility model of node A
     * @param bMob the mobility model of node B
     * @param channelCondition the channel condition
     * @return the parameters table
     */
    virtual Ptr<const ParamsTable> GetThreeGppTable(
        const Ptr<const MobilityModel> aMob,
        const Ptr<const MobilityModel> bMob,
        Ptr<const ChannelCondition> channelCondition) const;

    /**
     * Prepare 3gpp channel parameters among the nodes a and b.
     * The function does the following steps described in 3GPP 38.901:
     *
     * Step 4: Generate large scale parameters. All LSPS are uncorrelated.
     * Step 5: Generate Delays.
     * Step 6: Generate cluster powers.
     * Step 7: Generate arrival and departure angles for both azimuth and elevation.
     * Step 8: Coupling of rays within a cluster for both azimuth and elevation
     * shuffle all the arrays to perform random coupling
     * Step 9: Generate the cross polarization power ratios
     * Step 10: Draw initial phases
     *
     * All relevant generated parameters are added then to ThreeGppChannelParams
     * which is the return value of this function.
     * @param channelCondition the channel condition
     * @param table3gpp the 3gpp parameters from the table
     * @param aMob the a node mobility model
     * @param bMob the b node mobility model
     * @return ThreeGppChannelParams structure with all the channel parameters generated
     * according 38.901 steps from 4 to 10.
     */
    Ptr<ThreeGppChannelParams> GenerateChannelParameters(
        const Ptr<const ChannelCondition> channelCondition,
        const Ptr<const ParamsTable> table3gpp,
        const Ptr<const MobilityModel> aMob,
        const Ptr<const MobilityModel> bMob) const;

    /**
     * @brief Large-scale channel parameters (3GPP TR 38.901).
     *
     * Contains the large-scale statistics used by the channel model.
     * Shadow fading (SF) is handled in the propagation loss model, not here.
     *
     * @note Units:
     *   - DS: delay spread [s]
     *   - ASD/ASA/ZSD/ZSA: angular spreads [deg]
     *   - kFactor: Rician K-factor [dB] (LOS only; 0 for NLOS usage)
     */
    struct LargeScaleParameters
    {
        /** Delay spread [s]. */
        double DS{0};
        /** Azimuth spread of departure [deg]. */
        double ASD{0};
        /** Azimuth spread of arrival [deg]. */
        double ASA{0};
        /** Zenith spread of arrival [deg]. */
        double ZSA{0};
        /** Zenith spread of departure [deg]. */
        double ZSD{0};
        /** Rician K-factor [dB] (used in LOS). */
        double kFactor{0};
    };

    /**
     * @brief Generate large-scale parameters (LSPs) for the current channel state.
     *
     * Draws correlated large-scale parameters according to the 3GPP table and the
     * specified LOS/NLOS condition, including delay spread (DS), angular spreads
     * (ASD/ASA/ZSD/ZSA), and K-factor for LOS.
     *
     * @param losCondition Line-of-sight condition (LOS or NLOS).
     * @param table3gpp Pointer to the 3GPP parameters table (means, std-devs, sqrt correlation).
     * @return LargeScaleParameters structure containing the generated LSPs.
     */
    LargeScaleParameters GenerateLSPs(ChannelCondition::LosConditionValue losCondition,
                                      const Ptr<const ParamsTable> table3gpp) const;

    /**
     * Generate the cluster delays.
     * @param DS Delay Spread
     * @param table3gpp 3GPP parameters from the table
     * @param minTau the minimum delay
     * @param clusterDelays output vector for cluster delays
     */

    void GenerateClusterDelays(double DS,
                               const Ptr<const ParamsTable> table3gpp,
                               double* minTau,
                               DoubleVector* clusterDelays) const;

    /**
     * Generate cluster powers.
     * @param clusterDelays cluster delays
     * @param DS Delay Spread
     * @param table3gpp 3GPP parameters from the table
     * @param clusterPowers output vector for cluster powers
     */
    void GenerateClusterPowers(const DoubleVector& clusterDelays,
                               const double DS,
                               const Ptr<const ParamsTable> table3gpp,
                               DoubleVector* clusterPowers) const;

    /**
     * Remove the clusters with less power.
     * @param clusterPowers input/output vector of cluster powers (will be modified)
     * @param clusterDelays input/output vector of cluster delays (will be modified)
     * @param losCondition LOS condition
     * @param table3gpp 3GPP parameters from the table
     * @param kFactor K factor
     * @param powerMax output for maximum power
     */
    MatrixBasedChannelModel::DoubleVector RemoveWeakClusters(
        DoubleVector* clusterPowers,
        DoubleVector* clusterDelays,
        ChannelCondition::LosConditionValue losCondition,
        const Ptr<const ParamsTable> table3gpp,
        double kFactor,
        double* powerMax) const;

    /**
    * @brief Adjusts cluster delays for LOS channel condition based on the K-factor.
    *
    * This function scales the initial NLOS cluster delays to reflect LOS conditions
    * as described in 3GPP TR 38.901. The delays are normalized using a correction
    * factor `cTau` that depends on the Rician K-factor, as defined in Table 7.5-3.
    *
    * The formula used to compute `cTau` is:
    * \f[
    * c_{\tau} = 0.7705 - 0.0433K + 0.0002K^2 + 0.000017K^3
    * \f]
    *
    * Each cluster delay \f$\tau_i\f$ is then scaled by:
    * \f[
    * \tau_i^{\text{LOS}} = \frac{\tau_i^{\text{NLOS}}}{c_{\tau}}
    * \f]
    * (see Equation 7.5-4)
    *
    * @param clusterDelays Pointer to a vector of initial (NLOS) cluster delays, which will be
    updated in place.
    * @param reducedClusterNumber The number of clusters to be considered (can be < total clusters).
    * @param kFactor The Rician K-factor, used to adjust the delay scaling factor `cTau`.
    */
    void AdjustClusterDelaysForLosCondition(DoubleVector* clusterDelays,
                                            uint8_t reducedClusterNumber,
                                            double kFactor) const;

    /**
     * @brief Calculates the cPhi parameter for azimuth angular spread modeling.
     *
     * This function computes the `cPhi` coefficient used to model the azimuth
     * angular spread (ASA or ASD) for a given channel condition and cluster configuration.
     *
     * - In NLOS conditions, the value is taken directly from the `cNlosTablePhi`.
     * - In LOS conditions, the NLOS value is adjusted using a polynomial function
     *   of the Rician K-factor, as defined in Table 7.5-10 of 3GPP TR 38.901.
     *
     * The formula applied under LOS is:
     * \f[
     * c_{\phi,\text{LOS}} = c_{\phi,\text{NLOS}} \cdot (1.1035 - 0.028K - 0.002K^2 + 0.0001K^3)
     * \f]
     *
     * @param losCondition Indicates whether the channel is LOS or NLOS.
     * @param table3gpp Pointer to the table containing the number of clusters.
     * @param kFactor The Rician K-factor, used to scale cPhi in LOS conditions.
     *
     * @return The computed cPhi value.
     *
     * @throw NS_FATAL_ERROR if the number of clusters is invalid or not found in the lookup table.
     */
    static double CalculateCphi(ChannelCondition::LosConditionValue losCondition,
                                const Ptr<const ParamsTable> table3gpp,
                                double kFactor);

    /**
     * @brief Calculates the cTheta parameter for zenith angular spread modeling.
     *
     * This function computes the `cTheta` coefficient used to model the zenith
     * angular spread (ZSA or ZSD) for a given channel condition and cluster configuration.
     *
     * - In NLOS conditions, the value is taken directly from the `cNlosTableTheta`.
     * - In LOS conditions, the NLOS value is adjusted using a polynomial function
     *   of the Rician K-factor, as defined in Table 7.5-15 of 3GPP TR 38.901.
     *
     * The formula applied under LOS is:
     * \f[
     * c_{\theta,\text{LOS}} = c_{\theta,\text{NLOS}} \cdot (1.3086 + 0.0339K - 0.0077K^2 +
     * 0.0002K^3) \f]
     *
     * @param losCondition Indicates whether the channel is LOS or NLOS.
     * @param table3gpp Pointer to the table containing the number of clusters.
     * @param kFactor The Rician K-factor, used to scale cTheta in LOS conditions.
     *
     * @return The computed cTheta value.
     *
     * @throw NS_FATAL_ERROR if the number of clusters is invalid or not found in the lookup table.
     */
    static double CalculateCtheta(ChannelCondition::LosConditionValue losCondition,
                                  const Ptr<const ParamsTable> table3gpp,
                                  double kFactor);

    /**
     * @brief Per-cluster center angles for arrival/departure in azimuth and zenith.
     */
    struct ClusterAngles
    {
        /** Azimuth of arrival (AOA) center angles per cluster [deg]. */
        DoubleVector aoa;
        /** Azimuth of departure (AOD) center angles per cluster [deg]. */
        DoubleVector aod;
        /** Zenith of arrival (ZOA) center angles per cluster [deg]. */
        DoubleVector zoa;
        /** Zenith of departure (ZOD) center angles per cluster [deg]. */
        DoubleVector zod;
    };

    /**
     * Generate cluster angles for a 3GPP channel model based on provided parameters.
     * This method computes angles such as Angle of Arrival (AoA), Angle of Departure (AoD),
     * Zenith Angle of Arrival (ZoA), and Zenith Angle of Departure (ZoD) for each cluster.
     *
     * @param channelParams Pointer to the structure containing channel parameters.
     * @param clusterPowerForAngles A vector containing the power of clusters used for angle
     * generation.
     * @param powerMax The maximum power among clusters.
     * @param cPhi Constant used for calculating azimuthal angles.
     * @param cTheta Constant used for calculating zenith angles.
     * @param lsps Reference to the structure holding large-scale parameters.
     * @param aMob Pointer to the mobility model of Node A.
     * @param bMob Pointer to the mobility model of Node B.
     * @param table3gpp Pointer to the table containing 3GPP-specific parameters.
     * @param clusterAoa Pointer to the output vector for cluster AoA values.
     * @param clusterAod Pointer to the output vector for cluster AoD values.
     * @param clusterZoa Pointer to the output vector for cluster ZoA values.
     * @param clusterZod Pointer to the output vector for cluster ZoD values.
     */
    void GenerateClusterAngles(const Ptr<const ThreeGppChannelParams> channelParams,
                               const DoubleVector& clusterPowerForAngles,
                               double powerMax,
                               double cPhi,
                               double cTheta,
                               LargeScaleParameters& lsps,
                               const Ptr<const MobilityModel> aMob,
                               const Ptr<const MobilityModel> bMob,
                               const Ptr<const ParamsTable> table3gpp,
                               DoubleVector* clusterAoa,
                               DoubleVector* clusterAod,
                               DoubleVector* clusterZoa,
                               DoubleVector* clusterZod) const;

    /**
     * @brief Calculates the per-cluster power attenuation (in dB) due to self-blocking and
     *        non-self-blocking effects, based on the 3GPP TR 38.901 blockage model
     * (Section 7.6.4.1).
     *
     * This method applies the spatial blockage attenuation to each cluster of the channel model
     * by determining whether the corresponding azimuth (AOA) and zenith (ZOA) angles fall
     * into predefined self-blocking or randomly generated non-self-blocking regions. The resulting
     * attenuation values are returned as a vector.
     *
     * - **Self-blocking** simulates the user’s own body blocking the signal. The region is defined
     *   using fixed or portrait-mode-specific parameters (Table 7.6.4.1-1).
     *
     * - **Non-self-blocking** simulates environmental blockers (e.g., furniture, walls) and
     *   is modeled using correlated random variables for blocker location and size, based on
     *   scenario-specific rules from Table 7.6.4.1-2 and spatial correlation models
     * (Tables 7.6.4.1-3, 7.6.4.1-4).
     *
     * @param [in,out] nonSelfBlocking
     *      Pointer to a 2D vector representing the existing or to-be-generated non-self-blocking
     *      regions. Each inner vector contains the parameters for a single blocker:
     *      [phi_k, x_k, theta_k, y_k, r_k], as defined in 3GPP Table 7.6.4.1-2.
     *      If empty, this function will generate a new set based on the scenario.
     *
     * @param [in,out] attenuation_dB A vector of attenuation values in decibels (dB),
     *      with one value per cluster. The attenuation includes:
     *         - 30 dB for clusters blocked by the self-blocking region.
     *         - A variable value based on the attenuation formula in Eq. (7.6-22) for non-self
     * blockers.
     *
     * @param [in] channelParams
     *      Pointer to the channel parameters, including UT location and generation time, used
     *      to calculate spatial correlation and motion updates for non-self-blocking regions.
     *
     * @param [in] clusterAOA
     *      A vector of azimuth angles of arrival (AOA) in degrees for each cluster.
     *      Must be in the range [0°, 360°].
     *
     * @param [in] clusterZOA
     *      A vector of zenith angles of arrival (ZOA) in degrees for each cluster.
     *      Must be in the range [0°, 180°].
     *
     * @note
     * - Follows the 3GPP TR 38.901 blockage model precisely, including the transformation
     *   from normally-distributed to uniformly-distributed random variables with correlated
     * updates.
     * - Uses scenario-specific correlation distances and blocker dimensions.
     * - Attenuation for non-self-blocking is computed using the formulas (7.6-22) to (7.6-27).
     * - A cluster can be affected by both self-blocking and non-self-blocking.
     *
     * @see 3GPP TR 38.901 v15.x.x or v16.x.x, Section 7.6.4.1
     */
    void CalcAttenuationOfBlockage(
        Double2DVector* nonSelfBlocking,
        DoubleVector* attenuation_dB,
        const Ptr<const ThreeGppChannelModel::ThreeGppChannelParams> channelParams,
        const DoubleVector& clusterAOA,
        const DoubleVector& clusterZOA) const;

    void UpdateClusterDelay(DoubleVector* clusterDelay,
                            DoubleVector* prevClusterDelay,
                            bool* newChannel,
                            const Ptr<const ThreeGppChannelParams> channelParams) const;

    /**
     * @brief Update cluster angles based on node mobility according to 3GPP TR 38.901.
     *
     * This function implements the spatial consistency procedure for updating cluster
     * angles when nodes move, as specified in 3GPP TR 38.901 Section 7.6.3. The cluster
     * angles are updated based on the relative velocity between transmitter and receiver
     * nodes, considering both Line-of-Sight (LOS) and Non-Line-of-Sight (NLOS) conditions.
     *
     * The update equations are based on 3GPP TR 38.901 equations (7.6-11) to (7.6-14):
     * - For LOS: Simple relative velocity calculation
     * - For NLOS: Complex velocity transformation considering cluster-specific directions
     *
     * @param channelParams Pointer to the channel parameters containing node velocities,
     *                      LOS condition, and reduced cluster number. Must contain valid
     *                      m_rxSpeed, m_txSpeed, m_losCondition, and m_reducedClusterNumber.
     * @param ioClusterAoa [in,out] Pointer to vector of cluster Angle of Arrival values
     *                     in degrees. Updated with new AOA values after mobility.
     * @param ioClusterAod [in,out] Pointer to vector of cluster Angle of Departure values
     *                     in degrees. Updated with new AOD values after mobility.
     * @param ioClusterZoa [in,out] Pointer to vector of cluster Zenith angle of Arrival
     *                     values in degrees. Updated with new ZOA values after mobility.
     * @param ioClusterZod [in,out] Pointer to vector of cluster Zenith angle of Departure
     *                     values in degrees. Updated with new ZOD values after mobility.
     * @param prevClusterDelay Vector containing the previous cluster delays in seconds.
     *                         Used in the angle update calculations. Size must equal
     *                         channelParams->m_reducedClusterNumber.
     *
     * @post Angle values are wrapped to appropriate ranges (0-360° for azimuth, 0-180° for zenith)
     * @note This function uses the channel update period (m_updatePeriod) to calculate
     *       the time-based angle changes.
     * @note For NLOS conditions, a random sign multiplier (Xn) is generated for each
     *       cluster using uniform random distribution.
     * @note The function implements 3GPP spatial consistency to maintain realistic
     *       channel behavior as nodes move in the simulation environment.
     *
     */

    void UpdateClusterAngles(const Ptr<const ThreeGppChannelParams> channelParams,
                             DoubleVector* ioClusterAoa,
                             DoubleVector* ioClusterAod,
                             DoubleVector* ioClusterZoa,
                             DoubleVector* ioClusterZod,
                             const DoubleVector& prevClusterDelay,
                             bool isSameDir) const;

    /**
     * @brief Applies blockage-based attenuation to the cluster powers according to
     *        the 3GPP TR 38.901 blockage model (Section 7.6.4.1).
     *
     * This method attenuates the power of each multipath cluster based on whether
     * it is blocked by self- or non-self-blocking regions. The attenuation is calculated
     * using the `CalcAttenuationOfBlockage()` method and applied in dB to each cluster power.
     *
     * If the `m_blockage` flag is enabled, each cluster is checked for intersection
     * with:
     *   - **Self-blocking** regions, defined based on device orientation.
     *   - **Non-self-blocking** regions, generated or updated using spatial correlation and
     *     scenario-specific parameters.
     *
     * The attenuation is subtracted from the cluster powers using:
     * \f[
     * P' = \frac{P}{10^{A_{\text{dB}}/10}}
     * \f]
     * where \f$ A_{\text{dB}} \f$ is the blockage attenuation for the cluster.
     *
     * @param [in,out] clusterPowers
     *      Pointer to the vector of cluster powers (linear scale), modified in-place
     *      with the applied attenuation (one per cluster).
     *
     * @param [out] attenuation_dB
     *      Pointer to the output vector containing the per-cluster attenuation values
     *      in dB. This is filled using `CalcAttenuationOfBlockage()`.
     *
     * @param [in] channelParams
     *      Pointer to the channel parameters (e.g., UT location, time of generation),
     *      used for computing motion-based updates to non-self-blocking regions.
     *
     * @param [in] clusterAoa
     *      Azimuth angles of arrival (AOA) for each cluster, in degrees.
     *
     * @param [in] clusterZoa
     *      Zenith angles of arrival (ZOA) for each cluster, in degrees.
     *
     * @note
     * - If `m_blockage` is false, no attenuation is applied, and `attenuation_dB` contains a single
     * 0.
     * - This function assumes the vectors are pre-sized and match `reducedClusterNumber`.
     * - Internally uses `CalcAttenuationOfBlockage()` to compute spatial intersection with
     * blockers.
     *
     * @see ThreeGppChannelModel::CalcAttenuationOfBlockage
     * @see 3GPP TR 38.901 v15.x.x or v16.x.x, Section 7.6.4.1
     */
    void ApplyAttenuationToClusterPowers(Double2DVector* nonSelfBlocking,
                                         DoubleVector* clusterPowers,
                                         DoubleVector* attenuation_dB,
                                         const Ptr<const ThreeGppChannelParams> channelParams,
                                         const DoubleVector& clusterAoa,
                                         const DoubleVector& clusterZoa) const;

    /**
     * @brief Randomly couples rays within each cluster and computes per-ray angles in radians.
     *
     * For each cluster, generates per-ray azimuth/zenith angles of arrival and departure
     * around the provided cluster center angles, using the 3GPP table parameters and
     * the standard intra-cluster offsets. The resulting per-ray angles are shuffled to
     * realize random coupling.
     *
     * @param channelParams Pointer to the channel parameters (number of reduced clusters, etc.).
     * @param table3gpp Pointer to the 3GPP parameters table (rays per cluster, coupling constants).
     * @param[out] rayAoaRadian Output per-ray azimuth-of-arrival angles [rad] as
     *                          [numClusters][raysPerCluster].
     * @param[out] rayAodRadian Output per-ray azimuth-of-departure angles [rad] as
     *                          [numClusters][raysPerCluster].
     * @param[out] rayZoaRadian Output per-ray zenith-of-arrival angles [rad] as
     *                          [numClusters][raysPerCluster].
     * @param[out] rayZodRadian Output per-ray zenith-of-departure angles [rad] as
     *                          [numClusters][raysPerCluster].
     * @param clusterAoa Input per-cluster azimuth-of-arrival center angles [deg].
     * @param clusterAod Input per-cluster azimuth-of-departure center angles [deg].
     * @param clusterZoa Input per-cluster zenith-of-arrival center angles [deg].
     * @param clusterZod Input per-cluster zenith-of-departure center angles [deg].
     *
     * @note Angles in clusterA* and clusterZ* are degrees; outputs are radians.
     * @pre ray* output containers are cleared and resized by this function.
     */
    void RandomRaysCoupling(const Ptr<const ThreeGppChannelParams> channelParams,
                            const Ptr<const ParamsTable> table3gpp,
                            Double2DVector* rayAoaRadian,
                            Double2DVector* rayAodRadian,
                            Double2DVector* rayZoaRadian,
                            Double2DVector* rayZodRadian,
                            const DoubleVector& clusterAoa,
                            const DoubleVector& clusterAod,
                            const DoubleVector& clusterZoa,
                            const DoubleVector& clusterZod) const;
    /**
     * @brief Generate cross-polarization power ratios and initial per-ray phases.
     *
     * For each cluster and ray, draws the cross-polarization power ratio (XPR) according
     * to the 3GPP parameters and initializes four polarization-dependent phases uniformly
     * in [-pi, pi]. The outputs are resized/filled by this function.
     *
     * @param[out] crossPolarizationPowerRatios Matrix [numClusters][raysPerCluster] with XPR values
     * (linear scale).
     * @param[out] clusterPhase 3D array [numClusters][raysPerCluster][4] with initial phases
     * (radians).
     * @param reducedClusterNumber Number of (possibly reduced) clusters to generate.
     * @param table3gpp Pointer to the 3GPP parameters table (uXpr, sigXpr, rays per cluster).
     */
    void GenerateCrossPolPowerRatiosAndInitialPhases(Double2DVector* crossPolarizationPowerRatios,
                                                     Double3DVector* clusterPhase,
                                                     uint8_t reducedClusterNumber,
                                                     const Ptr<const ParamsTable> table3gpp) const;
    /**
     * @brief Identify the two strongest clusters and derive sub-cluster parameters.
     *
     * Finds the indices of the two strongest clusters by power, and appends delayed copies
     * and duplicated angles for the strongest clusters as per 3GPP procedure (sub-clusters).
     *
     * @param channelParams Current channel parameters (cluster powers, counts).
     * @param table3gpp Pointer to 3GPP parameters table (cDS, rays per cluster).
     * @param[out] cluster1st Index of the strongest cluster.
     * @param[out] cluster2nd Index of the second strongest cluster.
     * @param[in,out] clusterDelay Per-cluster delays [s]; sub-cluster delays are appended.
     * @param[in,out] clusterAoa Per-cluster AOA centers [deg]; duplicated for sub-clusters.
     * @param[in,out] clusterAod Per-cluster AOD centers [deg]; duplicated for sub-clusters.
     * @param[in,out] clusterZoa Per-cluster ZOA centers [deg]; duplicated for sub-clusters.
     * @param[in,out] clusterZod Per-cluster ZOD centers [deg]; duplicated for sub-clusters.
     */
    void FindStrongestClusters(const Ptr<const ThreeGppChannelParams> channelParams,
                               const Ptr<const ParamsTable> table3gpp,
                               uint8_t* cluster1st,
                               uint8_t* cluster2nd,
                               DoubleVector* clusterDelay,
                               DoubleVector* clusterAoa,
                               DoubleVector* clusterAod,
                               DoubleVector* clusterZoa,
                               DoubleVector* clusterZod) const;

    /**
     * @brief Precomputes the sine and cosine values for angles.
     *
     * This method precomputes the sine and cosine values of specified angles,
     * storing them in a vector of pairs. The computation is based on provided
     * channel parameters.
     *
     * @param channelParams Pointer to the constant ThreeGppChannelParams object containing
     * channel-specific parameters.
     * @param cachedAngleSinCos Pointer to the vector where precomputed sine and cosine values will
     * be stored.
     */
    void PrecomputeAnglesSinCos(
        Ptr<const ThreeGppChannelParams> channelParams,
        std::vector<std::vector<std::pair<double, double>>>* cachedAngleSinCos) const;

    /**
     * @brief Generate additional Doppler terms for delayed (reflected) paths.
     *
     * Produces per-cluster Doppler parameters used to model moving scatterers
     * as per TR 37.885. Two (or four) extra terms are included to account for
     * first/second strongest cluster sub-rays.
     *
     * @param reducedClusterNumber Number of (possibly reduced) clusters.
     * @param[out] dopplerTermAlpha Per-cluster alpha terms in [-1, 1].
     * @param[out] dopplerTermD Per-cluster Doppler speed terms in [-vScatt, vScatt] [m/s].
     */
    void GenerateDopplerTerms(uint8_t reducedClusterNumber,
                              DoubleVector* dopplerTermAlpha,
                              DoubleVector* dopplerTermD) const;

    /**
     * Compute the channel matrix between two nodes a and b, and their
     * antenna arrays aAntenna and bAntenna using the procedure
     * described in 3GPP TR 38.901
     * @param channelParams the channel parameters previously generated for the pair of
     * nodes a and b
     * @param table3gpp the 3gpp parameters table
     * @param sMob the mobility model of node s
     * @param uMob the mobility model of node u
     * @param sAntenna the antenna array of node s
     * @param uAntenna the antenna array of node u
     * @return the channel realization
     */
    virtual Ptr<ChannelMatrix> GetNewChannel(Ptr<const ThreeGppChannelParams> channelParams,
                                             Ptr<const ParamsTable> table3gpp,
                                             const Ptr<const MobilityModel> sMob,
                                             const Ptr<const MobilityModel> uMob,
                                             Ptr<const PhasedArrayModel> sAntenna,
                                             Ptr<const PhasedArrayModel> uAntenna) const;

    /**
     * @brief Update channel parameters for spatial consistency using 3GPP TR 38.901 procedure A.
     *
     * This function updates existing channel parameters to maintain spatial and temporal
     * consistency when nodes move. It reuses most of the functions from GenerateChannelParameters
     * but applies spatial correlation to LSPs and temporal evolution to phases.
     *
     * Following 3GPP TR 38.901 Procedure A, the function:
     * - Updates Large Scale Parameters (LSPs)
     * - Regenerates cluster delays and powers
     * - Filters weak clusters
     * - Updates angles and phases with continuity from previous channel state
     * - Maintains Doppler terms and cross-polarization ratios for consistency
     * - Preserves phase evolution according to 3GPP specifications
     *
     * @param channelParams existing channel parameters to update
     * @param channelCondition current channel condition
     * @param aMob mobility model of device A
     * @param bMob mobility model of device B
     * @param dis2D current 2D distance between devices
     */
    void UpdateChannelParametersForConsistency(Ptr<ThreeGppChannelParams> channelParams,
                                               Ptr<const ChannelCondition> channelCondition,
                                               Ptr<const MobilityModel> aMob,
                                               Ptr<const MobilityModel> bMob) const;

    /**
     * Check if the channel params has to be updated
     * @param channelParams channel params
     * @param channelCondition the channel condition
     * @return true if the channel params has to be updated, false otherwise
     */
    bool NewChannelParamsNeeded(uint64_t channelParamsKey,
                                Ptr<const ChannelCondition> condition) const;

    /**
     * @brief Check if channel matrix needs update based on parameter changes or coherence time
     * expiration
     *
     * This method determines whether a new channel matrix should be generated by comparing
     * the generation times of the current channel parameters and existing channel matrix.
     * It also considers any potential changes in antenna configurations.
     *
     * @param channelMatrixKey Channel matrix key
     * @param channelParams Current channel parameters to check against
     * @param aAntenna Transmitting antenna model configuration
     * @param bAntenna Receiving antenna model configuration
     * @return whether the channel matrix needs an update
     */
    bool NewChannelMatrixNeeded(uint64_t channelMatrixKey,
                                Ptr<const ThreeGppChannelParams> channelParams,
                                Ptr<const PhasedArrayModel> aAntenna,
                                Ptr<const PhasedArrayModel> bAntenna);

    /**
     * @brief Check if spatial-consistency requires updating channel parameters.
     *
     * Returns true when spatial consistency is enabled and the coherence time
     * (UpdatePeriod) has elapsed since the parameters were generated.
     *
     * @param channelParams Channel parameters whose generation time is checked.
     * @return true if a parameter update is needed; false otherwise.
     */
    bool SpatialConsistencyUpdate(Ptr<const ThreeGppChannelParams> channelParams);

    /**
     * Check if the channel matrix has to be updated due to
     * changes in the number of antenna ports
     * @param aAntenna the antenna array of node a
     * @param bAntenna the antenna array of node b
     * @param channelMatrix channel matrix structure
     * @return true if the channel matrix has to be updated, false otherwise
     */
    bool AntennaSetupChanged(Ptr<const PhasedArrayModel> aAntenna,
                             Ptr<const PhasedArrayModel> bAntenna,
                             Ptr<const ChannelMatrix> channelMatrix);

    /**
     * Update the channel matrix between two devices using the consistency procedure (procedure A)
     * described in 3GPP TR 38.901
     * @param channelId the Id of the channel between the two devices
     * @param channelCondition the channel condition
     * @param sMob the s node mobility model
     * @param uMob the u node mobility model
     * @param sAntenna the s node antenna array
     * @param uAntenna the u node antenna array
     * @param uAngle the u node angle
     * @param sAngle the s node angle
     * @param dis2D the 2D distance between tx and rx
     * @param hBS the height of the BS
     * @param hUT the height of the UT
     * @return the updated channel realization
     */
    Ptr<ChannelMatrix> GetUpdatedChannel(uint32_t channelId,
                                         Ptr<const ChannelCondition> channelCondition,
                                         Ptr<const MobilityModel> sMob,
                                         Ptr<const MobilityModel> uMob,
                                         Ptr<const PhasedArrayModel> sAntenna,
                                         Ptr<const PhasedArrayModel> uAntenna,
                                         Angles& uAngle,
                                         Angles& sAngle,
                                         double dis2D,
                                         double hBS,
                                         double hUT) const;

    /**
     * Applies the blockage model A described in 3GPP TR 38.901
     * @param params the channel matrix
     * @param clusterAOA vector containing the azimuth angle of arrival for each cluster
     * @param clusterZOA vector containing the zenith angle of arrival for each cluster
     * @return vector containing the power attenuation for each cluster
     */
    DoubleVector CalcAttenuationOfBlockage(Ptr<ChannelMatrix> params,
                                           const DoubleVector& clusterAOA,
                                           const DoubleVector& clusterZOA) const;

    std::unordered_map<uint64_t, Ptr<ChannelMatrix>>
        m_channelMatrixMap; //!< map containing the channel realizations per pair of
                            //!< PhasedAntennaArray instances, the key of this map is reciprocal
                            //!< uniquely identifies a pair of PhasedAntennaArrays
    std::unordered_map<uint64_t, Ptr<ThreeGppChannelParams>>
        m_channelParamsMap; //!< map containing the common channel parameters per pair of nodes, the
                            //!< key of this map is reciprocal and uniquely identifies a pair of
                            //!< nodes
    Time m_updatePeriod;    //!< the channel update period
    double m_frequency;     //!< the operating frequency
    std::string m_scenario; //!< the 3GPP scenario
    Ptr<ChannelConditionModel> m_channelConditionModel; //!< the channel condition model
    Ptr<UniformRandomVariable> m_uniformRv;             //!< uniform random variable
    Ptr<NormalRandomVariable> m_normalRv;               //!< normal random variable
    Ptr<UniformRandomVariable>
        m_uniformRvShuffle; //!< uniform random variable used to shuffle array in GetNewChannel

    // Variable used to compute the additional Doppler contribution for the delayed
    // (reflected) paths, as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3.
    double m_vScatt; //!< value used to compute the additional Doppler contribution for the delayed
                     //!< paths
    Ptr<UniformRandomVariable> m_uniformRvDoppler; //!< uniform random variable, used to compute the
                                                   //!< additional Doppler contribution
    // parameters for the spatial consistency update
    bool m_spatial_consistency; //!< enables spatial consistency, procedure A

    // parameters for the blockage model
    bool m_blockage;               //!< enables the blockage model A
    uint16_t m_numNonSelfBlocking; //!< number of non-self-blocking regions
    bool m_portraitMode;           //!< true if portrait mode, false if landscape
    double m_blockerSpeed;         //!< the blocker speed

    static const uint8_t PHI_INDEX = 0; //!< index of the PHI value in the m_nonSelfBlocking array
    static const uint8_t X_INDEX = 1;   //!< index of the X value in the m_nonSelfBlocking array
    static const uint8_t THETA_INDEX =
        2;                            //!< index of the THETA value in the m_nonSelfBlocking array
    static const uint8_t Y_INDEX = 3; //!< index of the Y value in the m_nonSelfBlocking array
    static const uint8_t R_INDEX = 4; //!< index of the R value in the m_nonSelfBlocking array
};
} // namespace ns3

#endif /* THREE_GPP_CHANNEL_H */
