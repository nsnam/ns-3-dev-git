/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef LTE_RLC_H
#define LTE_RLC_H

#include "lte-mac-sap.h"
#include "lte-rlc-sap.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/uinteger.h"

namespace ns3
{

// class LteRlcSapProvider;
// class LteRlcSapUser;
//
// class LteMacSapProvider;
// class LteMacSapUser;

/**
 * This abstract base class defines the API to interact with the Radio Link Control
 * (LTE_RLC) in LTE, see 3GPP TS 36.322
 *
 */
class LteRlc : public Object // SimpleRefCount<LteRlc>
{
    /// allow LteRlcSpecificLteMacSapUser class friend access
    friend class LteRlcSpecificLteMacSapUser;
    /// allow LteRlcSpecificLteRlcSapProvider<LteRlc> class friend access
    friend class LteRlcSpecificLteRlcSapProvider<LteRlc>;

  public:
    LteRlc();
    ~LteRlc() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    void DoDispose() override;

    /**
     *
     *
     * @param rnti
     */
    void SetRnti(uint16_t rnti);

    /**
     *
     *
     * @param lcId
     */
    void SetLcId(uint8_t lcId);

    /**
     * @param packetDelayBudget
     */
    void SetPacketDelayBudgetMs(uint16_t packetDelayBudget);

    /**
     *
     *
     * @param s the RLC SAP user to be used by this LTE_RLC
     */
    void SetLteRlcSapUser(LteRlcSapUser* s);

    /**
     *
     *
     * @return the RLC SAP Provider interface offered to the PDCP by this LTE_RLC
     */
    LteRlcSapProvider* GetLteRlcSapProvider();

    /**
     *
     *
     * @param s the MAC SAP Provider to be used by this LTE_RLC
     */
    void SetLteMacSapProvider(LteMacSapProvider* s);

    /**
     *
     *
     * @return the MAC SAP User interface offered to the MAC by this LTE_RLC
     */
    LteMacSapUser* GetLteMacSapUser();

    /**
     * TracedCallback signature for NotifyTxOpportunity events.
     *
     * @param [in] rnti C-RNTI scheduled.
     * @param [in] lcid The logical channel id corresponding to
     *             the sending RLC instance.
     * @param [in] bytes The number of bytes to transmit
     */
    typedef void (*NotifyTxTracedCallback)(uint16_t rnti, uint8_t lcid, uint32_t bytes);

    /**
     * TracedCallback signature for
     *
     * @param [in] rnti C-RNTI scheduled.
     * @param [in] lcid The logical channel id corresponding to
     *             the sending RLC instance.
     * @param [in] bytes The packet size.
     * @param [in] delay Delay since sender timestamp, in ns.
     */
    typedef void (*ReceiveTracedCallback)(uint16_t rnti,
                                          uint8_t lcid,
                                          uint32_t bytes,
                                          uint64_t delay);

    /// @todo MRE What is the sense to duplicate all the interfaces here???
    // NB to avoid the use of multiple inheritance

  protected:
    // Interface forwarded by LteRlcSapProvider
    /**
     * Transmit PDCP PDU
     *
     * @param p packet
     */
    virtual void DoTransmitPdcpPdu(Ptr<Packet> p) = 0;

    LteRlcSapUser* m_rlcSapUser;         ///< RLC SAP user
    LteRlcSapProvider* m_rlcSapProvider; ///< RLC SAP provider

    // Interface forwarded by LteMacSapUser
    /**
     * Notify transmit opportunity
     *
     * @param params LteMacSapUser::TxOpportunityParameters
     */
    virtual void DoNotifyTxOpportunity(LteMacSapUser::TxOpportunityParameters params) = 0;
    /**
     * Notify HARQ delivery failure
     */
    virtual void DoNotifyHarqDeliveryFailure() = 0;
    /**
     * Receive PDU function
     *
     * @param params the LteMacSapUser::ReceivePduParameters
     */
    virtual void DoReceivePdu(LteMacSapUser::ReceivePduParameters params) = 0;

    LteMacSapUser* m_macSapUser;         ///< MAC SAP user
    LteMacSapProvider* m_macSapProvider; ///< MAC SAP provider

    uint16_t m_rnti; ///< RNTI
    uint8_t m_lcid;  ///< LCID
    uint16_t m_packetDelayBudgetMs{
        UINT16_MAX}; //!< the packet delay budget in ms of the corresponding logical channel

    /**
     * Used to inform of a PDU delivery to the MAC SAP provider
     */
    TracedCallback<uint16_t, uint8_t, uint32_t> m_txPdu;
    /**
     * Used to inform of a PDU reception from the MAC SAP user
     */
    TracedCallback<uint16_t, uint8_t, uint32_t, uint64_t> m_rxPdu;
    /**
     * The trace source fired when the RLC drops a packet before
     * transmission.
     */
    TracedCallback<Ptr<const Packet>> m_txDropTrace;
};

/**
 * LTE_RLC Saturation Mode (SM): simulation-specific mode used for
 * experiments that do not need to consider the layers above the LTE_RLC.
 * The LTE_RLC SM, unlike the standard LTE_RLC modes, it does not provide
 * data delivery services to upper layers; rather, it just generates a
 * new LTE_RLC PDU whenever the MAC notifies a transmission opportunity.
 *
 */
class LteRlcSm : public LteRlc
{
  public:
    LteRlcSm();
    ~LteRlcSm() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    void DoInitialize() override;
    void DoDispose() override;

    void DoTransmitPdcpPdu(Ptr<Packet> p) override;
    void DoNotifyTxOpportunity(LteMacSapUser::TxOpportunityParameters txOpParams) override;
    void DoNotifyHarqDeliveryFailure() override;
    void DoReceivePdu(LteMacSapUser::ReceivePduParameters rxPduParams) override;

  private:
    /// Report buffer status
    void ReportBufferStatus();
};

// /**
//  * Implements LTE_RLC Transparent Mode (TM), see  3GPP TS 36.322
//  *
//  */
// class LteRlcTm : public LteRlc
// {
// public:
//   virtual ~LteRlcTm ();

// };

// /**
//  * Implements LTE_RLC Unacknowledged Mode (UM), see  3GPP TS 36.322
//  *
//  */
// class LteRlcUm : public LteRlc
// {
// public:
//   virtual ~LteRlcUm ();

// };

// /**
//  * Implements LTE_RLC Acknowledged Mode (AM), see  3GPP TS 36.322
//  *
//  */

// class LteRlcAm : public LteRlc
// {
// public:
//   virtual ~LteRlcAm ();
// };

} // namespace ns3

#endif // LTE_RLC_H
