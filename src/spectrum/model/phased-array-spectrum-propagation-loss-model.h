/*
 * Copyright (c) 2021 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H
#define PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H

#include "spectrum-value.h"

#include "ns3/mobility-model.h"
#include "ns3/object.h"
#include "ns3/phased-array-model.h"

namespace ns3
{

struct SpectrumSignalParameters;

/**
 * @ingroup spectrum
 *
 * @brief spectrum-aware propagation loss model that is
 * compatible with PhasedArrayModel type of ns-3 antenna
 *
 * Interface for propagation loss models to be adopted when
 * transmissions are modeled with a power spectral density by means of
 * the SpectrumValue class, and when PhasedArrayModel type of atenna
 * is being used for TX and RX.
 *
 */
class PhasedArraySpectrumPropagationLossModel : public Object
{
  public:
    PhasedArraySpectrumPropagationLossModel();
    ~PhasedArraySpectrumPropagationLossModel() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Used to chain various instances of PhasedArraySpectrumPropagationLossModel
     *
     * @param next
     */
    void SetNext(Ptr<PhasedArraySpectrumPropagationLossModel> next);

    /**
     * Return the pointer to the next PhasedArraySpectrumPropagationLossModel, if any.
     *
     * @return Pointer to the next model, if any
     */
    Ptr<PhasedArraySpectrumPropagationLossModel> GetNext() const;

    /**
     * Calculate the received PSD using the beamforming vectors currently set on the
     * phased antenna arrays of the sender and of the receiver.
     *
     * @param txPsd the spectrum signal parameters.
     * @param a sender mobility
     * @param b receiver mobility
     * @param aPhasedArrayModel the instance of the phased antenna array of the sender
     * @param bPhasedArrayModel the instance of the phased antenna array of the receiver
     *
     * @return SpectrumSignalParameters in which is updated the PSD to contain
     * a set of values Vs frequency representing the received
     * power in the same units used for the txPower parameter,
     * and additional chanSpectrumMatrix is computed to support MIMO systems.
     */
    Ptr<SpectrumSignalParameters> CalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> txPsd,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel) const;

    /**
     * Calculate the received PSD using the given beamforming vectors, which need not be
     * the ones currently set on the phased antenna arrays. The arrays provide the
     * geometry, the element patterns and the port layout only.
     *
     * The sender's vector should be the one in effect when the signal was transmitted and
     * the receiver's the one in effect when it is received. If the transmission and the
     * reception are handled in separate events, the caller must capture the sender's
     * vector at transmission time and pass it here rather than read the array when the
     * signal arrives, since the array may have been re-steered in between.
     *
     * @param txPsd the spectrum signal parameters.
     * @param a sender mobility
     * @param b receiver mobility
     * @param aPhasedArrayModel the instance of the phased antenna array of the sender
     * @param bPhasedArrayModel the instance of the phased antenna array of the receiver
     * @param aBeamformingVector the beamforming vector applied to the sender's array
     * @param bBeamformingVector the beamforming vector applied to the receiver's array
     *
     * @return SpectrumSignalParameters in which is updated the PSD to contain
     * a set of values Vs frequency representing the received
     * power in the same units used for the txPower parameter,
     * and additional chanSpectrumMatrix is computed to support MIMO systems.
     */
    Ptr<SpectrumSignalParameters> CalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> txPsd,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel,
        const PhasedArrayModel::ComplexVector& aBeamformingVector,
        const PhasedArrayModel::ComplexVector& bBeamformingVector) const;

    /**
     * If this loss model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.  If there are PhasedArraySpectrumPropagationLossModels
     * chained together, this method will also assign streams to the
     * downstream models.
     *
     * @param stream the stream index offset start
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void DoDispose() override;
    /**
     * Assign a fixed random variable stream number to the random variables used by this model.
     *
     * Subclasses must implement this; those not using random variables can return zero.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    virtual int64_t DoAssignStreams(int64_t stream) = 0;

  private:
    /**
     *
     * @param params the spectrum signal parameters.
     * @param a sender mobility
     * @param b receiver mobility
     * @param aPhasedArrayModel the instance of the phased antenna array of the sender
     * @param bPhasedArrayModel the instance of the phased antenna array of the receiver
     * @param aBeamformingVector the beamforming vector applied to the sender's array
     * @param bBeamformingVector the beamforming vector applied to the receiver's array
     *
     * @return SpectrumSignalParameters in which is updated the PSD to contain
     * a set of values Vs frequency representing the received
     * power in the same units used for the txPower parameter,
     * and additional chanSpectrumMatrix is set.
     */
    virtual Ptr<SpectrumSignalParameters> DoCalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> params,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b,
        Ptr<const PhasedArrayModel> aPhasedArrayModel,
        Ptr<const PhasedArrayModel> bPhasedArrayModel,
        const PhasedArrayModel::ComplexVector& aBeamformingVector,
        const PhasedArrayModel::ComplexVector& bBeamformingVector) const = 0;

    Ptr<PhasedArraySpectrumPropagationLossModel>
        m_next; //!< PhasedArraySpectrumPropagationLossModel chained to this one.
};

} // namespace ns3

#endif /* PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H */
