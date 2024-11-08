/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef HT_OPERATION_H
#define HT_OPERATION_H

#include "ns3/wifi-information-element.h"

/**
 * This defines the maximum number of supported MCSs that a STA is
 * allowed to have. Currently this number is set for IEEE 802.11n
 */
#define MAX_SUPPORTED_MCS (77)

namespace ns3
{

/// HtProtectionType enumeration
enum HtProtectionType
{
    NO_PROTECTION,
    NON_MEMBER_PROTECTION,
    TWENTY_MHZ_PROTECTION,
    MIXED_MODE_PROTECTION
};

/**
 * @brief The HT Operation Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise
 * the HT Operation Information Element
 */
class HtOperation : public WifiInformationElement
{
  public:
    HtOperation();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the Primary Channel field in the HT Operation information element.
     *
     * @param ctrl the Primary Channel field in the HT Operation information element
     */
    void SetPrimaryChannel(uint8_t ctrl);
    /**
     * Set the Information Subset 1 field in the HT Operation information element.
     *
     * @param ctrl the Information Subset 1 field in the HT Operation information element
     */
    void SetInformationSubset1(uint8_t ctrl);
    /**
     * Set the Information Subset 2 field in the HT Operation information element.
     *
     * @param ctrl the Information Subset 2 field in the HT Operation information element
     */
    void SetInformationSubset2(uint16_t ctrl);
    /**
     * Set the Information Subset 3 field in the HT Operation information element.
     *
     * @param ctrl the Information Subset 3 field in the HT Operation information element
     */
    void SetInformationSubset3(uint16_t ctrl);
    /**
     * Set the Basic MCS Set field in the HT Operation information element.
     *
     * @param ctrl1 the first 64 bytes of the Basic MCS Set field in the HT Operation
     * information element
     * @param ctrl2 the last 64 bytes of the Basic MCS Set field in the HT Operation
     * information element
     */
    void SetBasicMcsSet(uint64_t ctrl1, uint64_t ctrl2);

    /**
     * Set the secondary channel offset.
     *
     * @param secondaryChannelOffset the secondary channel offset
     */
    void SetSecondaryChannelOffset(uint8_t secondaryChannelOffset);
    /**
     * Set the STA channel width.
     *
     * @param staChannelWidth the STA channel width
     */
    void SetStaChannelWidth(uint8_t staChannelWidth);
    /**
     * Set the RIFS mode.
     *
     * @param rifsMode the RIFS mode
     */
    void SetRifsMode(uint8_t rifsMode);

    /**
     * Set the HT protection.
     *
     * @param htProtection the HT protection
     */
    void SetHtProtection(uint8_t htProtection);
    /**
     * Set the non GF HT STAs present.
     *
     * @param nonGfHtStasPresent the non GF HT STAs present
     */
    void SetNonGfHtStasPresent(uint8_t nonGfHtStasPresent);
    /**
     * Set the OBSS non HT STAs present.
     *
     * @param obssNonHtStasPresent the OBSS non HTA STAs present
     */
    void SetObssNonHtStasPresent(uint8_t obssNonHtStasPresent);

    /**
     * Set the dual beacon.
     *
     * @param dualBeacon the dual beacon
     */
    void SetDualBeacon(uint8_t dualBeacon);
    /**
     * Set the dual CTS protection.
     *
     * @param dualCtsProtection the dual CTS protection
     */
    void SetDualCtsProtection(uint8_t dualCtsProtection);
    /**
     * Set the STBC beacon.
     *
     * @param stbcBeacon the STBC beacon
     */
    void SetStbcBeacon(uint8_t stbcBeacon);
    /**
     * Set the LSIG TXOP protection full support.
     *
     * @param lSigTxopProtectionFullSupport the LSIG TXOP protection full support
     */
    void SetLSigTxopProtectionFullSupport(uint8_t lSigTxopProtectionFullSupport);
    /**
     * Set the PCO active.
     *
     * @param pcoActive the PCO active
     */
    void SetPcoActive(uint8_t pcoActive);
    /**
     * Set the PCO phase.
     *
     * @param pcoPhase the PCO phase
     */
    void SetPhase(uint8_t pcoPhase);

