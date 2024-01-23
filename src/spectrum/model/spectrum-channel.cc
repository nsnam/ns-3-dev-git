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

#include "spectrum-channel.h"

#include <ns3/abort.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/pointer.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumChannel");

NS_OBJECT_ENSURE_REGISTERED(SpectrumChannel);

SpectrumChannel::SpectrumChannel()
{
    NS_LOG_FUNCTION(this);
}

SpectrumChannel::~SpectrumChannel()
{
}

void
SpectrumChannel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_propagationLoss = nullptr;
    m_propagationDelay = nullptr;
    m_spectrumPropagationLoss = nullptr;
}

TypeId
SpectrumChannel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SpectrumChannel")
            .SetParent<Channel>()
            .SetGroupName("Spectrum")
            .AddAttribute("MaxLossDb",
                          "If a single-frequency PropagationLossModel is used, "
                          "this value represents the maximum loss in dB for which "
                          "transmissions will be passed to the receiving PHY. "
                          "Signals for which the PropagationLossModel returns "
                          "a loss bigger than this value will not be propagated "
                          "to the receiver. This parameter is to be used to reduce "
                          "the computational load by not propagating signals "
                          "that are far beyond the interference range. Note that "
                          "the default value corresponds to considering all signals "
                          "for reception. Tune this value with care.",
                          DoubleValue(1.0e9),
                          MakeDoubleAccessor(&SpectrumChannel::m_maxLossDb),
                          MakeDoubleChecker<double>())

            .AddAttribute("PropagationLossModel",
                          "A pointer to the propagation loss model attached to this channel.",
                          PointerValue(nullptr),
                          MakePointerAccessor(&SpectrumChannel::m_propagationLoss),
                          MakePointerChecker<PropagationLossModel>())

            .AddTraceSource("Gain",
                            "This trace is fired whenever a new path loss value "
                            "is calculated. The parameters to this trace are : "
                            "Pointer to the mobility model of the transmitter, "
                            "Pointer to the mobility model of the receiver, "
                            "Tx antenna gain, "
                            "Rx antenna gain, "
                            "Propagation gain, "
                            "Pathloss",
                            MakeTraceSourceAccessor(&SpectrumChannel::m_gainTrace),
                            "ns3::SpectrumChannel::GainTracedCallback")

            .AddTraceSource("PathLoss",
                            "This trace is fired whenever a new path loss value "
                            "is calculated. The first and second parameters "
                            "to the trace are pointers respectively to the TX and "
                            "RX SpectrumPhy instances, whereas the third parameters "
                            "is the loss value in dB. Note that the loss value "
                            "reported by this trace is the single-frequency loss "
                            "value obtained by evaluating only the TX and RX "
                            "AntennaModels and the PropagationLossModel. "
                            "In particular, note that SpectrumPropagationLossModel "
                            "(even if present) is never used to evaluate the "
                            "loss value reported in this trace.",
                            MakeTraceSourceAccessor(&SpectrumChannel::m_pathLossTrace),
                            "ns3::SpectrumChannel::LossTracedCallback")

            .AddTraceSource("TxSigParams",
                            "This trace is fired whenever a signal is transmitted. "
                            "The sole parameter is a pointer to a copy of the "
                            "SpectrumSignalParameters provided by the transmitter.",
                            MakeTraceSourceAccessor(&SpectrumChannel::m_txSigParamsTrace),
                            "ns3::SpectrumChannel::SignalParametersTracedCallback");
    return tid;
}

void
SpectrumChannel::AddPropagationLossModel(Ptr<PropagationLossModel> loss)
{
    NS_LOG_FUNCTION(this << loss);
    if (m_propagationLoss)
    {
        loss->SetNext(m_propagationLoss);
    }
    m_propagationLoss = loss;
}

void
SpectrumChannel::AddSpectrumPropagationLossModel(Ptr<SpectrumPropagationLossModel> loss)
{
    NS_LOG_FUNCTION(this << loss);
    if (m_spectrumPropagationLoss)
    {
        loss->SetNext(m_spectrumPropagationLoss);
    }
    m_spectrumPropagationLoss = loss;
}

