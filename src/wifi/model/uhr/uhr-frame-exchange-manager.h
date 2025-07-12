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

    void NotifySwitchingStartNow(Time duration) override;

    /**
     * Notify that the PHY is switching to or from a DSO subband.
     * This notification is sent by the DSO Manager.
     */
    void NotifyDsoSwitching();

  protected:
    void ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                     RxSignalInfo rxSignalInfo,
                     const WifiTxVector& txVector,
                     bool inAmpdu) override;
    void ReceivedQosNullAfterBsrpTf(Mac48Address sender) override;
    void PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) override;
    void TxopEnd(const std::optional<Mac48Address>& txopHolder) override;
    void ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector) override;
    void NotifyChannelReleased(Ptr<Txop> txop) override;
    void TbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations) override;
    std::set<uint8_t> GetIndicesOccupyingRu(const CtrlTriggerHeader& trigger,
                                            uint16_t aid) const override;

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
     * @param delay the delay after which the DSO STA is expected to switch back to its primary
     * subband
     */
    void DsoSwitchBackToPrimary(const Mac48Address& address, const Time& delay);

    bool m_dsoIcfReceived{
        false}; //!< flag whether an ICF has been received to indicate the start of a DSO TXOP
    bool m_switchingForDso{false}; //!< flag whether channel is switching for DSO operations

    std::map<Mac48Address, WifiRu::RuSpec>
        m_dsoStas; //!< STAs that are being served in the current DSO TXOP
};

} // namespace ns3

#endif /* UHR_FRAME_EXCHANGE_MANAGER_H */
