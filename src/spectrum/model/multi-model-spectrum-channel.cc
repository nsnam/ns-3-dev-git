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

#include "multi-model-spectrum-channel.h"

#include "spectrum-converter.h"
#include "spectrum-phy.h"
#include "spectrum-propagation-loss-model.h"
#include "spectrum-transmit-filter.h"

#include <ns3/angles.h>
#include <ns3/antenna-model.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/node.h>
#include <ns3/object.h>
#include <ns3/packet-burst.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>

#include <algorithm>
#include <iostream>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MultiModelSpectrumChannel");

NS_OBJECT_ENSURE_REGISTERED(MultiModelSpectrumChannel);

/**
 * \brief Output stream operator
 * \param lhs output stream
 * \param rhs the TxSpectrumModelInfoMap to print
 * \return an output stream
 */
std::ostream&
operator<<(std::ostream& lhs, TxSpectrumModelInfoMap_t& rhs)
{
    for (auto it = rhs.begin(); it != rhs.end(); ++it)
    {
        for (auto jt = it->second.m_spectrumConverterMap.begin();
             jt != it->second.m_spectrumConverterMap.end();
             ++jt)
        {
            lhs << "(" << it->first << "," << jt->first << ") ";
        }
    }
    return lhs;
}

TxSpectrumModelInfo::TxSpectrumModelInfo(Ptr<const SpectrumModel> txSpectrumModel)
    : m_txSpectrumModel(txSpectrumModel)
{
}

RxSpectrumModelInfo::RxSpectrumModelInfo(Ptr<const SpectrumModel> rxSpectrumModel)
    : m_rxSpectrumModel(rxSpectrumModel)
{
}

MultiModelSpectrumChannel::MultiModelSpectrumChannel()
    : m_numDevices{0}
{
    NS_LOG_FUNCTION(this);
}

void
MultiModelSpectrumChannel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_txSpectrumModelInfoMap.clear();
    m_rxSpectrumModelInfoMap.clear();
    SpectrumChannel::DoDispose();
}

TypeId
MultiModelSpectrumChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MultiModelSpectrumChannel")
                            .SetParent<SpectrumChannel>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<MultiModelSpectrumChannel>()

        ;
    return tid;
}

void
MultiModelSpectrumChannel::RemoveRx(Ptr<SpectrumPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);

    // remove a previous entry of this phy if it exists
    // we need to scan for all rxSpectrumModel values since we don't
    // know which spectrum model the phy had when it was previously added
    // (it's probably different than the current one)
    for (auto rxInfoIterator = m_rxSpectrumModelInfoMap.begin();
         rxInfoIterator != m_rxSpectrumModelInfoMap.end();
         ++rxInfoIterator)
    {
        auto phyIt = std::find(rxInfoIterator->second.m_rxPhys.begin(),
                               rxInfoIterator->second.m_rxPhys.end(),
                               phy);
        if (phyIt != rxInfoIterator->second.m_rxPhys.end())
        {
            rxInfoIterator->second.m_rxPhys.erase(phyIt);
            --m_numDevices;
            break; // there should be at most one entry
        }
    }
}

void
MultiModelSpectrumChannel::AddRx(Ptr<SpectrumPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);

    Ptr<const SpectrumModel> rxSpectrumModel = phy->GetRxSpectrumModel();

    NS_ASSERT_MSG(rxSpectrumModel,
                  "phy->GetRxSpectrumModel () returned 0. Please check that the RxSpectrumModel is "
                  "already set for the phy before calling MultiModelSpectrumChannel::AddRx (phy)");

    SpectrumModelUid_t rxSpectrumModelUid = rxSpectrumModel->GetUid();

    RemoveRx(phy);

    ++m_numDevices;

    auto [rxInfoIterator, inserted] =
        m_rxSpectrumModelInfoMap.emplace(rxSpectrumModelUid, RxSpectrumModelInfo(rxSpectrumModel));

    // rxInfoIterator points either to the newly inserted element or to the element that
    // prevented insertion. In both cases, add the phy to the element pointed to by rxInfoIterator
    rxInfoIterator->second.m_rxPhys.push_back(phy);

    if (inserted)
    {
        // create the necessary converters for all the TX spectrum models that we know of
        for (auto txInfoIterator = m_txSpectrumModelInfoMap.begin();
             txInfoIterator != m_txSpectrumModelInfoMap.end();
             ++txInfoIterator)
        {
            Ptr<const SpectrumModel> txSpectrumModel = txInfoIterator->second.m_txSpectrumModel;
            SpectrumModelUid_t txSpectrumModelUid = txSpectrumModel->GetUid();

            if (rxSpectrumModelUid != txSpectrumModelUid &&
                !txSpectrumModel->IsOrthogonal(*rxSpectrumModel))
            {
                NS_LOG_LOGIC("Creating converter between SpectrumModelUid "
                             << txSpectrumModel->GetUid() << " and " << rxSpectrumModelUid);
                SpectrumConverter converter(txSpectrumModel, rxSpectrumModel);
                auto ret2 = txInfoIterator->second.m_spectrumConverterMap.insert(
                    std::make_pair(rxSpectrumModelUid, converter));
                NS_ASSERT(ret2.second);
            }
        }
    }
}

