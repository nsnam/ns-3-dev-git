/*
 * Copyright (c) 2022 University of Washington
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

#ifndef SPECTRUM_TRANSMIT_FILTER_H
#define SPECTRUM_TRANSMIT_FILTER_H

#include <ns3/object.h>

namespace ns3
{

struct SpectrumSignalParameters;
class SpectrumPhy;

/**
 * \ingroup spectrum
 *
 * \brief spectrum-aware transmit filter object
 *
 * Interface for transmit filters that permit an early discard of signal
 * reception before propagation loss models or receiving Phy objects have
 * to process the signal, for performance optimization purposes.
 *
 */
class SpectrumTransmitFilter : public Object
{
  public:
    SpectrumTransmitFilter();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Add a transmit filter to be consulted next if this filter does not
     * filter the signal
     *
     * \param next next transmit filter to add to the chain
     */
    void SetNext(Ptr<SpectrumTransmitFilter> next);

    /**
     * Return the next transmit filter in the chain
     *
     * \return next transmit filter in the chain
     */
    Ptr<const SpectrumTransmitFilter> GetNext() const;

    /**
     * Evaluate whether the signal to be scheduled on the receiving Phy should
     * instead be filtered (discarded) before being processed in this channel
     * and on the receiving Phy.
     *
     * \param params the spectrum signal parameters.
     * \param receiverPhy pointer to the receiving SpectrumPhy
     *
     * \return whether to perform filtering of the signal
     */
    bool Filter(Ptr<const SpectrumSignalParameters> params, Ptr<const SpectrumPhy> receiverPhy);

    /**
     * If this loss model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.  If there are SpectrumTransmitFilters chained
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
     * Evaluate whether the signal to be scheduled on the receiving Phy should
     * instead be filtered (discarded) before being processed in this channel
     * and on the receiving Phy.
     *
     * \param params the spectrum signal parameters.
     * \param receiverPhy pointer to the receiving SpectrumPhy
     *
     * \return whether to perform filtering of the signal
     */
    virtual bool DoFilter(Ptr<const SpectrumSignalParameters> params,
                          Ptr<const SpectrumPhy> receiverPhy) = 0;

    Ptr<SpectrumTransmitFilter> m_next{nullptr}; //!< SpectrumTransmitFilter chained to this one.
};

} // namespace ns3

#endif /* SPECTRUM_TRANSMIT_FILTER_H */