    /**
     * Set the receive MCS bitmask.
     *
     * @param index the MCS bitmask
     */
    void SetRxMcsBitmask(uint8_t index);
    /**
     * Set the receive highest supported data rate.
     *
     * @param maxSupportedRate the maximum supported data rate
     */
    void SetRxHighestSupportedDataRate(uint16_t maxSupportedRate);
    /**
     * Set the transmit MCS set defined.
     *
     * @param txMcsSetDefined the transmit MCS set defined
     */
    void SetTxMcsSetDefined(uint8_t txMcsSetDefined);
    /**
     * Set the transmit / receive MCS set unequal.
     *
     * @param txRxMcsSetUnequal the transmit / receive MCS set unequal
     */
    void SetTxRxMcsSetUnequal(uint8_t txRxMcsSetUnequal);
    /**
     * Set the transmit maximum number spatial streams.
     *
     * @param maxTxSpatialStreams the maximum transmit spatial streams
     */
    void SetTxMaxNSpatialStreams(uint8_t maxTxSpatialStreams);
    /**
     * Set the transmit unequal modulation.
     *
     * @param txUnequalModulation the transmit unequal modulation
     */
    void SetTxUnequalModulation(uint8_t txUnequalModulation);

    /**
     * Return the Primary Channel field in the HT Operation information element.
     *
     * @return the Primary Channel field in the HT Operation information element
     */
    uint8_t GetPrimaryChannel() const;
    /**
     * Return the Information Subset 1 field in the HT Operation information element.
     *
     * @return the Information Subset 1 field in the HT Operation information element
     */
    uint8_t GetInformationSubset1() const;
    /**
     * Return the Information Subset 2 field in the HT Operation information element.
     *
     * @return the Information Subset 2 field in the HT Operation information element
     */
    uint16_t GetInformationSubset2() const;
    /**
     * Return the Information Subset 3 field in the HT Operation information element.
     *
     * @return the Information Subset 3 field in the HT Operation information element
     */
    uint16_t GetInformationSubset3() const;
    /**
     * Return the first 64 bytes of the Basic MCS Set field in the HT Operation information element.
     *
     * @return the first 64 bytes of the Basic MCS Set field in the HT Operation information element
     */
    uint64_t GetBasicMcsSet1() const;
    /**
     * Return the last 64 bytes of the Basic MCS Set field in the HT Operation information element.
     *
     * @return the last 64 bytes of the Basic MCS Set field in the HT Operation information element
     */
    uint64_t GetBasicMcsSet2() const;

    /**
     * Return the secondary channel offset.
     *
     * @return the secondary channel offset
     */
    uint8_t GetSecondaryChannelOffset() const;
    /**
     * Return the STA channel width.
     *
     * @return the STA channel width
     */
    uint8_t GetStaChannelWidth() const;
    /**
     * Return the RIFS mode.
     *
     * @return the RIFS mode
     */
    uint8_t GetRifsMode() const;

    /**
     * Return the HT protection.
     *
     * @return the HT protection
     */
    uint8_t GetHtProtection() const;
    /**
     * Return the non GF HT STAs present.
     *
     * @return the non GF HT STAs present
     */
    uint8_t GetNonGfHtStasPresent() const;
    /**
     * Return the OBSS non HT STAs present.
     *
     * @return the OBSS non HT STAs present
     */
    uint8_t GetObssNonHtStasPresent() const;