TxSpectrumModelInfoMap_t::const_iterator
MultiModelSpectrumChannel::FindAndEventuallyAddTxSpectrumModel(
    Ptr<const SpectrumModel> txSpectrumModel)
{
    NS_LOG_FUNCTION(this << txSpectrumModel);
    SpectrumModelUid_t txSpectrumModelUid = txSpectrumModel->GetUid();
    auto txInfoIterator = m_txSpectrumModelInfoMap.find(txSpectrumModelUid);

    if (txInfoIterator == m_txSpectrumModelInfoMap.end())
    {
        // first time we see this TX SpectrumModel
        // we add it to the list
        auto ret = m_txSpectrumModelInfoMap.insert(
            std::make_pair(txSpectrumModelUid, TxSpectrumModelInfo(txSpectrumModel)));
        NS_ASSERT(ret.second);
        txInfoIterator = ret.first;

        // and we create the converters for all the RX SpectrumModels that we know of
        for (auto rxInfoIterator = m_rxSpectrumModelInfoMap.begin();
             rxInfoIterator != m_rxSpectrumModelInfoMap.end();
             ++rxInfoIterator)
        {
            Ptr<const SpectrumModel> rxSpectrumModel = rxInfoIterator->second.m_rxSpectrumModel;
            SpectrumModelUid_t rxSpectrumModelUid = rxSpectrumModel->GetUid();

            if (rxSpectrumModelUid != txSpectrumModelUid &&
                !txSpectrumModel->IsOrthogonal(*rxSpectrumModel))
            {
                NS_LOG_LOGIC("Creating converter between SpectrumModelUid "
                             << txSpectrumModelUid << " and " << rxSpectrumModelUid);

                SpectrumConverter converter(txSpectrumModel, rxSpectrumModel);
                auto ret2 = txInfoIterator->second.m_spectrumConverterMap.insert(
                    std::make_pair(rxSpectrumModelUid, converter));
                NS_ASSERT(ret2.second);
            }
        }
    }
    else
    {
        NS_LOG_LOGIC("SpectrumModelUid " << txSpectrumModelUid << " already present");
    }
    return txInfoIterator;
}

