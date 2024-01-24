/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SPECTRUM_PROPAGATION_LOSS_MODEL_H
#define SPECTRUM_PROPAGATION_LOSS_MODEL_H

#include "spectrum-value.h"

#include <ns3/mobility-model.h>
#include <ns3/object.h>

namespace ns3
{

struct SpectrumSignalParameters;

/**
 * \ingroup spectrum
 *
 * \brief spectrum-aware propagation loss model
 *
 * Interface for propagation loss models to be adopted when
 * transmissions are modeled with a power spectral density by means of
 * the SpectrumValue class.
 *
 */
class SpectrumPropagationLossModel : public Object
{
  public:
    SpectrumPropagationLossModel();
    ~SpectrumPropagationLossModel() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Used to chain various instances of SpectrumPropagationLossModel
     *
     * @param next
     */
    void SetNext(Ptr<SpectrumPropagationLossModel> next);

    /**
     * Return the pointer to the next SpectrumPropagationLossModel, if any.
     *
     * @return Pointer to the next model, if any.
     */
    Ptr<SpectrumPropagationLossModel> GetNext() const;

    /**
     * This method is to be called to calculate
     *
     * @param params the spectrum signal parameters.
     * @param a sender mobility
     * @param b receiver mobility
     *
     * @return set of values Vs frequency representing the received
     * power in the same units used for the txPower parameter.
     */
    Ptr<SpectrumValue> CalcRxPowerSpectralDensity(Ptr<const SpectrumSignalParameters> params,
                                                  Ptr<const MobilityModel> a,
                                                  Ptr<const MobilityModel> b) const;

    /**
     * If this loss model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.  If there are SpectrumPropagationLossModels chained
     * together, this method will also assign streams to the
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
     *
     * @return set of values Vs frequency representing the received
     * power in the same units used for the txPower parameter.
     */
    virtual Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> params,
        Ptr<const MobilityModel> a,
        Ptr<const MobilityModel> b) const = 0;

    Ptr<SpectrumPropagationLossModel> m_next; //!< SpectrumPropagationLossModel chained to this one.
};

} // namespace ns3

#endif /* SPECTRUM_PROPAGATION_LOSS_MODEL_H */
