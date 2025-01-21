/*
 * Copyright (c) 2020 Orange Labs
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 */

#ifndef VHT_PHY_H
#define VHT_PHY_H

#include "ns3/ht-phy.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::VhtPhy class.
 */

namespace ns3
{

/**
 * This defines the BSS membership value for VHT PHY.
 */
#define VHT_PHY 126

/**
 * @brief PHY entity for VHT (11ac)
 * @ingroup wifi
 *
 * VHT PHY is based on HT PHY.
 *
 * Refer to IEEE 802.11-2016, clause 21.
 */
class VhtPhy : public HtPhy
{
  public:
    /**
     * Constructor for VHT PHY
     *
     * @param buildModeList flag used to add VHT modes to list (disabled
     *                      by child classes to only add child classes' modes)
     */
    VhtPhy(bool buildModeList = true);
    /**
     * Destructor for VHT PHY
     */
    ~VhtPhy() override;

    WifiMode GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const override;
    const PpduFormats& GetPpduFormats() const override;
    Time GetDuration(WifiPpduField field, const WifiTxVector& txVector) const override;
    Time GetLSigDuration(WifiPreamble preamble) const override;
    Time GetTrainingDuration(const WifiTxVector& txVector,
                             uint8_t nDataLtf,
                             uint8_t nExtensionLtf = 0) const override;
    Ptr<WifiPpdu> BuildPpdu(const WifiConstPsduMap& psdus,
                            const WifiTxVector& txVector,
                            Time ppduDuration) override;
    dBm_u GetCcaThreshold(const Ptr<const WifiPpdu> ppdu,
                          WifiChannelListType channelType) const override;

    /**
     * @return the WifiMode used for the SIG-A field
     */
    virtual WifiMode GetSigAMode() const;
    /**
     * @param txVector the transmission parameters
     * @return the WifiMode used for the SIG-B field
     */
    virtual WifiMode GetSigBMode(const WifiTxVector& txVector) const;

    /**
     * @param preamble the type of preamble
     * @return the duration of the SIG-A field
     */
    virtual Time GetSigADuration(WifiPreamble preamble) const;
    /**
     * @param txVector the transmission parameters
     * @return the duration of the SIG-B field
     */
    virtual Time GetSigBDuration(const WifiTxVector& txVector) const;

    /**
     * Initialize all VHT modes.
     */
    static void InitializeModes();
    /**
     * Return the VHT MCS corresponding to
     * the provided index.
     *
     * @param index the index of the MCS
     * @return an VHT MCS
     */
    static WifiMode GetVhtMcs(uint8_t index);

    /**
     * Return MCS 0 from VHT MCS values.
     *
     * @return MCS 0 from VHT MCS values
     */
    static WifiMode GetVhtMcs0();
    /**
     * Return MCS 1 from VHT MCS values.
     *
     * @return MCS 1 from VHT MCS values
     */
    static WifiMode GetVhtMcs1();
    /**
     * Return MCS 2 from VHT MCS values.
     *
     * @return MCS 2 from VHT MCS values
     */
    static WifiMode GetVhtMcs2();
    /**
     * Return MCS 3 from VHT MCS values.
     *
     * @return MCS 3 from VHT MCS values
     */
    static WifiMode GetVhtMcs3();
    /**
     * Return MCS 4 from VHT MCS values.
     *
     * @return MCS 4 from VHT MCS values
     */
    static WifiMode GetVhtMcs4();
    /**
     * Return MCS 5 from VHT MCS values.
     *
     * @return MCS 5 from VHT MCS values
     */
    static WifiMode GetVhtMcs5();
    /**
     * Return MCS 6 from VHT MCS values.
     *
     * @return MCS 6 from VHT MCS values
     */
    static WifiMode GetVhtMcs6();
    /**
     * Return MCS 7 from VHT MCS values.
     *
     * @return MCS 7 from VHT MCS values
     */
    static WifiMode GetVhtMcs7();
    /**
     * Return MCS 8 from VHT MCS values.
     *
     * @return MCS 8 from VHT MCS values
     */
    static WifiMode GetVhtMcs8();
    /**
     * Return MCS 9 from VHT MCS values.
     *
     * @return MCS 9 from VHT MCS values
     */
    static WifiMode GetVhtMcs9();