void
MultiModelSpectrumChannel::StartTx(Ptr<SpectrumSignalParameters> txParams)
{
    NS_LOG_FUNCTION(this << txParams);

    NS_ASSERT(txParams->txPhy);
    NS_ASSERT(txParams->psd);
    Ptr<SpectrumSignalParameters> txParamsTrace =
        txParams->Copy(); // copy it since traced value cannot be const (because of potential
                          // underlying DynamicCasts)
    m_txSigParamsTrace(txParamsTrace);

    Ptr<MobilityModel> txMobility = txParams->txPhy->GetMobility();
    SpectrumModelUid_t txSpectrumModelUid = txParams->psd->GetSpectrumModelUid();
    NS_LOG_LOGIC("txSpectrumModelUid " << txSpectrumModelUid);

    //
    auto txInfoIteratorerator =
        FindAndEventuallyAddTxSpectrumModel(txParams->psd->GetSpectrumModel());
    NS_ASSERT(txInfoIteratorerator != m_txSpectrumModelInfoMap.end());

    NS_LOG_LOGIC("converter map for TX SpectrumModel with Uid " << txInfoIteratorerator->first);
    NS_LOG_LOGIC(
        "converter map size: " << txInfoIteratorerator->second.m_spectrumConverterMap.size());
    NS_LOG_LOGIC("converter map first element: "
                 << txInfoIteratorerator->second.m_spectrumConverterMap.begin()->first);

    for (auto rxInfoIterator = m_rxSpectrumModelInfoMap.begin();
         rxInfoIterator != m_rxSpectrumModelInfoMap.end();
         ++rxInfoIterator)
    {
        SpectrumModelUid_t rxSpectrumModelUid = rxInfoIterator->second.m_rxSpectrumModel->GetUid();
        NS_LOG_LOGIC("rxSpectrumModelUids " << rxSpectrumModelUid);

        Ptr<SpectrumValue> convertedTxPowerSpectrum;
        if (txSpectrumModelUid == rxSpectrumModelUid)
        {
            NS_LOG_LOGIC("no spectrum conversion needed");
            convertedTxPowerSpectrum = txParams->psd;
        }
        else
        {
            NS_LOG_LOGIC("converting txPowerSpectrum SpectrumModelUids "
                         << txSpectrumModelUid << " --> " << rxSpectrumModelUid);
            auto rxConverterIterator =
                txInfoIteratorerator->second.m_spectrumConverterMap.find(rxSpectrumModelUid);
            if (rxConverterIterator == txInfoIteratorerator->second.m_spectrumConverterMap.end())
            {
                // No converter means TX SpectrumModel is orthogonal to RX SpectrumModel
                continue;
            }
            convertedTxPowerSpectrum = rxConverterIterator->second.Convert(txParams->psd);
        }

        for (auto rxPhyIterator = rxInfoIterator->second.m_rxPhys.begin();
             rxPhyIterator != rxInfoIterator->second.m_rxPhys.end();
             ++rxPhyIterator)
        {
            NS_ASSERT_MSG((*rxPhyIterator)->GetRxSpectrumModel()->GetUid() == rxSpectrumModelUid,
                          "SpectrumModel change was not notified to MultiModelSpectrumChannel "
                          "(i.e., AddRx should be called again after model is changed)");

            if ((*rxPhyIterator) != txParams->txPhy)
            {
                Ptr<NetDevice> rxNetDevice = (*rxPhyIterator)->GetDevice();
                Ptr<NetDevice> txNetDevice = txParams->txPhy->GetDevice();

                if (rxNetDevice && txNetDevice)
                {
                    // we assume that devices are attached to a node
                    if (rxNetDevice->GetNode()->GetId() == txNetDevice->GetNode()->GetId())
                    {
                        NS_LOG_DEBUG(
                            "Skipping the pathloss calculation among different antennas of the "
                            "same node, not supported yet by any pathloss model in ns-3.");
                        continue;
                    }
                }

                if (m_filter && m_filter->Filter(txParams, *rxPhyIterator))
                {
                    continue;
                }

                NS_LOG_LOGIC("copying signal parameters " << txParams);
                Ptr<SpectrumSignalParameters> rxParams = txParams->Copy();
                rxParams->psd = Copy<SpectrumValue>(convertedTxPowerSpectrum);
                Time delay = MicroSeconds(0);

                Ptr<MobilityModel> receiverMobility = (*rxPhyIterator)->GetMobility();

                if (txMobility && receiverMobility)
                {
                    double txAntennaGain = 0;
                    double rxAntennaGain = 0;
                    double propagationGainDb = 0;
                    double pathLossDb = 0;
                    if (rxParams->txAntenna)
                    {
                        Angles txAngles(receiverMobility->GetPosition(), txMobility->GetPosition());
                        txAntennaGain = rxParams->txAntenna->GetGainDb(txAngles);
                        NS_LOG_LOGIC("txAntennaGain = " << txAntennaGain << " dB");
                        pathLossDb -= txAntennaGain;
                    }
                    Ptr<AntennaModel> rxAntenna =
                        DynamicCast<AntennaModel>((*rxPhyIterator)->GetAntenna());
                    if (rxAntenna)
                    {
                        Angles rxAngles(txMobility->GetPosition(), receiverMobility->GetPosition());
                        rxAntennaGain = rxAntenna->GetGainDb(rxAngles);
                        NS_LOG_LOGIC("rxAntennaGain = " << rxAntennaGain << " dB");
                        pathLossDb -= rxAntennaGain;
                    }
                    if (m_propagationLoss)
                    {
                        propagationGainDb =
                            m_propagationLoss->CalcRxPower(0, txMobility, receiverMobility);
                        NS_LOG_LOGIC("propagationGainDb = " << propagationGainDb << " dB");
                        pathLossDb -= propagationGainDb;
                    }
                    NS_LOG_LOGIC("total pathLoss = " << pathLossDb << " dB");
                    // Gain trace
                    m_gainTrace(txMobility,
                                receiverMobility,
                                txAntennaGain,
                                rxAntennaGain,
                                propagationGainDb,
                                pathLossDb);
                    // Pathloss trace
                    m_pathLossTrace(txParams->txPhy, *rxPhyIterator, pathLossDb);
                    if (pathLossDb > m_maxLossDb)
                    {
                        // beyond range
                        continue;
                    }
                    double pathGainLinear = std::pow(10.0, (-pathLossDb) / 10.0);
                    *(rxParams->psd) *= pathGainLinear;

                    if (m_propagationDelay)
                    {
                        delay = m_propagationDelay->GetDelay(txMobility, receiverMobility);
                    }
                }

                if (rxNetDevice)
                {
                    // the receiver has a NetDevice, so we expect that it is attached to a Node
                    uint32_t dstNode = rxNetDevice->GetNode()->GetId();
                    Simulator::ScheduleWithContext(dstNode,
                                                   delay,
                                                   &MultiModelSpectrumChannel::StartRx,
                                                   this,
                                                   rxParams,
                                                   *rxPhyIterator);
                }
                else
                {
                    // the receiver is not attached to a NetDevice, so we cannot assume that it is
                    // attached to a node
                    Simulator::Schedule(delay,
                                        &MultiModelSpectrumChannel::StartRx,
                                        this,
                                        rxParams,
                                        *rxPhyIterator);
                }
            }
        }
    }
}

