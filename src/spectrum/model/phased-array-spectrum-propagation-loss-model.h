/*
 * Copyright (c) 2021 CTTC
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

#ifndef PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H
#define PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H

#include "spectrum-value.h"

#include <ns3/mobility-model.h>
#include <ns3/object.h>
#include <ns3/phased-array-model.h>

namespace ns3
{

struct SpectrumSignalParameters;

/**
 * \ingroup spectrum
 *
 * \brief spectrum-aware propagation loss model that is
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
     * \brief Get the type ID.
     * \return the object TypeId
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
     * This method is to be called to calculate
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
     * If this loss model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.  If there are PhasedArraySpectrumPropagationLossModels
     * chained together, this method will also assign streams to the
     * downstream models.
     *
     * \param stream the stream index offset start
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void DoDispose() override;
    /**
     * Assign a fixed random variable stream number to the random variables used by this model.
     *
     * Subclasses must implement this; those not using random variables can return zero.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
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
        Ptr<const PhasedArrayModel> bPhasedArrayModel) const = 0;

    Ptr<PhasedArraySpectrumPropagationLossModel>
        m_next; //!< PhasedArraySpectrumPropagationLossModel chained to this one.
};

} // namespace ns3

#endif /* PHASED_ARRAY_SPECTRUM_PROPAGATION_LOSS_MODEL_H */