    /**
     * Return the coding rate corresponding to
     * the supplied VHT MCS index. This function is
     * reused by child classes and is used as a callback
     * for WifiMode operation.
     *
     * @param mcsValue the MCS index
     * @return the coding rate.
     */
    static WifiCodeRate GetCodeRate(uint8_t mcsValue);
    /**
     * Return the constellation size corresponding
     * to the supplied VHT MCS index. This function is
     * reused by child classes and is used as a callback for
     * WifiMode operation.
     *
     * @param mcsValue the MCS index
     * @return the size of modulation constellation.
     */
    static uint16_t GetConstellationSize(uint8_t mcsValue);
    /**
     * Return the PHY rate corresponding to the supplied VHT MCS
     * index, channel width, guard interval, and number of
     * spatial stream. This function calls HtPhy::CalculatePhyRate
     * and is mainly used as a callback for WifiMode operation.
     *
     * @param mcsValue the VHT MCS index
     * @param channelWidth the considered channel width
     * @param guardInterval the considered guard interval duration
     * @param nss the considered number of stream
     *
     * @return the physical bit rate of this signal in bps.
     */
    static uint64_t GetPhyRate(uint8_t mcsValue,
                               MHz_u channelWidth,
                               Time guardInterval,
                               uint8_t nss);
    /**
     * Return the PHY rate corresponding to
     * the supplied TXVECTOR.
     * This function is mainly used as a callback
     * for WifiMode operation.
     *
     * @param txVector the TXVECTOR used for the transmission
     * @param staId the station ID (only here to have a common signature for all callbacks)
     * @return the physical bit rate of this signal in bps.
     */
    static uint64_t GetPhyRateFromTxVector(const WifiTxVector& txVector, uint16_t staId);
    /**
     * Return the data rate corresponding to
     * the supplied TXVECTOR.
     * This function is mainly used as a callback
     * for WifiMode operation.
     *
     * @param txVector the TXVECTOR used for the transmission
     * @param staId the station ID (only here to have a common signature for all callbacks)
     * @return the data bit rate in bps.
     */
    static uint64_t GetDataRateFromTxVector(const WifiTxVector& txVector, uint16_t staId);
    /**
     * Return the data rate corresponding to
     * the supplied VHT MCS index, channel width,
     * guard interval, and number of spatial
     * streams.
     *
     * @param mcsValue the MCS index
     * @param channelWidth the channel width
     * @param guardInterval the guard interval duration
     * @param nss the number of spatial streams
     * @return the data bit rate in bps.
     */
    static uint64_t GetDataRate(uint8_t mcsValue,
                                MHz_u channelWidth,
                                Time guardInterval,
                                uint8_t nss);
    /**
     * Calculate the rate in bps of the non-HT Reference Rate corresponding
     * to the supplied VHT MCS index. This function calls CalculateNonHtReferenceRate
     * and is used as a callback for WifiMode operation.
     *
     * @param mcsValue the VHT MCS index
     * @return the rate in bps of the non-HT Reference Rate.
     */
    static uint64_t GetNonHtReferenceRate(uint8_t mcsValue);
    /**
     * Check whether the combination of <MCS, channel width, NSS> is allowed.
     * This function is used as a callback for WifiMode operation.
     *
     * @param mcsValue the considered MCS index
     * @param channelWidth the considered channel width
     * @param nss the considered number of streams
     * @returns true if this <MCS, channel width, NSS> combination is allowed, false otherwise.
     */
    static bool IsCombinationAllowed(uint8_t mcsValue, MHz_u channelWidth, uint8_t nss);
    /**
     * Check whether the combination in TXVECTOR is allowed.
     * This function is used as a callback for WifiMode operation.
     *
     * @param txVector the TXVECTOR
     * @returns true if this combination is allowed, false otherwise.
     */
    static bool IsAllowed(const WifiTxVector& txVector);

