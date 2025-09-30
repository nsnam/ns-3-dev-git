/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_FRAME_EXCHANGE_MANAGER_H
#define UHR_FRAME_EXCHANGE_MANAGER_H

#include "ns3/eht-frame-exchange-manager.h"

#include <optional>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * UhrFrameExchangeManager handles the frame exchange sequences
 * for UHR stations.
 */
class UhrFrameExchangeManager : public EhtFrameExchangeManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    UhrFrameExchangeManager();
    ~UhrFrameExchangeManager() override;

    void SetIcfPaddingAndTxVector(CtrlTriggerHeader& trigger,
                                  WifiTxVector& txVector) const override;

  protected:
    void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                     RxSignalInfo rxSignalInfo,
                     const WifiTxVector& txVector,
                     bool inAmpdu) override;
    void ReceivedQosNullAfterBsrpTf(Mac48Address sender) override;
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void TxopEnd(const std::optional<Mac48Address>& txopHolder) override;
    void ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector) override;
    void ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector) override;
    void NotifyChannelReleased() override;
    void TbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations) override;
    std::set<uint8_t> GetIndicesOccupyingRu(const CtrlTriggerHeader& trigger,
                                            uint16_t aid) const override;
    bool GetTxAllowedFor(const Time& duration) const override;

    /**
     * @param psdu the given PSDU
     * @param aid the AID of a DSO STA
     * @param address the link MAC address of a DSO STA
     * @return whether the DSO STA having the given AID and MAC address shall switch back to
     *         the primary subband when receiving the given PSDU
     */
    bool ShouldDsoSwitchBackToPrimary(Ptr<const WifiPsdu> psdu,
                                      uint16_t aid,
                                      const Mac48Address& address) const;

    /**
     * This method is intended to be called when an AP detects that an DSO STA previously involved
     * in the current TXOP will start switching back to its primary subchannel. This method blocks
     * the transmissions on the current link of the given DSO STA until the switch back delay
     * advertised by the DSO STA expires.
     *
     * @param address the link MAC address of the given DSO STA
     * @param delay the delay after which the DSO STA is expected to have switched back to its
     * primary subband
     */
    void DsoSwitchBackToPrimary(const Mac48Address& address, const Time& delay);

    /**
     * All DSO STAs are expected to switch back to their primary subchannel after a certain delay.
     *
     * @param delay the delay after which the DSO STAs are expected to start switching back to their
     * primary subband (hence do not account for the channel switch duration)
     */
    void SwitchToPrimarySubchannel(const Time& delay);

    /**
     * @return whether at least one DSO STA assigned outside of the P20 subband has responded to the
     * DSO ICF and none of the DSO STAs assigned inside of P20 subband has responded to the DSO ICF.
     */
    bool ReceivedDsoIcfResponsesExceptOnP20() const;

    bool m_dsoIcfReceived{
        false}; //!< flag whether an ICF has been received to indicate the start of a DSO TXOP

    std::map<Mac48Address, WifiRu::RuSpec>
        m_dsoStas; //!< STAs that are being served in the current DSO TXOP

    std::optional<CtrlTriggerHeader>
        m_trigger; //!< the last Trigger Frame received in the current TXOP
};

} // namespace ns3

#endif /* UHR_FRAME_EXCHANGE_MANAGER_H */
