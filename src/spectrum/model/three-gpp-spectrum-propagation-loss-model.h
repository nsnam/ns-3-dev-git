/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H
#define THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H

#include "matrix-based-channel-model.h"
#include "phased-array-spectrum-propagation-loss-model.h"

#include "ns3/random-variable-stream.h"

#include <complex.h>
#include <map>
#include <unordered_map>

class ThreeGppCalcLongTermMultiPortTest;
class ThreeGppMimoPolarizationTest;

namespace ns3
{

class NetDevice;

/**
 * @ingroup spectrum
 * @brief 3GPP Spectrum Propagation Loss Model
 *
 * This class models the frequency dependent propagation phenomena in the way
 * described by 3GPP TR 38.901 document. The main method is DoCalcRxPowerSpectralDensity,
 * which takes as input the power spectral density (PSD) of the transmitted signal,
 * the mobility models of the transmitting node and receiving node, and
 * returns the PSD of the received signal.
 *
 * @see MatrixBasedChannelModel
 * @see PhasedArrayModel
 * @see ChannelCondition
 */
class ThreeGppSpectrumPropagationLossModel : public PhasedArraySpectrumPropagationLossModel
{
    friend class ::ThreeGppCalcLongTermMultiPortTest;
    friend class ::ThreeGppMimoPolarizationTest;

  public:
    /**
     * Constructor
     */
    ThreeGppSpectrumPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppSpectrumPropagationLossModel() override;

    void DoDispose() override;

    /**
     * Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Set the channel model object
     * @param channel a pointer to an object implementing the MatrixBasedChannelModel interface
     */
    void SetChannelModel(Ptr<MatrixBasedChannelModel> channel);

    /**
     * Get the channel model object
     * @return a pointer to the object implementing the MatrixBasedChannelModel interface
     */
    Ptr<MatrixBasedChannelModel> GetChannelModel() const;

    /**
     * Sets the value of an attribute belonging to the associated
     * MatrixBasedChannelModel instance
     * @param name name of the attribute
     * @param value the attribute value
     */
    void SetChannelModelAttribute(const std::string& name, const AttributeValue& value);

    /**
     * Returns the value of an attribute belonging to the associated
     * MatrixBasedChannelModel instance
     * @param name name of the attribute
     * @param value where the result should be stored
     */
    void GetChannelModelAttribute(const std::string& name, AttributeValue& value) const;

    /**
     * @brief Computes the received PSD.
     *
     * This function computes the received PSD by applying the 3GPP fast fading
     * model and the beamforming gain.
     * In particular, it retrieves the matrix representing the channel between
     * node a and node b, computes the corresponding long term component, i.e.,
     * the product between the cluster matrices and the TX and RX beamforming
     * vectors (w_rx^T H^n_ab w_tx), and accounts for the Doppler component and
     * the propagation delay.
     * To reduce the computational load, the long term component associated with
     * a certain channel is cached and recomputed only when the channel realization
     * is updated, or when the beamforming vectors change.
     *
     * @param spectrumSignalParams spectrum signal tx parameters
     * @param a first node mobility model
     * @param b second node mobility model
     * @param aPhasedArrayModel the antenna array of the first node
     * @param bPhasedArrayModel the antenna array of the second node
     * @return the received PSD
     */
    Ptr<SpectrumSignalParameters> DoCalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> spectrumSignalParams,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel) const override;

  protected:
    /**
     * Data structure that stores the long term component for a tx-rx pair
     */
    struct LongTerm : public SimpleRefCount<LongTerm>
    {
        Ptr<const MatrixBasedChannelModel::Complex3DVector>
            m_longTerm; //!< vector containing the long term component for each cluster
        Ptr<const MatrixBasedChannelModel::ChannelMatrix>
            m_channel; //!< pointer to the channel matrix used to compute the long term
        PhasedArrayModel::ComplexVector
            m_sW; //!< the beamforming vector for the node s used to compute the long term
        PhasedArrayModel::ComplexVector
            m_uW; //!< the beamforming vector for the node u used to compute the long term
    };

