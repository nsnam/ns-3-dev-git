/*
 * Copyright (c) 2020 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 * Copyright (c) 2020 Institute for the Wireless Internet of Things,
 * Northeastern University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef MATRIX_BASED_CHANNEL_H
#define MATRIX_BASED_CHANNEL_H

// #include <complex>
#include "ns3/matrix-array.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/phased-array-model.h"
#include "ns3/vector.h"

#include <tuple>

namespace ns3
{

class MobilityModel;

/**
 * @ingroup spectrum
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
    ~MatrixBasedChannelModel() override;

    /// Type definition for vectors of doubles
    using DoubleVector = std::vector<double>;

    /// Type definition for matrices of doubles
    using Double2DVector = std::vector<DoubleVector>;

    /// Type definition for 3D matrices of doubles
    using Double3DVector = std::vector<Double2DVector>;

    using Complex2DVector = ComplexMatrixArray; //!< Create an alias for 2D complex vectors
    using Complex3DVector = ComplexMatrixArray; //!< Create an alias for 3D complex vectors

    /**
     * Data structure that stores a channel realization
     */
    struct ChannelMatrix : public SimpleRefCount<ChannelMatrix>
    {
        /**
         * Channel matrix H[u][s][n].
         */
        Complex3DVector m_channel;

        /**
         * Generation time.
         */
        Time m_generatedTime;

        /**
         * The first element is the ID of the antenna of the s-node (the
         * antenna of the transmitter when the channel was generated), the
         * second element is ID of the antenna of the u-node antenna (the
         * antenna of the receiver when the channel was generated).
         */
        std::pair<uint32_t, uint32_t> m_antennaPair;

        /**
         * The first element is the s-node ID (the transmitter when the channel was
         * generated), the second element is the u-node ID (the receiver when the
         * channel was generated).
         */
        std::pair<uint32_t, uint32_t> m_nodeIds;

        /**
         * Returns true if the ChannelMatrix object was generated
         * considering node b as transmitter and node a as receiver.
         * @param aAntennaId the ID of the antenna array of the a node
         * @param bAntennaId the ID of the antenna array of the b node
         * @return true if b is the rx and a is the tx, false otherwise
         */
        bool IsReverse(uint32_t aAntennaId, uint32_t bAntennaId) const
        {
            uint32_t sAntennaId;
            uint32_t uAntennaId;
            std::tie(sAntennaId, uAntennaId) = m_antennaPair;
            NS_ASSERT_MSG((sAntennaId == aAntennaId && uAntennaId == bAntennaId) ||
                              (sAntennaId == bAntennaId && uAntennaId == aAntennaId),
                          "This channel matrix does not represent the channel among the antenna "
                          "arrays for which are provided IDs.");
            return (sAntennaId == bAntennaId && uAntennaId == aAntennaId);
        }
    };

    /**
     * Data structure that stores channel parameters
     */
    struct ChannelParams : public SimpleRefCount<ChannelParams>
    {
        /**
         * Generation time.
         */
        Time m_generatedTime;

        /**
         * Cluster delay in nanoseconds.
         */
        DoubleVector m_delay;

        /**
         * Cluster angle angle[direction][n], where direction = 0(AOA), 1(ZOA), 2(AOD), 3(ZOD)
         * in degree.
         */
        Double2DVector m_angle;

        /**
         * Sin/cos of cluster angle angle[direction][n], where direction = 0(AOA), 1(ZOA), 2(AOD),
         * 3(ZOD) in degree.
         */
        std::vector<std::vector<std::pair<double, double>>> m_cachedAngleSincos;

        /**
         * Alpha term per cluster as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3
         * for calculating doppler.
         */
        DoubleVector m_alpha;

        /**
         * D term per cluster as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3
         * for calculating doppler.
         */
        DoubleVector m_D;

        /**
         * The first element is the s-node ID (the transmitter when the channel params were
         * generated), the second element is the u-node ID (the receiver when the channel params
         * were generated generated).
         */
        std::pair<uint32_t, uint32_t> m_nodeIds;

        /**
         * Auxiliary variable to m_cachedDelaySincos
         *
         * It is used to determine RB width (12*SCS) changes due to numerology,
         * in case the number of the RBs in the channel remains constant.
         */
        mutable double m_cachedRbWidth = 0.0;

        /**
         * Matrix array that holds the precomputed delay sincos
         */
        mutable ComplexMatrixArray m_cachedDelaySincos;

        /**
         * Destructor for ChannelParams
         */
        virtual ~ChannelParams() = default;
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
     * @param aMob mobility model of the a device
     * @param bMob mobility model of the b device
     * @param aAntenna antenna of the a device
     * @param bAntenna antenna of the b device
     * @return the channel matrix
     */
    virtual Ptr<const ChannelMatrix> GetChannel(Ptr<const MobilityModel> aMob,
                                                Ptr<const MobilityModel> bMob,
                                                Ptr<const PhasedArrayModel> aAntenna,
                                                Ptr<const PhasedArrayModel> bAntenna) = 0;

    /**
     * Returns a channel parameters structure used to obtain the channel between
     * the nodes with mobility objects passed as input parameters.
     *
     * @param aMob mobility model of the a device
     * @param bMob mobility model of the b device
     * @return the channel matrix
     */
    virtual Ptr<const ChannelParams> GetParams(Ptr<const MobilityModel> aMob,
                                               Ptr<const MobilityModel> bMob) const = 0;

    /**
     * Generate a unique value for the pair of unsigned integer of 32 bits,
     * where the order does not matter, i.e., the same value will be returned for (a,b) and (b,a).
     * @param a the first value
     * @param b the second value
     * @return return an integer representing a unique value for the pair of values
     */
    static uint64_t GetKey(uint32_t a, uint32_t b)
    {
        return (uint64_t)std::min(a, b) << 32 | std::max(a, b);
    }

    static const uint8_t AOA_INDEX = 0; //!< index of the AOA value in the m_angle array
    static const uint8_t ZOA_INDEX = 1; //!< index of the ZOA value in the m_angle array
    static const uint8_t AOD_INDEX = 2; //!< index of the AOD value in the m_angle array
    static const uint8_t ZOD_INDEX = 3; //!< index of the ZOD value in the m_angle array
};

}; // namespace ns3

#endif // MATRIX_BASED_CHANNEL_H