void
MultiModelSpectrumChannel::StartRx(Ptr<SpectrumSignalParameters> params, Ptr<SpectrumPhy> receiver)
{
    NS_LOG_FUNCTION(this);
    if (m_spectrumPropagationLoss)
    {
        params->psd =
            m_spectrumPropagationLoss->CalcRxPowerSpectralDensity(params,
                                                                  params->txPhy->GetMobility(),
                                                                  receiver->GetMobility());
    }
    else if (m_phasedArraySpectrumPropagationLoss)
    {
        Ptr<const PhasedArrayModel> txPhasedArrayModel =
            DynamicCast<PhasedArrayModel>(params->txPhy->GetAntenna());
        Ptr<const PhasedArrayModel> rxPhasedArrayModel =
            DynamicCast<PhasedArrayModel>(receiver->GetAntenna());

        NS_ASSERT_MSG(txPhasedArrayModel && rxPhasedArrayModel,
                      "PhasedArrayModel instances should be installed at both TX and RX "
                      "SpectrumPhy in order to use PhasedArraySpectrumPropagationLoss.");

        params->psd = m_phasedArraySpectrumPropagationLoss->CalcRxPowerSpectralDensity(
            params,
            params->txPhy->GetMobility(),
            receiver->GetMobility(),
            txPhasedArrayModel,
            rxPhasedArrayModel);
    }
    receiver->StartRx(params);
}

std::size_t
MultiModelSpectrumChannel::GetNDevices() const
{
    return m_numDevices;
}

Ptr<NetDevice>
MultiModelSpectrumChannel::GetDevice(std::size_t i) const
{
    NS_ASSERT(i < m_numDevices);
    // this method implementation is computationally intensive. This
    // method would be faster if we actually used a std::vector for
    // storing devices, which we don't due to the need to have fast
    // SpectrumModel conversions and to allow PHY devices to change a
    // SpectrumModel at run time. Note that having this method slow is
    // acceptable as it is not used much at run time (often not at all).
    // On the other hand, having slow SpectrumModel conversion would be
    // less acceptable.
    std::size_t j = 0;
    for (auto rxInfoIterator = m_rxSpectrumModelInfoMap.begin();
         rxInfoIterator != m_rxSpectrumModelInfoMap.end();
         ++rxInfoIterator)
    {
        for (const auto& phyIt : rxInfoIterator->second.m_rxPhys)
        {
            if (j == i)
            {
                return (*phyIt).GetDevice();
            }
            j++;
        }
    }
    NS_FATAL_ERROR("m_numDevices > actual number of devices");
    return nullptr;
}

} // namespace ns3