    /**
     * Computes the frequency-domain channel matrix with the dimensions numRxPorts*numTxPorts*numRBs
     * @param inPsd the input PSD
     * @param longTerm the long term component
     * @param channelMatrix the channel matrix structure
     * @param channelParams the channel parameters, including delays
     * @param doppler the doppler for each cluster
     * @param numTxPorts the number of antenna ports at the transmitter
     * @param numRxPorts the number of antenna ports at the receiver
     * @param isReverse true if params and longTerm were computed with RX->TX switched
     * @return 3D spectrum channel matrix with dimensions numRxPorts * numTxPorts * numRBs
     */
    Ptr<MatrixBasedChannelModel::Complex3DVector> GenSpectrumChannelMatrix(
        Ptr<SpectrumValue> inPsd,
        Ptr<const MatrixBasedChannelModel::Complex3DVector> longTerm,
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
        Ptr<const MatrixBasedChannelModel::ChannelParams> channelParams,
        PhasedArrayModel::ComplexVector doppler,
        uint8_t numTxPorts,
        uint8_t numRxPorts,
        bool isReverse) const;

    /**
     * Get the operating frequency
     * @return the operating frequency in Hz
     */
    double GetFrequency() const;

    /**
     * Looks for the long term component in m_longTermMap. If found, checks
     * whether it has to be updated. If not found or if it has to be updated,
     * calls the method CalcLongTerm to compute it.
     * @param channelMatrix the channel matrix
     * @param aPhasedArrayModel the antenna array of the tx device
     * @param bPhasedArrayModel the antenna array of the rx device
     * @return vector containing the long term component for each cluster
     */
    Ptr<const MatrixBasedChannelModel::Complex3DVector> GetLongTerm(
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel) const;
    /**
     * Computes the long term component
     * @param channelMatrix the channel matrix H
     * @param sAnt the pointer to the antenna of the s device
     * @param uAnt the pointer to the antenna of the u device
     * @return the long term component
     */
    Ptr<const MatrixBasedChannelModel::Complex3DVector> CalcLongTerm(
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
        Ptr<const PhasedArrayModel> sAnt,
        Ptr<const PhasedArrayModel> uAnt) const;

    /**
     * @brief Computes a longTerm component from a specific port of s device to the
     * specific port of u device and for a specific cluster index
     * @param params The params that include the channel matrix
     * @param sAnt pointer to first antenna
     * @param uAnt uAnt pointer to second antenna
     * @param sPortIdx the port index of the s device
     * @param uPortIdx the port index of the u device
     * @param cIndex the cluster index
     * @return longTerm component for port pair and for a specific cluster index
     */
    std::complex<double> CalculateLongTermComponent(
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> params,
        Ptr<const PhasedArrayModel> sAnt,
        Ptr<const PhasedArrayModel> uAnt,
        uint16_t sPortIdx,
        uint16_t uPortIdx,
        uint16_t cIndex) const;

    /**
     * @brief Computes the beamforming gain and applies it to the TX PSD
     * @param params SpectrumSignalParameters holding TX PSD
     * @param longTerm the long term component
     * @param channelMatrix the channel matrix structure
     * @param channelParams the channel params structure
     * @param sSpeed the speed of the first node
     * @param uSpeed the speed of the second node
     * @param numTxPorts the number of the ports of the first node
     * @param numRxPorts the number of the porst of the second node
     * @param isReverse indicator that tells whether the channel matrix is reverse
     * @return
     */
    Ptr<SpectrumSignalParameters> CalcBeamformingGain(
        Ptr<const SpectrumSignalParameters> params,
        Ptr<const MatrixBasedChannelModel::Complex3DVector> longTerm,
        Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
        Ptr<const MatrixBasedChannelModel::ChannelParams> channelParams,
        const Vector& sSpeed,
        const Vector& uSpeed,
        uint8_t numTxPorts,
        uint8_t numRxPorts,
        bool isReverse) const;

    int64_t DoAssignStreams(int64_t stream) override;

    mutable std::unordered_map<uint64_t, Ptr<const LongTerm>>
        m_longTermMap;                           //!< map containing the long term components
    Ptr<MatrixBasedChannelModel> m_channelModel; //!< the model to generate the channel matrix
};
} // namespace ns3

#endif /* THREE_GPP_SPECTRUM_PROPAGATION_LOSS_H */