void
SpectrumChannel::AddPhasedArraySpectrumPropagationLossModel(
    Ptr<PhasedArraySpectrumPropagationLossModel> loss)
{
    NS_LOG_FUNCTION(this << loss);
    if (m_phasedArraySpectrumPropagationLoss)
    {
        loss->SetNext(m_phasedArraySpectrumPropagationLoss);
    }
    m_phasedArraySpectrumPropagationLoss = loss;
}

void
SpectrumChannel::AddSpectrumTransmitFilter(Ptr<SpectrumTransmitFilter> filter)
{
    NS_LOG_FUNCTION(this << filter);

    if (m_filter)
    {
        filter->SetNext(m_filter);
    }
    m_filter = filter;
}

Ptr<SpectrumTransmitFilter>
SpectrumChannel::GetSpectrumTransmitFilter() const
{
    return m_filter;
}

void
SpectrumChannel::SetPropagationDelayModel(Ptr<PropagationDelayModel> delay)
{
    NS_ABORT_MSG_IF(m_propagationDelay, "Error, called SetPropagationDelayModel() twice");
    m_propagationDelay = delay;
}

Ptr<SpectrumPropagationLossModel>
SpectrumChannel::GetSpectrumPropagationLossModel() const
{
    return m_spectrumPropagationLoss;
}

Ptr<PhasedArraySpectrumPropagationLossModel>
SpectrumChannel::GetPhasedArraySpectrumPropagationLossModel() const
{
    return m_phasedArraySpectrumPropagationLoss;
}

Ptr<PropagationLossModel>
SpectrumChannel::GetPropagationLossModel() const
{
    return m_propagationLoss;
}

Ptr<PropagationDelayModel>
SpectrumChannel::GetPropagationDelayModel() const
{
    return m_propagationDelay;
}

int64_t
SpectrumChannel::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    auto lastCurrentStream = stream;
    if (m_propagationLoss)
    {
        currentStream += m_propagationLoss->AssignStreams(currentStream);
    }
    if (currentStream - lastCurrentStream)
    {
        NS_LOG_DEBUG("PropagationLossModel objects used " << currentStream - lastCurrentStream
                                                          << " streams");
    }
    lastCurrentStream = currentStream;
    if (m_propagationDelay)
    {
        m_propagationDelay->AssignStreams(currentStream);
        currentStream += 1;
    }
    if (currentStream - lastCurrentStream)
    {
        NS_LOG_DEBUG("PropagationDelayModel object used " << currentStream - lastCurrentStream
                                                          << " streams");
    }
    lastCurrentStream = currentStream;
    if (m_spectrumPropagationLoss)
    {
        currentStream += m_spectrumPropagationLoss->AssignStreams(currentStream);
    }
    if (currentStream - lastCurrentStream)
    {
        NS_LOG_DEBUG("SpectrumPropagationLossModel objects used "
                     << currentStream - lastCurrentStream << " streams");
    }
    lastCurrentStream = currentStream;
    if (m_phasedArraySpectrumPropagationLoss)
    {
        currentStream += m_phasedArraySpectrumPropagationLoss->AssignStreams(currentStream);
    }
    if (currentStream - lastCurrentStream)
    {
        NS_LOG_DEBUG("PhasedArraySpectrumPropagationLossModel objects used "
                     << currentStream - lastCurrentStream << " streams");
    }
    lastCurrentStream = currentStream;
    if (m_filter)
    {
        currentStream += m_filter->AssignStreams(currentStream);
    }
    if (currentStream - lastCurrentStream)
    {
        NS_LOG_DEBUG("SpectrumTransmitFilter objects used " << currentStream - lastCurrentStream
                                                            << " streams");
    }
    currentStream += DoAssignStreams(currentStream);
    NS_LOG_DEBUG("Assigned a total of " << currentStream - stream << " streams");
    return (currentStream - stream);
}

int64_t
SpectrumChannel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return 0;
}

} // namespace ns3
