/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADVANCED_EMLSR_MANAGER_H
#define ADVANCED_EMLSR_MANAGER_H

#include "default-emlsr-manager.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * AdvancedEmlsrManager is an advanced EMLSR manager.
 */
class AdvancedEmlsrManager : public DefaultEmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    AdvancedEmlsrManager();
    ~AdvancedEmlsrManager() override;

  protected:
    void DoDispose() override;
    void DoSetWifiMac(Ptr<StaWifiMac> mac) override;
    std::pair<bool, Time> DoGetDelayUntilAccessRequest(uint8_t linkId) override;
    std::pair<bool, Time> GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId) override;
    void SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci) override;

    /**
     * Possibly take actions when notified of the MAC header of the MPDU being received by the
     * given PHY.
     *
     * \param phy the given PHY
     * \param macHdr the MAC header of the MPDU being received
     * \param txVector the TXVECTOR used to transmit the PSDU
     * \param psduDuration the remaining duration of the PSDU
     */
    void ReceivedMacHdr(Ptr<WifiPhy> phy,
                        const WifiMacHeader& macHdr,
                        const WifiTxVector& txVector,
                        Time psduDuration);

    /**
     * Use information from NAV and CCA performed by the given PHY on the given link in the last
     * PIFS interval to determine whether the given EDCAF can start a TXOP. This function is
     * intended to be used when the main PHY switches channel to start an UL TXOP on a link where
     * channel access was obtained by a non-TX capable aux PHY.
     *
     * \param phy the PHY that performed CCA in the last PIFS interval
     * \param linkId the ID of the given link
     * \param edca the given EDCAF
     */
    void CheckNavAndCcaLastPifs(Ptr<WifiPhy> phy, uint8_t linkId, Ptr<QosTxop> edca);

    /**
     * Determine whether the main PHY shall be requested to switch to the link of an aux PHY that
     * has gained channel access through the given AC but it is not TX capable.
     *
     * \param linkId the ID of the link on which the aux PHY is operating
     * \param aci the index of the given AC
     * \return whether the main PHY shall be requested to switch to the link of the aux PHY
     */
    bool RequestMainPhyToSwitch(uint8_t linkId, AcIndex aci);

  private:
    void DoNotifyTxopEnd(uint8_t linkId) override;
    void DoNotifyIcfReceived(uint8_t linkId) override;
    void DoNotifyUlTxopStart(uint8_t linkId) override;

    bool m_useNotifiedMacHdr;      //!< whether to use the information about the MAC header of
                                   //!< the MPDU being received (if notified by the PHY)
    bool m_allowUlTxopInRx;        //!< whether a (main or aux) PHY is allowed to start an UL
                                   //!< TXOP if another PHY is receiving a PPDU
    bool m_interruptSwitching;     //!< whether a main PHY switching can be interrupted to start
                                   //!< switching to another link
    bool m_useAuxPhyCca;           //!< whether the CCA performed in the last PIFS interval by a
                                   //!< non-TX capable aux PHY should be used when the main PHY
                                   //!< ends switching to the aux PHY's link to determine whether
                                   //!< TX can start or not
    Time m_switchMainPhyBackDelay; //!< duration of the timer started in case of non-TX capable aux
                                   //!< PHY when medium is sensed busy during the PIFS interval
                                   //!< preceding/following the main PHY switch end
    EventId m_ccaLastPifs;         //!< event scheduled in case of non-TX capable aux PHY to
                                   //!< determine whether TX can be started based on whether
                                   //!< the medium has been idle during the last PIFS interval
    EventId m_switchMainPhyBackEvent; //!< event scheduled in case of non-TX capable aux PHY when
                                      //!< medium is sensed busy during the PIFS interval
                                      //!< preceding/following the main PHY switch end
};

} // namespace ns3

#endif /* ADVANCED_EMLSR_MANAGER_H */
