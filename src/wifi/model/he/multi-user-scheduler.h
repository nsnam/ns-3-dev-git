/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef MULTI_USER_SCHEDULER_H
#define MULTI_USER_SCHEDULER_H

#include "he-ru.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/ctrl-headers.h"
#include "ns3/object.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/wifi-tx-parameters.h"

#include <unordered_map>

namespace ns3
{

class HeFrameExchangeManager;

typedef std::unordered_map<uint16_t /* staId */, Ptr<WifiPsdu> /* PSDU */> WifiPsduMap;

/**
 * \ingroup wifi
 *
 * MultiUserScheduler is an abstract base class defining the API that APs
 * supporting at least VHT can use to determine the format of their next transmission.
 * VHT APs can only transmit DL MU PPDUs by using MU-MIMO, while HE APs can
 * transmit both DL MU PPDUs and UL MU PPDUs by using OFDMA in addition to MU-MIMO.
 *
 * However, given that DL MU-MIMO is not yet supported, a MultiUserScheduler can
 * only be aggregated to HE APs.
 */
class MultiUserScheduler : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    MultiUserScheduler();
    ~MultiUserScheduler() override;

    /// Enumeration of the possible transmission formats
    enum TxFormat
    {
        NO_TX = 0,
        SU_TX,
        DL_MU_TX,
        UL_MU_TX
    };

    /// Information to be provided in case of DL MU transmission
    struct DlMuInfo
    {
        WifiPsduMap psduMap;       //!< the DL MU PPDU to transmit
        WifiTxParameters txParams; //!< the transmission parameters
    };

    /// Information to be provided in case of UL MU transmission
    struct UlMuInfo
    {
        CtrlTriggerHeader trigger; //!< the Trigger Frame used to solicit TB PPDUs
        WifiMacHeader macHdr;      //!< the MAC header for the Trigger Frame
        WifiTxParameters txParams; //!< the transmission parameters for the Trigger Frame
    };

    /**
     * Notify the Multi-user Scheduler that the given AC of the AP gained channel
     * access. The Multi-user Scheduler determines the format of the next transmission.
     *
     * \param edca the EDCAF which has been granted the opportunity to transmit
     * \param availableTime the amount of time allowed for the frame exchange. Pass
     *                      Time::Min() in case the TXOP limit is null
     * \param initialFrame true if the frame being transmitted is the initial frame
     *                     of the TXOP. This is used to determine whether the TXOP
     *                     limit can be exceeded
     * \param allowedWidth the allowed width in MHz for the next transmission
     * \param linkId the ID of the link over which channel access was gained
     * \return the format of the next transmission
     */
    TxFormat NotifyAccessGranted(Ptr<QosTxop> edca,
                                 Time availableTime,
                                 bool initialFrame,
                                 uint16_t allowedWidth,
                                 uint8_t linkId);

    /**
     * Get the information required to perform a DL MU transmission on the given link. Note
     * that this method can only be called if GetTxFormat returns DL_MU_TX on the given link.
     *
     * \param linkId the ID of the given link
     * \return the information required to perform a DL MU transmission
     */
    DlMuInfo& GetDlMuInfo(uint8_t linkId);

    /**
     * Get the information required to solicit an UL MU transmission on the given link. Note
     * that this method can only be called if GetTxFormat returns UL_MU_TX on the given link.
     *
     * \param linkId the ID of the given link
     * \return the information required to solicit an UL MU transmission
     */
    UlMuInfo& GetUlMuInfo(uint8_t linkId);

    /**
     * Set the duration of the interval between two consecutive requests for channel
     * access made by the MultiUserScheduler.
     *
     * \param interval the duration of the interval between two consecutive requests
     *                 for channel access
     */
    void SetAccessReqInterval(Time interval);

  protected:
    /**
     * Get the station manager attached to the AP on the given link.
     *
     * \param linkId the ID of the given link
     * \return the station manager attached to the AP on the given link
     */
    Ptr<WifiRemoteStationManager> GetWifiRemoteStationManager(uint8_t linkId) const;

