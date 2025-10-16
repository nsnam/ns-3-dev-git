/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 * Copyright (c) 2026, CTTC, Centre Tecnologic de Telecomunicacions de Catalunya
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef THREE_GPP_CHANNEL_H
#define THREE_GPP_CHANNEL_H

#include "matrix-based-channel-model.h"

#include "ns3/channel-condition-model.h"

#include <unordered_map>

namespace ns3
{

class MobilityModel;

/**
 * ThreeGppChannelModel extends MatrixBasedChannelModel and represents a channel
 * model based on 3GPP specifications, including functionality for
 * managing channel conditions, scenarios, and frequency requirements.
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
     * Looks for the channel matrix associated with the aMob and bMob pair in m_channelMatrixMap.
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
     * Looks for the channel params associated with the aMob and bMob pair in
     * m_channelParamsMap. If not found, it will return a nullptr.
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

    /**
     * Wrap an (azimuth, inclination) angle pair in a valid range.
     * Specifically, inclination must be in [0, M_PI] and azimuth in [0, 2*M_PI).
     * If the inclination angle is outside its range, the azimuth angle is
     * rotated by M_PI.
     * This method aims specifically at solving the problem of generating angles at
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
    struct ThreeGppChannelParams : ChannelParams
    {
        /// contains the information about the LOS state of the channel
        ChannelCondition::LosConditionValue m_losCondition;
        /// contains the information about the O2I state of the channel
        ChannelCondition::O2iConditionValue m_o2iCondition;
        /*The following parameters are stored for spatially consistent updating. The notation is
        that of 3GPP technical reports, but it can apply also to other channel realizations*/
        /// store the blockages
        Double2DVector m_nonSelfBlocking;
        /** stores the normal variable for random angles angle[cluster][id]
         * generated for equation (7.6-11)-(7.6-14), where id follows
         * MatrixBasedChannelModel::{AOA,ZOA,AOD,ZOD}_INDEX ordering.
         */
        Double2DVector m_norRvAngles;
        /// delay spread
        double m_DS;
        /// K factor
        double m_K_factor;
        /// reduced cluster number
        uint8_t m_reducedClusterNumber;
        /** The signs of the XN per cluster -1 or +1, per NLOS used in channel consistency procedure
         */
        std::vector<int> m_clusterXnNlosSign;
        Double2DVector m_rayAodRadian;                 //!< the vector containing AOD angles
        Double2DVector m_rayAoaRadian;                 //!< the vector containing AOA angles
        Double2DVector m_rayZodRadian;                 //!< the vector containing ZOD angles
        Double2DVector m_rayZoaRadian;                 //!< the vector containing ZOA angles
        Double3DVector m_clusterPhase;                 //!< the initial random phases
        Double2DVector m_crossPolarizationPowerRatios; //!< cross-polarization power ratios
        double m_dis2D;                                //!< 2D distance between tx and rx
        double m_dis3D;                                //!< 3D distance between tx and rx
        DoubleVector m_clusterShadowing;               //!< cluster shadowing
        DoubleVector m_clusterPower;                   //!< cluster powers
        DoubleVector m_attenuation_dB;   //!< vector that stores the attenuation of the blockage
        uint8_t m_cluster1st;            //!< index of the first strongest cluster
        uint8_t m_cluster2nd;            //!< index of the second-strongest cluster
        Vector m_txSpeed;                //!< TX velocity
        Vector m_rxSpeed;                //!< RX velocity
        DoubleVector m_delayConsistency; //!< cluster delay for consistency update
        /**
         * The effective 2D displacement used to determine whether a channel-consistency update
         * can be applied, computed between consecutive endpoint positions.
         *
         * Since channel parameters are stored using a reciprocal key, endpoint-specific state is
         * handled using a canonical ordering by node ID (min ID / max ID). The displacement is
         * computed as the maximum of the two endpoint displacements since the last
         * generation/update.
         */
        double m_endpointDisplacement2D{0.0};
        /**
         * 2D displacement of the canonical relative position vector between the two endpoints
         * since the last generation/update.
         *
         * This corresponds to the change in the link geometry in the horizontal plane and is used
         * by Procedure A updates that evolve cluster-level random variables as a function of
         * displacement (e.g., per-cluster shadowing correlation).
         */
        double m_relativeDisplacement2D{0.0};
        /**
         * Position of the endpoint with the smallest node ID (canonical endpoint ordering) at the
         * time of the last channel parameter generation/update.
         */
        Vector m_lastPositionFirst;
        /**
         * Position of the endpoint with the largest node ID (canonical endpoint ordering) at the
         * time of the last channel parameter generation/update.
         */
        Vector m_lastPositionSecond;
        /**
         * Canonical 2D relative position vector (first -> second) stored at the time of the last
         * channel parameter generation/update.
         *
         * This is used to compute m_relativeDisplacement2D as the 2D displacement of the link
         * geometry between consecutive updates.
         */
        Vector2D m_lastRelativePosition2D;
    };

    /**
     * Data structure that stores the parameters of 3GPP TR 38.901, Table 7.5-6,
     * for a certain scenario
     */
    struct ParamsTable : SimpleRefCount<ParamsTable>
    {
        uint8_t m_numOfCluster = 0;   //!< Number of clusters
        uint8_t m_raysPerCluster = 0; //!< Number of rays per cluster
        double m_uLgDS = 0;           //!< Mean value of 10-base logarithm of delay spread
        double m_sigLgDS = 0; //!< Standard deviation value of 10-base logarithm of delay spread
        //! Mean value of 10-base logarithm of azimuth angle spread of departure
        double m_uLgASD = 0;
        //! Standard deviation of 10-base logarithm of azimuth angle spread of departure
        double m_sigLgASD = 0;
        //! Mean value of 10-base logarithm of azimuth angle spread of arrival
        double m_uLgASA = 0;
        //! Standard deviation of 10-base logarithm of azimuth angle spread of arrival
        double m_sigLgASA = 0;
        //! Mean value of 10-base logarithm of zenith angle spread of arrival
        double m_uLgZSA = 0;
        //! Standard deviation of 10-base logarithm of zenith angle spread of arrival
        double m_sigLgZSA = 0;
        //! Mean value of 10-base logarithm of zenith angle spread of departure
        double m_uLgZSD = 0;
        //! Standard deviation of 10-base logarithm of zenith angle spread of departure
        double m_sigLgZSD = 0;
        double m_offsetZOD = 0;              //!< Offset of a zenith angle of departure
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
        //! Correlation distance for spatial consistency (7.6.3.1-2) in meters.
        //! Default value set to 1, because V2V and NTN models documentations do not provide this
        //! parameter value
        double m_perClusterRayDcorDistance = 1;
        //! The spatial correlation distance for the random variable determining the centre of the
        //! blocker in meters; Default value set to 1, because V2V and NTN models documentations do
        //! not provide
        // this parameter value
        double m_blockerDcorDistance = 1;

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
        Ptr<const MobilityModel> aMob,
        Ptr<const MobilityModel> bMob,
        Ptr<const ChannelCondition> channelCondition) const;

  protected:
    /**
     * Update distance- and displacement-related values between two nodes.
     *
     * The function computes the current 2D and 3D distances between two mobility models.
     *
     * It also computes two 2D displacement measures with respect to previously stored state.
     * Since channel parameters are stored using a reciprocal key, endpoint-specific state is
     * handled using a canonical ordering by node ID (min ID / max ID).
     * ``endpointDisplacement2D`` is the maximum 2D displacement of the two endpoints since the last
     * channel parameter generation/update.
     * ``relativeDisplacement2D`` is the 2D displacement of the canonical relative position vector
     * between the endpoints since the last generation/update.
     *
     * @param aMob The mobility model of the first node (unordered).
     * @param bMob The mobility model of the second node (unordered).
     * @param distance2D Output parameter that stores the 2D distance between the two nodes.
     * @param distance3D Output parameter that stores the 3D distance between the two nodes.
     * @param endpointDisplacement2D Output parameter that stores the effective 2D displacement
     *        (in meters).
     * @param relativeDisplacement2D Output parameter that stores the 2D displacement (in meters)
     *        of the canonical relative position vector between the two endpoints since the last
     *        channel parameter generation/update.
     * @param lastPositionFirst Position of the endpoint with the smallest node ID at the time of
     *        the last channel parameter generation/update.
     * @param lastPositionSecond Position of the endpoint with the largest node ID at the time of
     *        the last channel parameter generation/update.
     * @param lastRelativePosition2D Previously stored canonical 2D relative position vector
     *        (first endpoint minus second endpoint) at the time of the last channel parameter
     *        generation/update.
     */
    void UpdateLinkGeometry(Ptr<const MobilityModel> aMob,
                            Ptr<const MobilityModel> bMob,
                            double* distance2D,
                            double* distance3D,
                            double* endpointDisplacement2D,
                            double* relativeDisplacement2D,
                            Vector* lastPositionFirst,
                            Vector* lastPositionSecond,
                            Vector2D* lastRelativePosition2D) const;

    /**
     * Prepare 3gpp channel parameters among the nodes a and b.
     * The function does the following steps described in 3GPP 38.901:
     *
     * Step 4: Generate large scale parameters. All LSPs are uncorrelated.
     * Step 5: Generate Delays.
     * Step 6: Generate cluster powers.
     * Step 7: Generate arrival and departure angles for both azimuth and elevation.
     * Step 8: Coupling of rays within a cluster for both azimuth and elevation
     * shuffle all the arrays to perform random coupling
     * Step 9: Generate the cross-polarization power ratios
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
        Ptr<const ChannelCondition> channelCondition,
        Ptr<const ParamsTable> table3gpp,
        Ptr<const MobilityModel> aMob,
        Ptr<const MobilityModel> bMob) const;

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
    LargeScaleParameters GenerateLSPs(const ChannelCondition::LosConditionValue losCondition,
                                      Ptr<const ParamsTable> table3gpp) const;

    /**
     * Generate the cluster delays.
     * @param DS Delay Spread
     * @param table3gpp 3GPP parameters from the table
     * @param minTau the minimum delay
     * @param clusterDelays output vector for cluster delays
     */

    void GenerateClusterDelays(const double DS,
                               Ptr<const ParamsTable> table3gpp,
                               double* minTau,
                               DoubleVector* clusterDelays) const;

    /**
     * @brief Generate per-cluster shadowing terms (in dB) as specified by 3GPP TR 38.901.
     *
     * Draws one large-scale shadowing realization per cluster using the scenario-specific
     * per-cluster shadowing standard deviation from the provided 3GPP table. These terms
     * represent the intrinsic random strength of each cluster and are later used when
     * computing and updating cluster powers.
     *
     * Behavior:
     * - The output vector is resized to the number of clusters defined by the table.
     * - Each entry is an independent zero-mean Gaussian random variable with standard
     *   deviation equal to table3gpp->m_perClusterShadowingStd (units: dB).
     *
     * @param table3gpp Pointer to the 3GPP parameters table (provides number of clusters
     *                  and per-cluster shadowing std-dev).
     * @param clusterShadowing Output vector filled with per-cluster shadowing values (dB);
     *                         size equals the number of clusters.
     */
    void GenerateClusterShadowingTerm(Ptr<const ParamsTable> table3gpp,
                                      DoubleVector* clusterShadowing) const;

    /**
     * Update shadowing per cluster by using the normalized auto correlation function R, which is
     * defined as an exponential function in ITU-R P.1816 and used by 38.901 in 7.4-5 equation for
     * the correlation of shadowing.
     * @param table3gpp The 3GPP parameters
     * @param clusterShadowing The previous values of the per-cluster shadowing.
     * @param displacementLength The displacement length between the nodes.
     */
    void UpdateClusterShadowingTerm(Ptr<const ParamsTable> table3gpp,
                                    DoubleVector* clusterShadowing,
                                    const double displacementLength) const;

    /**
     * Generate cluster powers.
     * @param clusterDelays cluster delays
     * @param DS Delay Spread
     * @param table3gpp 3GPP parameters from the table
     * @param clusterShadowing the shadowing realization per each cluster, representing the
     * intrinsic random strength of each cluster
     * @param clusterPowers output vector for cluster powers
     */
    void GenerateClusterPowers(const DoubleVector& clusterDelays,
                               const double DS,
                               Ptr<const ParamsTable> table3gpp,
                               const DoubleVector& clusterShadowing,
                               DoubleVector* clusterPowers) const;

    /**
     * Remove the clusters with less power.
     * @param clusterPowers input/output vector of cluster powers (will be modified)
     * @param clusterDelays input/output vector of cluster delays (will be modified)
     * @param losCondition LOS condition
     * @param table3gpp 3GPP parameters from the table
     * @param kFactor K factor
     * @param powerMax output for maximum power
     * @return Vector of per-cluster powers used for angle generation, after removing clusters
     * more than 25 dB below the strongest cluster.
     */
    DoubleVector RemoveWeakClusters(DoubleVector* clusterPowers,
                                    DoubleVector* clusterDelays,
                                    const ChannelCondition::LosConditionValue losCondition,
                                    Ptr<const ParamsTable> table3gpp,
                                    const double kFactor,
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
     * tau_i^{\text{LOS}} = \frac{\tau_i^{\text{NLOS}}}{c_{\tau}}
     * \f]
     * (see Equation 7.5-4)
     *
     * @param clusterDelays Pointer to a vector of initial (NLOS) cluster delays, which will be
     * updated in place.
     * @param reducedClusterNumber The number of clusters to be considered (can be < total
     * clusters).
     * @param kFactor The Rician K-factor, used to adjust the delay scaling factor `cTau`.
     */
    void AdjustClusterDelaysForLosCondition(DoubleVector* clusterDelays,
                                            const uint8_t reducedClusterNumber,
                                            const double kFactor) const;

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
     * @warning NS_FATAL_ERROR if the number of clusters is invalid or not found in the lookup
     * table.
     */
    static double CalculateCphi(const ChannelCondition::LosConditionValue losCondition,
                                Ptr<const ParamsTable> table3gpp,
                                const double kFactor);

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
     * @warning NS_FATAL_ERROR if the number of clusters is invalid or not found in the lookup
     * table.
     */
    static double CalculateCtheta(const ChannelCondition::LosConditionValue losCondition,
                                  Ptr<const ParamsTable> table3gpp,
                                  const double kFactor);

    /**
     * @brief Generate a random sign (+1 or -1) for each cluster for XN for NLOS computation for
     * channel consistency updates.
     *
     * Fills the provided vector with one sign per cluster, where each sign is
     * independently drawn from a fair Bernoulli distribution over {+1, -1}.
     * The output vector is cleared and reserved to the specified size.
     *
     * @param clusterNumber Number of clusters to generate signs for.
     * @param clusterSign Output pointer to the vector that will contain the signs
     *        (size = clusterNumber), each element being either +1 or -1.
     */
    void GenerateClusterXnNLos(const uint8_t clusterNumber, std::vector<int>* clusterSign) const;

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
     * @param clusterAngles Per-cluster angles [deg]; duplicated for sub-clusters.
     */
    void GenerateClusterAngles(Ptr<const ThreeGppChannelParams> channelParams,
                               const DoubleVector& clusterPowerForAngles,
                               double powerMax,
                               double cPhi,
                               double cTheta,
                               const LargeScaleParameters& lsps,
                               Ptr<const MobilityModel> aMob,
                               Ptr<const MobilityModel> bMob,
                               Ptr<const ParamsTable> table3gpp,
                               Double2DVector* clusterAngles) const;

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
     *
     * @param [in,out] powerAttenuation A vector of attenuation values in decibels (dB),
     *      with one value per cluster to be generated.
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
     * @param [in] table3gpp Table with 3GPP parameters
     *
     * @note
     * - Follows the 3GPP TR 38.901 blockage model precisely, including the transformation
     *   from normally-distributed to uniformly distributed random variables with correlated
     * updates.
     * - Uses scenario-specific correlation distances and blocker dimensions.
     * - Attenuation for non-self-blocking is computed using the formulas (7.6-22) to (7.6-27).
     * - A cluster can be affected by both self-blocking and non-self-blocking.
     *
     * @see 3GPP TR 38.901 v15.x.x or v16.x.x, Section 7.6.4.1
     */
    void CalcAttenuationOfBlockage(Double2DVector* nonSelfBlocking,
                                   DoubleVector* powerAttenuation,
                                   Ptr<const ThreeGppChannelParams> channelParams,
                                   const DoubleVector& clusterAOA,
                                   const DoubleVector& clusterZOA,
                                   Ptr<const ParamsTable> table3gpp) const;

    /**
     * Updates the cluster delays for the 3GPP channel model based on the
     * channel parameters and consistency requirements.
     *
     * @param clusterDelay Pointer to the vector that stores the current cluster delays to be
     * updated.
     * @param delayConsistency Pointer to the vector capturing the consistency of the delays.
     * @param channelParams Smart pointer to a constant ThreeGppChannelParams object containing
     *                      the necessary channel parameters for update computations.
     */
    void UpdateClusterDelay(DoubleVector* clusterDelay,
                            DoubleVector* delayConsistency,
                            Ptr<const ThreeGppChannelParams> channelParams) const;

    /**
     * @brief Rotate a 3D velocity vector by specified angles, around y-axis and z-axis.
     * Rotation matrices being used are the ones defined in 38.901 in Equation (7.1-2).
     * The formula being applied is the one from Procedure A, Rn,rx and Rn,ry from formula 7.6-10b
     * and 7.6-10c:
     *
     *                                      [[ 1,  0,  0 ],
     * R = Rz (alphaRad)  * Ry (betaRad) *  [ 0,  Xn, 0 ],  * Ry (gammaRad) * Rz (etaRad)
     *                                      [ 0,  0,  1 ]]
     *
     * NOTE: Currently, only x and y components are calculated, the z component is not used.
     *
     * @param alphaRad the first angle from the rotation formula R [rad]
     * @param betaRad the second angle from the rotation formula R [rad]
     * @param gammaRad the third angle from the rotation formula R [rad]
     * @param etaRad the fourth angle from the rotation formula R [rad]
     * @param speed Input velocity vector in the original (unrotated) frame [m/s].
     * @param Xn Direction sign indicator; use +1 to keep orientation, −1 to flip.
     * @return The rotated (and possibly sign-flipped) velocity vector in the target frame [m/s].
     */
    Vector ApplyVelocityRotation(double alphaRad,
                                 double betaRad,
                                 double gammaRad,
                                 double etaRad,
                                 const Vector& speed,
                                 const int Xn) const;

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
     * @param clusterAngles Cluster angles {AOA, ZOA, AOD, ZOD}
     * @param prevClusterDelay Vector containing the previous cluster delays in seconds.
     *                         Used in the angle update calculations. Size must equal
     *                         channelParams->m_reducedClusterNumber.
     */
    void UpdateClusterAngles(Ptr<const ThreeGppChannelParams> channelParams,
                             Double2DVector* clusterAngles,
                             const DoubleVector& prevClusterDelay) const;

    /**
     * Shift per-ray angles to follow updated cluster mean angles while preserving
     * intra-cluster offsets and existing random coupling.
     * This is used during Spatial Consistency Procedure A updates when cluster means
     * change, but ray coupling, XPR, and initial phases must remain the same.
     *
     * @param table3gpp Table of 3GPP parameters.
     * @param channelParams Channel parameters to update (rays stored inside will be modified).
     * @param prevClusterAngles Previous cluster angles {AOA, ZOA, AOD, ZOD}
     * @param rayAoaRadian Ray AOA in radian to be updated.
     * @param rayAodRadian Ray AOD in radian to be updated.
     * @param rayZodRadian Ray ZOD in radian to be updated.
     * @param rayZoaRadian Ray ZOA in radian to be updated.
     */
    void ShiftRayAnglesToUpdatedClusterMeans(Ptr<const ParamsTable> table3gpp,
                                             Ptr<const ThreeGppChannelParams> channelParams,
                                             const Double2DVector& prevClusterAngles,
                                             Double2DVector* rayAodRadian,
                                             Double2DVector* rayAoaRadian,
                                             Double2DVector* rayZodRadian,
                                             Double2DVector* rayZoaRadian) const;

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
     *@param [in,out] nonSelfBlocking
     *      Pointer to the 2D vector of non-self-blocking regions to be generated.
     *      Each inner vector contains the parameters for a single blocker as defined in
     *      3GPP Table 7.6.4.1-2.
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
     * @param [in] table3gpp
     *      Table with 3GPP scenario dependent parameters
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
                                         Ptr<const ThreeGppChannelParams> channelParams,
                                         const DoubleVector& clusterAoa,
                                         const DoubleVector& clusterZoa,
                                         Ptr<const ParamsTable> table3gpp) const;

    /**
     * @brief Compute per-ray angles (no shuffling), centered on cluster means.
     *
     * Initializes and fills per-ray AOA/AOD/ZOA/ZOD angles in radians for each
     * cluster according to 3GPP intra-cluster offsets. Angles are wrapped to valid
     * ranges via WrapAngles.
     *
     * @param channelParams Channel parameters (provides reduced cluster count).
     * @param table3gpp 3GPP parameters table (provides rays per cluster and spread constants).
     * @param[out] rayAoaRadian Per-ray azimuth-of-arrival angles [rad] as
     * [numClusters][raysPerCluster].
     * @param[out] rayAodRadian Per-ray azimuth-of-departure angles [rad] as
     * [numClusters][raysPerCluster].
     * @param[out] rayZoaRadian Per-ray zenith-of-arrival angles [rad] as
     * [numClusters][raysPerCluster].
     * @param[out] rayZodRadian Per-ray zenith-of-departure angles [rad] as
     * [numClusters][raysPerCluster].
     */
    void ComputeRayAngles(Ptr<const ThreeGppChannelParams> channelParams,
                          Ptr<const ParamsTable> table3gpp,
                          Double2DVector* rayAoaRadian,
                          Double2DVector* rayAodRadian,
                          Double2DVector* rayZoaRadian,
                          Double2DVector* rayZodRadian) const;

    /**
     * @brief Randomly couples rays within each cluster by shuffling per-ray angles.
     *
     * Applies in-place shuffling of per-ray AOA/AOD/ZOA/ZOD arrays independently
     * for each cluster, realizing random intra-cluster ray coupling.
     *
     * @param channelParams Channel parameters (provides reduced cluster count).
     * @param[in,out] rayAoaRadian Per-ray AOA angles [rad] as [numClusters][raysPerCluster].
     * @param[in,out] rayAodRadian Per-ray AOD angles [rad] as [numClusters][raysPerCluster].
     * @param[in,out] rayZoaRadian Per-ray ZOA angles [rad] as [numClusters][raysPerCluster].
     * @param[in,out] rayZodRadian Per-ray ZOD angles [rad] as [numClusters][raysPerCluster].
     */
    void RandomRaysCoupling(Ptr<const ThreeGppChannelParams> channelParams,
                            Double2DVector* rayAoaRadian,
                            Double2DVector* rayAodRadian,
                            Double2DVector* rayZoaRadian,
                            Double2DVector* rayZodRadian) const;
    /**
     * @brief Generate cross-polarization power ratios and initial per-ray phases.
     *
     * For each cluster and ray, draws the cross-polarization power ratio (XPR) according
     * to the 3GPP parameters and initializes four polarization-dependent phases
     * uniformly in [-pi, pi]. The outputs are resized/filled by this function.
     *
     * @param[out] crossPolarizationPowerRatios Matrix [numClusters][raysPerCluster] with
     * XPR values (linear scale).
     * @param[out] clusterPhase 3D array [numClusters][raysPerCluster][4] with initial
     * phases (radians).
     * @param reducedClusterNumber Number of (possibly reduced) clusters to generate.
     * @param table3gpp Pointer to the 3GPP parameters table (uXpr, sigXpr, rays per
     * cluster).
     */
    void GenerateCrossPolPowerRatiosAndInitialPhases(Double2DVector* crossPolarizationPowerRatios,
                                                     Double3DVector* clusterPhase,
                                                     const uint8_t reducedClusterNumber,
                                                     Ptr<const ParamsTable> table3gpp) const;
    /**
     * @brief Identify the two strongest base clusters and append their derived subclusters.
     *
     * Finds the indices of the strongest and second-strongest clusters (by power) within the
     * base (reduced) cluster set, and appends the corresponding subclusters by extending the
     * per-cluster delay/angle vectors according to the 3GPP strongest-cluster subclustering rule.
     * The associated Doppler terms (alpha and dTerm) are appended in the same order,
     * inheriting the parent cluster values for each appended subcluster.
     *
     * @param channelParams Current channel parameters (cluster powers and reduced cluster count).
     * @param table3gpp 3GPP parameters table (e.g., cDS).
     * @param[out] cluster1st Index of the strongest base cluster.
     * @param[out] cluster2nd Index of the second-strongest base cluster.
     * @param[in,out] clusterDelay Per-cluster delays [s]; subcluster delays are appended.
     * @param[in,out] angles Per-cluster angles [deg]; subcluster angles are appended/duplicated.
     * @param[in,out] alpha Per-cluster Doppler alpha terms; subcluster terms are appended.
     * @param[in,out] dTerm Per-cluster Doppler D terms; subcluster terms are appended.
     * @param[in,out] clusterPower Per-cluster power; subcluster powers are appended.
     */
    void FindStrongestClusters(Ptr<const ThreeGppChannelParams> channelParams,
                               Ptr<const ParamsTable> table3gpp,
                               uint8_t* cluster1st,
                               uint8_t* cluster2nd,
                               DoubleVector* clusterDelay,
                               Double2DVector* angles,
                               DoubleVector* alpha,
                               DoubleVector* dTerm,
                               DoubleVector* clusterPower) const;

    /**
     * @brief Trim per-cluster vectors back to the base (reduced) cluster set.
     *
     * After channel generation/update, two or four derived subclusters may be appended
     * at the end of the per-cluster vectors (to represent the additional strongest-cluster
     * subclusters). This helper removes any such appended tail so that the vectors
     * contain exactly @p reducedClusterNumber base clusters.
     *
     * @param reducedClusterNumber Number of base clusters (after weak-cluster removal).
     * @param delay Per-cluster delays (tail subclusters, if present, are removed).
     * @param angles Per-cluster angles (AOA/ZOA/AOD/ZOD; tail subclusters, if present, are
     * removed).
     * @param cachedAngleSincos Optional cached sin/cos pairs per cluster angle (tail
     * subclusters, if present, are removed).
     * @param alpha Optional Doppler alpha terms per cluster (tail subclusters, if present, are
     * removed).
     * @param dTerm Optional Doppler D terms per cluster (tail subclusters, if present, are
     * removed).
     * @param clusterPower Per cluster power (tail subclusters, if present, are removed).
     */
    void TrimToBaseClusters(const uint8_t reducedClusterNumber,
                            DoubleVector* delay,
                            Double2DVector* angles,
                            std::vector<std::vector<std::pair<double, double>>>* cachedAngleSincos,
                            DoubleVector* alpha,
                            DoubleVector* dTerm,
                            DoubleVector* clusterPower) const;

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
     * as per TR 37.885. v15.3.0, Sec. 6.2.3. These terms account for an additional
     * Doppler contribution due to the presence of moving objects in the surrounding
     * environment, such as in vehicular scenarios. This contribution is applied
     * only to the delayed (reflected) paths and must be properly configured by
     * setting the value of m_vScatt, which is defined as "maximum speed of the
     * vehicle in the layout".
     *
     * @param reducedClusterNumber Number of (possibly reduced) clusters.
     * @param[out] dopplerTermAlpha Per-cluster alpha terms in [-1, 1].
     * @param[out] dopplerTermD Per-cluster Doppler speed terms in [-vScatt, vScatt] [m/s].
     */
    void GenerateDopplerTerms(const uint8_t reducedClusterNumber,
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
                                             Ptr<const MobilityModel> sMob,
                                             Ptr<const MobilityModel> uMob,
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
     */
    void UpdateChannelParameters(Ptr<ThreeGppChannelParams> channelParams,
                                 Ptr<const ChannelCondition> channelCondition,
                                 Ptr<const MobilityModel> aMob,
                                 Ptr<const MobilityModel> bMob) const;

    /**
     * Compute the effective 2D displacement of a link with respect to previously stored endpoint
     * positions.
     *
     * The channel parameters are stored using a reciprocal key, therefore endpoint-specific state
     * is handled using a canonical ordering by node ID (min ID / max ID). The returned endpoint
     * displacement is the maximum of the two endpoint displacements, so that the channel can evolve
     * even when the change in inter-node distance between consecutive updates is close to zero
     * (e.g., parallel motion in V2V).
     *
     * @param aMob The mobility model of the first node (unordered).
     * @param bMob The mobility model of the second node (unordered).
     * @param lastPositionFirst Previously stored position of the endpoint with the smallest node
     * ID.
     * @param lastPositionSecond Previously stored position of the endpoint with the largest node
     * ID.
     * @return The effective 2D displacement (in meters).
     */
    double ComputeEndpointDisplacement2d(Ptr<const MobilityModel> aMob,
                                         Ptr<const MobilityModel> bMob,
                                         const Vector& lastPositionFirst,
                                         const Vector& lastPositionSecond) const;

    /**
     * Check if the channel params has to be updated
     * @param channelParamsKey the channel params key
     * @param condition the pointer to the channel condition instance
     * @param aMob mobility model of the device a
     * @param bMob mobility model of the device b
     * @return true if the channel params has to be updated, false otherwise
     */
    bool NewChannelParamsNeeded(const uint64_t channelParamsKey,
                                Ptr<const ChannelCondition> condition,
                                Ptr<const MobilityModel> aMob,
                                Ptr<const MobilityModel> bMob) const;

    /**
     * @brief Check if channel matrix needs update based on parameter changes or
     * the update period expired.
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
                                Ptr<const PhasedArrayModel> bAntenna) const;

    /**
     * @brief Check if spatial-consistency requires updating channel parameters.
     *
     * Returns true when spatial consistency is enabled, and the update period
     * (UpdatePeriod) has elapsed since the parameters were generated.
     *
     * @param channelParams Channel parameters whose generation time is checked.
     * @param aMob mobility model of the device a
     * @param bMob mobility model of the device b
     * @return true if a parameter update is needed; false otherwise.
     */
    bool ChannelUpdateNeeded(Ptr<const ThreeGppChannelParams> channelParams,
                             Ptr<const MobilityModel> aMob,
                             Ptr<const MobilityModel> bMob) const;

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
                             Ptr<const ChannelMatrix> channelMatrix) const;

    /**
     * map containing the channel realizations per a pair of
     * PhasedAntennaArray instances; the key of this map is reciprocal
     * uniquely identifies a pair of PhasedAntennaArrays
     */
    std::unordered_map<uint64_t, Ptr<ChannelMatrix>> m_channelMatrixMap;
    /**
     * map containing the common channel parameters per a pair of nodes, the
     * key of this map is reciprocal and uniquely identifies a pair of
     * nodes
     */
    std::unordered_map<uint64_t, Ptr<ThreeGppChannelParams>> m_channelParamsMap;
    /// the channel update period enables spatial consistency, procedure A
    Time m_updatePeriod;
    /// the operating frequency
    double m_frequency;
    /// the 3GPP scenario
    std::string m_scenario;
    /// the channel condition model
    Ptr<ChannelConditionModel> m_channelConditionModel;
    /// uniform random variable
    Ptr<UniformRandomVariable> m_uniformRv;
    /// normal random variable
    Ptr<NormalRandomVariable> m_normalRv;
    /// uniform random variable used to shuffle an array in GetNewChannel
    Ptr<UniformRandomVariable> m_uniformRvShuffle;
    /**
     * Variable used to compute the additional Doppler contribution for the delayed
     * (reflected) paths, as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3.
     */
    double m_vScatt;
    /**
     * Uniform random variable, used to compute the additional Doppler contribution
     */
    Ptr<UniformRandomVariable> m_uniformRvDoppler;
    // parameters for the blockage model
    /// enables the blockage model A
    bool m_blockage;
    /// number of non-self-blocking regions
    uint16_t m_numNonSelfBlocking;
    /// true if portrait mode, false if landscape
    bool m_portraitMode;
    /// the blocker speed
    double m_blockerSpeed;

    /// index of the PHI value in the m_nonSelfBlocking array
    static constexpr uint8_t PHI_INDEX = 0;
    /// index of the X value in the m_nonSelfBlocking array
    static constexpr uint8_t X_INDEX = 1;
    /// index of the THETA value in the m_nonSelfBlocking array
    static constexpr uint8_t THETA_INDEX = 2;
    /// index of the Y value in the m_nonSelfBlocking array
    static constexpr uint8_t Y_INDEX = 3;
    /// index of the R value in the m_nonSelfBlocking array
    static constexpr uint8_t R_INDEX = 4;
};
} // namespace ns3

#endif /* THREE_GPP_CHANNEL_H */
