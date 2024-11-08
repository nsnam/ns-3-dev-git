/*
 * Copyright (c) 2022 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef TWO_RAY_SPECTRUM_PROPAGATION_LOSS_H
#define TWO_RAY_SPECTRUM_PROPAGATION_LOSS_H

#include "phased-array-spectrum-propagation-loss-model.h"
#include "spectrum-signal-parameters.h"

#include "ns3/channel-condition-model.h"

#include <map>

class FtrFadingModelAverageTest;
class ArrayResponseTest;
class OverallGainAverageTest;

namespace ns3
{

class NetDevice;

/**
 * @ingroup spectrum
 * @brief Two Ray Spectrum Propagation Loss Model
 *
 * This class implements a performance-oriented channel and beamforming models alternative to the
 * ThreeGppSpectrumPropagationLossModel and ThreeGppChannelModel. The main method is
 * DoCalcRxPowerSpectralDensity, which takes as input the power spectral density (PSD) of the
 * transmitted signal, the mobility models of the transmitting node and receiving node, and returns
 * the PSD of the received signal.
 *
 * The fading model is calibrated with the ThreeGppChannelModel, aiming to provide an end-to-end
 * channel gain which matches as much as possible the TR 38.901-based channel model. Therefore, this
 * model is to be used for large-scale simulations of MIMO wireless networks, for which the
 * ThreeGppChannelModel would prove too computationally intensive. The target applicability of this
 * model, in terms of frequency range, is 0.5-100 GHz.
 *
 * @see PhasedArrayModel
 * @see ChannelCondition
 */
class TwoRaySpectrumPropagationLossModel : public PhasedArraySpectrumPropagationLossModel
{
    // Test classes needs access to CalcBeamformingGain and GetFtrFastFading
    friend class ::FtrFadingModelAverageTest;
    friend class ::ArrayResponseTest;
    friend class ::OverallGainAverageTest;

  public:
    /**
     * Struct holding the Fluctuating Two Ray fast-fading model parameters
     */
    struct FtrParams
    {
        /**
         * Default constructor, requiring the Fluctuating Two Ray fading model parameters as
         * arguments.
         *
         * @param m the m parameter of the FTR fading model, which represents the "Alpha" and "Beta"
         * parameters of the Gamma-distributed random variable used to sample the power of the
         * reflected components \param sigma the sigma parameter of the FTR fading model, which
         * expresses the power of the diffuse real and imaginary components \param k the k parameter
         * of the FTR fading model, which represents the ratio of the average power of the dominant
         * components to the power of the remaining diffuse multipath \param delta the delta
         * parameter of the FTR fading model, which express expressing how similar to each other are
         * the average received powers of the specular components
         */
        FtrParams(double m, double sigma, double k, double delta)
        {
            // Make sure the parameter values belong to the proper domains
            NS_ASSERT(delta >= 0.0 && delta <= 1.0);

            m_m = m;
            m_sigma = sigma;
            m_k = k;
            m_delta = delta;
        }

        /**
         * Delete no-arguments default constructor.
         */
        FtrParams() = delete;

        /**
         * Parameter m for the Gamma variable. Used both as the shape and rate parameters.
         */
        double m_m = 0;

        /**
         * Parameter sigma. Used as the variance of the amplitudes of the normal diffuse components.
         */
        double m_sigma = 0;

        /**
         * Parameter K. Expresses ratio between dominant specular components and diffuse components.
         */
        double m_k = 0;

        /**
         * Parameter delta [0, 1]. Expresses how similar the amplitudes of the two dominant specular
         * components are.
         */
        double m_delta = 0;
    };

    /**
     * Tuple collecting vectors of carrier frequencies and FTR fading model parameters, encoded as a
     * FtrParams struct
     */
    using CarrierFrequencyFtrParamsTuple = std::tuple<std::vector<double>, std::vector<FtrParams>>;