    /**
     * Return dual beacon.
     *
     * @return the dual beacon
     */
    uint8_t GetDualBeacon() const;
    /**
     * Return dual CTS protection.
     *
     * @return the dual CTS protection
     */
    uint8_t GetDualCtsProtection() const;
    /**
     * Return STBC beacon.
     *
     * @return the STBC beacon
     */
    uint8_t GetStbcBeacon() const;
    /**
     * Return LSIG TXOP protection full support.
     *
     * @return the LSIG TXOP protection full support
     */
    uint8_t GetLSigTxopProtectionFullSupport() const;
    /**
     * Return PCO active.
     *
     * @return the PCO active
     */
    uint8_t GetPcoActive() const;
    /**
     * Return phase.
     *
     * @return the phase
     */
    uint8_t GetPhase() const;

    /**
     * Return MCS is supported.
     *
     * @param mcs MCS
     *
     * @return the MCS is supported
     */
    bool IsSupportedMcs(uint8_t mcs) const;
    /**
     * Return receive highest supported data rate.
     *
     * @return receive highest supported data rate
     */
    uint16_t GetRxHighestSupportedDataRate() const;
    /**
     * Return transmit MCS set defined.
     *
     * @return the transmit MCS set defined
     */
    uint8_t GetTxMcsSetDefined() const;
    /**
     * Return transmit / receive MCS set unequal.
     *
     * @return transmit / receive MCS set unequal
     */
    uint8_t GetTxRxMcsSetUnequal() const;
    /**
     * Return transmit maximum number spatial streams.
     *
     * @return transmit maximum number spatial streams
     */
    uint8_t GetTxMaxNSpatialStreams() const;
    /**
     * Return transmit unequal modulation.
     *
     * @return transmit unequal modulation
     */
    uint8_t GetTxUnequalModulation() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    uint8_t m_primaryChannel; ///< primary channel

    // HT Information Subset 1
    uint8_t m_secondaryChannelOffset;     ///< secondary channel offset
    uint8_t m_staChannelWidth;            ///< STA channel width
    uint8_t m_rifsMode;                   ///< RIFS mode
    uint8_t m_reservedInformationSubset1; ///< reserved information subset 1

    // HT Information Subset 2
    uint8_t m_htProtection;                 ///< HT protection
    uint8_t m_nonGfHtStasPresent;           ///< non GF HT STAs present
    uint8_t m_reservedInformationSubset2_1; ///< reserved information subset 2-1
    uint8_t m_obssNonHtStasPresent;         ///< OBSS NON HT STAs present
    uint8_t m_reservedInformationSubset2_2; ///< reserved information subset 2-2

    // HT Information Subset 3
    uint8_t m_reservedInformationSubset3_1;  ///< reserved information subset 3-1
    uint8_t m_dualBeacon;                    ///< dual beacon
    uint8_t m_dualCtsProtection;             ///< dual CTS protection
    uint8_t m_stbcBeacon;                    ///< STBC beacon
    uint8_t m_lSigTxopProtectionFullSupport; ///< L-SIG TXOP protection full support
    uint8_t m_pcoActive;                     ///< PCO active
    uint8_t m_pcoPhase;                      ///< PCO phase
    uint8_t m_reservedInformationSubset3_2;  ///< reserved information subset 3-2

    // Basic MCS Set field
    uint8_t m_reservedMcsSet1;                 ///< reserved MCS set 1
    uint16_t m_rxHighestSupportedDataRate;     ///< receive highest supported data rate
    uint8_t m_reservedMcsSet2;                 ///< reserved MCS set2
    uint8_t m_txMcsSetDefined;                 ///< transmit MCS set defined
    uint8_t m_txRxMcsSetUnequal;               ///< transmit / receive MCS set unequal
    uint8_t m_txMaxNSpatialStreams;            ///< transmit maximum number spatial streams
    uint8_t m_txUnequalModulation;             ///< transmit unequal modulation
    uint32_t m_reservedMcsSet3;                ///< reserved MCS set 3
    uint8_t m_rxMcsBitmask[MAX_SUPPORTED_MCS]; ///< receive MCS bitmask
};

} // namespace ns3

#endif /* HT_OPERATION_H */