  protected:
    WifiMode GetHtSigMode() const override;
    Time GetHtSigDuration() const override;
    uint8_t GetNumberBccEncoders(const WifiTxVector& txVector) const override;
    PhyFieldRxStatus DoEndReceiveField(WifiPpduField field, Ptr<Event> event) override;
    bool IsAllConfigSupported(WifiPpduField field, Ptr<const WifiPpdu> ppdu) const override;
    uint32_t GetMaxPsduSize() const override;
    CcaIndication GetCcaIndication(const Ptr<const WifiPpdu> ppdu) override;

    /**
     * End receiving the SIG-A or SIG-B, perform VHT-specific actions, and
     * provide the status of the reception.
     *
     * Child classes can perform amendment-specific actions by specializing
     * @see ProcessSig.
     *
     * @param event the event holding incoming PPDU's information
     * @param field the current PPDU field
     * @return status of the reception of the SIG-A of SIG-B
     */
    PhyFieldRxStatus EndReceiveSig(Ptr<Event> event, WifiPpduField field);

    /**
     * Get the failure reason corresponding to the unsuccessful processing of a given PPDU field.
     *
     * @param field the PPDU field
     * @return the failure reason corresponding to the unsuccessful processing of the PPDU field
     */
    virtual WifiPhyRxfailureReason GetFailureReason(WifiPpduField field) const;

    /**
     * Process SIG-A or SIG-B, perform amendment-specific actions, and
     * provide an updated status of the reception.
     *
     * @param event the event holding incoming PPDU's information
     * @param status the status of the reception of the correctly received SIG-A or SIG-B after the
     * configuration support check
     * @param field the current PPDU field to identify whether it is SIG-A or SIG-B
     * @return the updated status of the reception of the SIG-A or SIG-B
     */
    virtual PhyFieldRxStatus ProcessSig(Ptr<Event> event,
                                        PhyFieldRxStatus status,
                                        WifiPpduField field);

    /**
     * Return the rate (in bps) of the non-HT Reference Rate
     * which corresponds to the supplied code rate and
     * constellation size.
     *
     * @param codeRate the convolutional coding rate
     * @param constellationSize the size of modulation constellation
     * @returns the rate in bps.
     *
     * To convert an VHT MCS to its corresponding non-HT Reference Rate
     * use the modulation and coding rate of the HT MCS
     * and lookup in Table 10-7 of IEEE 802.11-2016.
     */
    static uint64_t CalculateNonHtReferenceRate(WifiCodeRate codeRate, uint16_t constellationSize);
    /**
     * @param channelWidth the channel width
     * @return he number of usable subcarriers for data
     */
    static uint16_t GetUsableSubcarriers(MHz_u channelWidth);

  private:
    void BuildModeList() override;

    /**
     * Return the VHT MCS corresponding to
     * the provided index.
     * This method binds all the callbacks used by WifiMode.
     *
     * @param index the index of the MCS
     * @return a VHT MCS
     */
    static WifiMode CreateVhtMcs(uint8_t index);

    /**
     * Typedef for storing exceptions in the number of BCC encoders for VHT MCSs
     */
    typedef std::map<
        std::tuple<MHz_u /* channelWidth */, uint8_t /* Nss */, uint8_t /* MCS index */>,
        uint8_t /* Nes */>
        NesExceptionMap;
    static const NesExceptionMap m_exceptionsMap; //!< exception map for number of BCC encoders
                                                  //!< (extracted from VHT-MCS tables)
    static const PpduFormats m_vhtPpduFormats;    //!< VHT PPDU formats
};

} // namespace ns3

#endif /* VHT_PHY_H */