    /**
     * Nested map associating 3GPP scenario and LosCondition to the corresponding tuple of carrier
     * frequencies and corresponding fitted FTR parameters
     */
    using FtrParamsLookupTable =
        std::map<std::string,
                 std::map<ChannelCondition::LosConditionValue, CarrierFrequencyFtrParamsTuple>>;

    /**
     * Constructor
     */
    TwoRaySpectrumPropagationLossModel();

    /**
     * Destructor
     */
    ~TwoRaySpectrumPropagationLossModel() override;

    void DoDispose() override;

    /**
     * Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Sets the propagation scenario
     * @param scenario the propagation scenario
     */
    void SetScenario(const std::string& scenario);

    /**
     * Sets the center frequency of the model
     * @param f the center frequency in Hz
     */
    void SetFrequency(double f);

    /**
     * @brief Compute the received PSD.
     *
     * This function computes the received PSD by applying the Fluctuating Two-Ray (FTR)
     * fast fading model and the beamforming gain.
     * In particular, the beamforming gain is computed as the combination of the single-element
     * and the array gains.
     * The path loss and shadowing are to be computed separately, using the
     * ThreeGppPropagationLossModel class.
     *
     * @param txPsd the PSD of the transmitted signal
     * @param a first node mobility model
     * @param b second node mobility model
     * @param aPhasedArrayModel the antenna array of the first node
     * @param bPhasedArrayModel the antenna array of the second node
     * @return SpectrumSignalParameters including the PSD of the received signal
     */
    Ptr<SpectrumSignalParameters> DoCalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> txPsd,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel) const override;

  protected:
    int64_t DoAssignStreams(int64_t stream) override;

  private:
    /**
     * Retrieves the LOS condition associated to the specified mobility models
     *
     * @param a the mobility model of the first node
     * @param b the mobility model of the second node
     * @return the LOS condition of the link between a and b
     */
    ChannelCondition::LosConditionValue GetLosCondition(Ptr<const MobilityModel> a,
                                                        Ptr<const MobilityModel> b) const;

    /**
     * Retrieves the FTR fading model parameters related to the carrier frequency and LOS condition.
     *
     * The calibration has been undertaken using the 3GPP 38.901 as a reference. The latter's
     * small-scale fading distributions depend on the scenario (UMa, UMi, RMa, InH), the LOS
     * condition (LOS/NLOS) and the carrier frequency. Therefore, the output of the calibration is a
     * map associating such simulation parameters to the specific FTR parameters. Specifically, such
     * parameters represent the FTR distribution yielding channel realizations which exhibit the
     * closest statistics to the small-scale fading obtained when using the 3GPP 38.901 model. The
     * estimation relies on reference curves obtained by collecting multiple 38.901 channel
     * realizations and computing the corresponding end-to-end channel gain by setting the speed of
     * the TX and RX pair to 0, disabling the shadowing and fixing the LOS condition. In such a way,
     * any variation of the received power around the mean is given by the small-scale fading only.
     * Finally, the reference ECDF of the channel gains is compared to the ECDF obtained from the
     * FTR distribution using different values of the parameters. The parameters which provide the
     * best fit (in a goodness-of-fit sense) are kept for the specific scenario, LOS condition and
     * carrier frequency. The goodness of the fit is measured using the well-known Anderson-Darling
     * metric.
     *
     * For additional details, please refer to
     * src/spectrum/examples/three-gpp-two-ray-channel-calibration.cc and
     * src/spectrum/utils/two-ray-to-three-gpp-ch-calibration.py, which show how to obtain the
     * reference curves and then estimate the FTR parameters based on the reference data,
     * respectively.
     *
     * @param a first node mobility model
     * @param b second node mobility model
     * @return the corresponding FTR model parameters
     */
    FtrParams GetFtrParameters(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const;

    /**
     * Compute the stochastic power gain due to the fast fading, modeled according to the
     * Fluctuating Two-Ray Model.
     *
     * Specifically, the amplitude of the random fading term is sampled as:
     *
     * TODO: Currently the equations are not rendered, but neither are other formulas. Fixed in the
     * mainline, so this should also render properly after a rebase will be done
     *
     * \f$ V_r = \sqrt{\zeta} V_1 \exp(j \phi_1) + \sqrt{\zeta} V_2 \exp(j \phi_2) + X + jY \f$,
     * where \f$ \zeta \f$ is a unit-mean Gamma distributed random variable of shape and rate \f$ m
     * \f$, \f$ X \f$ and \f$ Y \f$ are zero-mean Gaussian random variables of variance \f$ \sigma^2
     * \f$, \f$ \phi_n \f$ is a uniform random variable over \f$ \left[ 0, 2\pi \right] \f$, \f$ V_1
     * \f$ and \f$ V_2 \f$ are the constant amplitudes of the reflected components.
     *
     * See J. M. Romero-Jerez, F. J. Lopez-Martinez, J. F. Paris and A. Goldsmith, "The Fluctuating
     * Two-Ray Fading Model for mmWave Communications," 2016 IEEE Globecom Workshops (GC Wkshps) for
     * further details on such model.
     *
     * @param params the FTR fading model parameters
     * @return the stochastic power gain due to the fast fading
     */
    double GetFtrFastFading(const FtrParams& params) const;

    /**
     * Compute the beamforming gain by combining single-element and array gains.
     *
     * Computes the overall beamforming and array gain, assuming analog beamforming
     * both at the transmitter and at the receiver and arbitrary single-element
     * radiation patterns. The computation is performed following Rebato, Mattia, et al.
     * "Study of realistic antenna patterns in 5G mmwave cellular scenarios.",
     * 2018 IEEE International Conference on Communications (ICC). IEEE, 2018.
     *
     * Additionally, whenever the link is in NLOS a penalty factor is introduced, to take into
     * account for the possible misalignment of the beamforming vectors due to the lack of a
     * dominant multipath component. See Kulkarni, Mandar N., Eugene Visotsky, and Jeffrey G.
     * Andrews. "Correction factor for analysis of MIMO wireless networks with highly directional
     * beamforming." IEEE Wireless Communications Letters 7.5 (2018) for further details on this
     * approach.
     *
     * @param a first node mobility model
     * @param b second node mobility model
     * @param aPhasedArrayModel the antenna array of the first node
     * @param bPhasedArrayModel the antenna array of the second node
     * @return the beamforming gain
     */
    double CalcBeamformingGain(Ptr<const MobilityModel> a,
                               Ptr<const MobilityModel> b,
                               Ptr<const PhasedArrayModel> aPhasedArrayModel,
                               Ptr<const PhasedArrayModel> bPhasedArrayModel) const;

    /**
     * Get the index of the closest carrier frequency for which the FTR estimated parameters are
     * available.
     *
     * @param frequencies the vector of carrier frequencies which have been calibrated
     * @param targetFc  the carrier frequency of the current simulation
     * @return the index of frequencies representing the argmin over frequencies of
     * abs(frequencies[index] - targetFc)
     */
    std::size_t SearchClosestFc(const std::vector<double>& frequencies, double targetFc) const;

    /**
     * The operating frequency
     */
    double m_frequency;

    /**
     * Random variable used to sample the uniform distributed phases of the FTR specular components.
     */
    Ptr<UniformRandomVariable> m_uniformRv;

    /**
     * Random variable used to sample the normal distributed amplitudes of the FTR diffuse
     * components.
     */
    Ptr<NormalRandomVariable> m_normalRv;

    /**
     * Random variable used to sample the Nakagami distributed amplitude of the FTR specular
     * components.
     */
    Ptr<GammaRandomVariable> m_gammaRv;

    std::string m_scenario; //!< the 3GPP scenario

    /**
     * Channel condition model used to retrieve the LOS/NLOS condition of the communicating
     * endpoints
     */
    Ptr<ChannelConditionModel> m_channelConditionModel;
};

} // namespace ns3

#endif /* TWO_RAY_SPECTRUM_PROPAGATION_LOSS_H */
