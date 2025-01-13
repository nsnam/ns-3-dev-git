/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef MULTI_MODEL_SPECTRUM_CHANNEL_H
#define MULTI_MODEL_SPECTRUM_CHANNEL_H

#include "spectrum-channel.h"
#include "spectrum-converter.h"
#include "spectrum-propagation-loss-model.h"
#include "spectrum-value.h"

#include "ns3/propagation-delay-model.h"

#include <map>
#include <set>

namespace ns3
{

/**
 * @ingroup spectrum
 * Container: SpectrumModelUid_t, SpectrumConverter
 */
typedef std::map<SpectrumModelUid_t, SpectrumConverter> SpectrumConverterMap_t;

/**
 * @ingroup spectrum
 * The Tx spectrum model information. This class is used to convert
 * one spectrum model into another one.
 */
class TxSpectrumModelInfo
{
  public:
    /**
     * Constructor.
     * @param txSpectrumModel the Tx Spectrum model.
     */
    TxSpectrumModelInfo(Ptr<const SpectrumModel> txSpectrumModel);

    Ptr<const SpectrumModel> m_txSpectrumModel;    //!< Tx Spectrum model.
    SpectrumConverterMap_t m_spectrumConverterMap; //!< Spectrum converter.
};

/**
 * @ingroup spectrum
 * Container: SpectrumModelUid_t, TxSpectrumModelInfo
 */
typedef std::map<SpectrumModelUid_t, TxSpectrumModelInfo> TxSpectrumModelInfoMap_t;

/**
 * @ingroup spectrum
 * The Rx spectrum model information. This class is used to convert
 * one spectrum model into another one.
 */
class RxSpectrumModelInfo
{
  public:
    /**
     * Constructor.
     * @param rxSpectrumModel the Rx Spectrum model.
     */
    RxSpectrumModelInfo(Ptr<const SpectrumModel> rxSpectrumModel);

    Ptr<const SpectrumModel> m_rxSpectrumModel; //!< Rx Spectrum model.
    std::vector<Ptr<SpectrumPhy>> m_rxPhys;     //!< Container of the Rx Spectrum phy objects.
};

/**
 * @ingroup spectrum
 * Container: SpectrumModelUid_t, RxSpectrumModelInfo
 */
typedef std::map<SpectrumModelUid_t, RxSpectrumModelInfo> RxSpectrumModelInfoMap_t;

/**
 * @ingroup spectrum
 *
 * This SpectrumChannel implementation can handle the presence of
 * SpectrumPhy instances which can use
 * different spectrum models, i.e.,  different SpectrumModel.
 *
 * @note It is allowed for a receiving SpectrumPhy to switch to a
 * different SpectrumModel during the simulation. The requirement
 * for this to work is that, after the SpectrumPhy switched its
 * SpectrumModel,  MultiModelSpectrumChannel::AddRx () is
 * called again passing the pointer to that SpectrumPhy.
 */
class MultiModelSpectrumChannel : public SpectrumChannel
{
  public:
    MultiModelSpectrumChannel();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from SpectrumChannel
    void RemoveRx(Ptr<SpectrumPhy> phy) override;
    void AddRx(Ptr<SpectrumPhy> phy) override;
    void StartTx(Ptr<SpectrumSignalParameters> params) override;

    // inherited from Channel
    std::size_t GetNDevices() const override;
    Ptr<NetDevice> GetDevice(std::size_t i) const override;

  protected:
    void DoDispose() override;

  private:
    /**
     * This method checks if m_rxSpectrumModelInfoMap contains an entry
     * for the given TX SpectrumModel. If such entry exists, it returns
     * an iterator pointing to it. If not, it creates a new entry in
     * m_txSpectrumModelInfoMap, and returns an iterator to it.
     *
     * @param txSpectrumModel The TX SpectrumModel  being considered
     *
     * @return An iterator pointing to the corresponding entry in m_txSpectrumModelInfoMap
     */
    TxSpectrumModelInfoMap_t::const_iterator FindAndEventuallyAddTxSpectrumModel(
        Ptr<const SpectrumModel> txSpectrumModel);

    /**
     * Used internally to reschedule transmission after the propagation delay.
     *
     * @param txPsd The transmitted PSD.
     * @param txAntennaGain The antenna gain at the transmitter.
     * @param params The signal parameters.
     * @param receiver A pointer to the receiver SpectrumPhy.
     * @param availableConvertedPsds available converted PSDs from the TX PSD.
     */
    virtual void StartRx(
        Ptr<SpectrumValue> txPsd,
        double txAntennaGain,
        Ptr<SpectrumSignalParameters> params,
        Ptr<SpectrumPhy> receiver,
        const std::map<SpectrumModelUid_t, Ptr<SpectrumValue>>& availableConvertedPsds);

    /**
     * Data structure holding, for each TX SpectrumModel,  all the
     * converters to any RX SpectrumModel, and all the corresponding
     * SpectrumPhy instances.
     *
     */
    TxSpectrumModelInfoMap_t m_txSpectrumModelInfoMap;

    /**
     * Data structure holding, for each RX spectrum model, all the
     * corresponding SpectrumPhy instances.
     */
    RxSpectrumModelInfoMap_t m_rxSpectrumModelInfoMap;

    /**
     * Number of devices connected to the channel.
     */
    std::size_t m_numDevices;
};

} // namespace ns3

#endif /* MULTI_MODEL_SPECTRUM_CHANNEL_H */