    /**
     * Get the HE Frame Exchange Manager attached to the AP on the given link.
     *
     * \param linkId the ID of the given link
     * \return the HE Frame Exchange Manager attached to the AP on the given link
     */
    Ptr<HeFrameExchangeManager> GetHeFem(uint8_t linkId) const;

    /**
     * Get an MPDU containing the given Trigger Frame.
     *
     * \param trigger the given Trigger Frame
     * \param linkId the ID of the link on which the Trigger Frame has to be sent
     * \return an MPDU containing the given Trigger Frame
     */
    Ptr<WifiMpdu> GetTriggerFrame(const CtrlTriggerHeader& trigger, uint8_t linkId) const;

    /**
     * Get the format of the last transmission on the given link, as determined by
     * the last call to NotifyAccessGranted that did not return NO_TX.
     *
     * \param linkId the ID of the given link
     * \return the format of the last transmission on the given link
     */
    TxFormat GetLastTxFormat(uint8_t linkId);

    /**
     * Get the maximum size in bytes among the A-MPDUs containing QoS Null frames
     * and solicited by the given (BSRP) Trigger Frame. For each station addressed
     * by the Trigger Frame, the expected response is an A-MPDU containing as many
     * QoS Null frames as the number of TIDs for which a BlockAck agreement has
     * been established between the station and the AP.
     *
     * \param trigger the given Trigger Frame
     * \return the maximum size in bytes among the A-MPDUs containing QoS Null frames
     *         and solicited by the given Trigger Frame
     */
    uint32_t GetMaxSizeOfQosNullAmpdu(const CtrlTriggerHeader& trigger) const;

    void DoDispose() override;
    void NotifyNewAggregate() override;
    void DoInitialize() override;

    Ptr<ApWifiMac> m_apMac;  //!< the AP wifi MAC
    Ptr<QosTxop> m_edca;     //!< the AC that gained channel access
    Time m_availableTime;    //!< the time available for frame exchange
    bool m_initialFrame;     //!< true if a TXOP is being started
    uint16_t m_allowedWidth; //!< the allowed width in MHz for the current transmission
    uint8_t m_linkId;        //!< the ID of the link over which channel access has been granted

  private:
    /**
     * Set the wifi MAC. Note that it must be the MAC of an HE AP.
     *
     * \param mac the AP wifi MAC
     */
    void SetWifiMac(Ptr<ApWifiMac> mac);

    /**
     * Perform actions required on expiration of the channel access request timer,
     * such as requesting channel access (if not requested already) and restarting
     * the channel access request timer.
     */
    void AccessReqTimeout();

    /**
     * Select the format of the next transmission.
     *
     * \return the format of the next transmission
     */
    virtual TxFormat SelectTxFormat() = 0;

    /**
     * Compute the information required to perform a DL MU transmission.
     *
     * \return the information required to perform a DL MU transmission
     */
    virtual DlMuInfo ComputeDlMuInfo() = 0;

    /**
     * Prepare the information required to solicit an UL MU transmission.
     *
     * \return the information required to solicit an UL MU transmission
     */
    virtual UlMuInfo ComputeUlMuInfo() = 0;

    /**
     * Ensure that the Trigger Frame returned in case of UL MU transmission is
     * correct. Currently, this method sets the CS Required, the AP Tx Power and
     * the UL Target Receive Power subfields.
     */
    void CheckTriggerFrame();

    /**
     * Type for the information about the last transmission
     */
    struct LastTxInfo
    {
        TxFormat lastTxFormat{NO_TX}; ///< the format of last transmission
        DlMuInfo dlInfo;              ///< information required to perform a DL MU transmission
        UlMuInfo ulInfo;              ///< information required to solicit an UL MU transmission
    };

    std::map<uint8_t, LastTxInfo> m_lastTxInfo; ///< Information about the last transmission
    EventId m_accessReqTimer;      ///< the timer controlling additional channel access requests
    Time m_accessReqInterval;      ///< duration of the interval between channel access requests
    AcIndex m_accessReqAc;         ///< AC we request channel access for
    bool m_restartTimerUponAccess; ///< whether the channel access timer has to be restarted
                                   ///< upon channel access
};

} // namespace ns3

#endif /* MULTI_USER_SCHEDULER_H */
