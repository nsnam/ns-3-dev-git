/*
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy and
 * spectrum-wifi-phy)
 */

#ifndef HE_PHY_H
#define HE_PHY_H

#include "he-ppdu.h"

#include "ns3/callback.h"
#include "ns3/vht-phy.h"
#include "ns3/wifi-phy-band.h"

#include <optional>

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::HePhy class
 * and ns3::HeSigAParameters struct.
 */

namespace ns3
{

class ObssPdAlgorithm;

/**
 * This defines the BSS membership value for HE PHY.
 */
#define HE_PHY 122

/**
 * Parameters for received HE-SIG-A for OBSS_PD based SR
 */
struct HeSigAParameters
{
    double rssiW;     ///< RSSI in W
    uint8_t bssColor; ///< BSS color
};

/**
 * \brief PHY entity for HE (11ax)
 * \ingroup wifi
 *
 * HE PHY is based on VHT PHY.
 *
 * Refer to P802.11ax/D4.0, clause 27.
 */
class HePhy : public VhtPhy
{
  public:
    /**
     * Callback upon end of HE-SIG-A
     *
     * arg1: Parameters of HE-SIG-A
     */
    typedef Callback<void, HeSigAParameters> EndOfHeSigACallback;

    /**
     * Constructor for HE PHY
     *
     * \param buildModeList flag used to add HE modes to list (disabled
     *                      by child classes to only add child classes' modes)
     */
    HePhy(bool buildModeList = true);
    /**
     * Destructor for HE PHY
     */
    ~HePhy() override;

    WifiMode GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const override;
    WifiMode GetSigAMode() const override;
    WifiMode GetSigBMode(const WifiTxVector& txVector) const override;
    const PpduFormats& GetPpduFormats() const override;
    Time GetLSigDuration(WifiPreamble preamble) const override;
    Time GetTrainingDuration(const WifiTxVector& txVector,
                             uint8_t nDataLtf,
                             uint8_t nExtensionLtf = 0) const override;
    Time GetSigADuration(WifiPreamble preamble) const override;
    Time GetSigBDuration(const WifiTxVector& txVector) const override;
    Ptr<WifiPpdu> BuildPpdu(const WifiConstPsduMap& psdus,
                            const WifiTxVector& txVector,
                            Time ppduDuration) override;
    Ptr<const WifiPsdu> GetAddressedPsduInPpdu(Ptr<const WifiPpdu> ppdu) const override;
    void StartReceivePreamble(Ptr<const WifiPpdu> ppdu,
                              RxPowerWattPerChannelBand& rxPowersW,
                              Time rxDuration) override;
    void CancelAllEvents() override;
    uint16_t GetStaId(const Ptr<const WifiPpdu> ppdu) const override;
    uint16_t GetMeasurementChannelWidth(const Ptr<const WifiPpdu> ppdu) const override;
    void StartTx(Ptr<const WifiPpdu> ppdu) override;
    Time CalculateTxDuration(WifiConstPsduMap psduMap,
                             const WifiTxVector& txVector,
                             WifiPhyBand band) const override;
    void SwitchMaybeToCcaBusy(const Ptr<const WifiPpdu> ppdu) override;
    double GetCcaThreshold(const Ptr<const WifiPpdu> ppdu,
                           WifiChannelListType channelType) const override;
    void NotifyCcaBusy(const Ptr<const WifiPpdu> ppdu,
                       Time duration,
                       WifiChannelListType channelType) override;
    bool CanStartRx(Ptr<const WifiPpdu> ppdu) const override;
    Ptr<const WifiPpdu> GetRxPpduFromTxPpdu(Ptr<const WifiPpdu> ppdu) override;

    /**
     * \return the BSS color of this PHY.
     */
    uint8_t GetBssColor() const;

    /**
     * Compute the L-SIG length value corresponding to the given HE TB PPDU duration.
     * If the latter is not a feasible duration (considering the selected guard interval),
     * a proper duration is computed and returned along with the L-SIG length value.
     *
     * \param ppduDuration the duration of the HE TB PPDU
     * \param txVector the TXVECTOR used for the transmission of this HE TB PPDU
     * \param band the frequency band being used
     *
     * \return the L-SIG length value and the adjusted HE TB PPDU duration.
     */
    static std::pair<uint16_t, Time> ConvertHeTbPpduDurationToLSigLength(
        Time ppduDuration,
        const WifiTxVector& txVector,
        WifiPhyBand band);
    /**
     * \param length the L-SIG length value
     * \param txVector the TXVECTOR used for the transmission of this HE TB PPDU
     * \param band the frequency band being used
     *
     * \return the duration of the HE TB PPDU corresponding to that L-SIG length value.
     */
    static Time ConvertLSigLengthToHeTbPpduDuration(uint16_t length,
                                                    const WifiTxVector& txVector,
                                                    WifiPhyBand band);
    /**
     * \param txVector the transmission parameters used for the HE TB PPDU
     *
     * \return the duration of the non-HE portion of the HE TB PPDU.
     */
    virtual Time CalculateNonHeDurationForHeTb(const WifiTxVector& txVector) const;

    /**
     * \param txVector the transmission parameters used for the HE MU PPDU
     *
     * \return the duration of the non-HE portion of the HE MU PPDU.
     */
    virtual Time CalculateNonHeDurationForHeMu(const WifiTxVector& txVector) const;

    /**
     * Get the band in the TX spectrum associated with the RU used by the PSDU
     * transmitted to/by a given STA in a DL MU PPDU/HE TB PPDU
     *
     * \param txVector the TXVECTOR used for the transmission
     * \param staId the STA-ID of the station
     *
     * \return the RU band in the TX spectrum
     */
    WifiSpectrumBandInfo GetRuBandForTx(const WifiTxVector& txVector, uint16_t staId) const;
    /**
     * Get the band in the RX spectrum associated with the RU used by the PSDU
     * transmitted to/by a given STA in a DL MU PPDU/HE TB PPDU
     *
     * \param txVector the TXVECTOR used for the transmission
     * \param staId the STA-ID of the station
     *
     * \return the RU band in the RX spectrum
     */
    WifiSpectrumBandInfo GetRuBandForRx(const WifiTxVector& txVector, uint16_t staId) const;
    /**
     * Get the band used to transmit the non-OFDMA part of an HE TB PPDU.
     *
     * \param txVector the TXVECTOR used for the transmission
     * \param staId the STA-ID of the station taking part of the UL MU
     *
     * \return the spectrum band used to transmit the non-OFDMA part of an HE TB PPDU
     */
    WifiSpectrumBandInfo GetNonOfdmaBand(const WifiTxVector& txVector, uint16_t staId) const;
    /**
     * Get the width in MHz of the non-OFDMA portion of an HE TB PPDU
     *
     * \param ru the RU in which the HE TB PPDU is sent
     * \return the width in MHz of the non-OFDMA portion of an HE TB PPDU
     */
    uint16_t GetNonOfdmaWidth(HeRu::RuSpec ru) const;

    /**
     * \return the UID of the HE TB PPDU being received
     */
    uint64_t GetCurrentHeTbPpduUid() const;

    /**
     * Set the TRIGVECTOR and the associated expiration time. A TRIGVECTOR shall expire
     * when the TX timer associated with the transmission of the Trigger Frame expires.
     *
     * \param trigVector the TRIGVECTOR
     * \param validity the amount of time (from now) until expiration of the TRIGVECTOR
     */
    void SetTrigVector(const WifiTxVector& trigVector, Time validity);

    /**
     * Get the center frequency of the non-HE portion of the current TxVector for the
     * given STA-ID.
     * Note this method is only to be used for UL MU.
     *
     * \param txVector the TXVECTOR that has the RU allocation
     * \param staId the STA-ID of the station taking part of the UL MU
     * \return the center frequency in MHz corresponding to the non-HE portion of the HE TB PPDU
     */
    uint16_t GetCenterFrequencyForNonHePart(const WifiTxVector& txVector, uint16_t staId) const;

    /**
     * Sets the OBSS-PD algorithm.
     *
     * \param algorithm the OBSS-PD algorithm
     */
    void SetObssPdAlgorithm(const Ptr<ObssPdAlgorithm> algorithm);

    /**
     * Set a callback for a end of HE-SIG-A.
     *
     * \param callback the EndOfHeSigACallback to set
     */
    void SetEndOfHeSigACallback(EndOfHeSigACallback callback);

    /**
     * Fire a EndOfHeSigA callback (if connected) once HE-SIG-A field has been received.
     * This method is scheduled immediately after end of HE-SIG-A, once
     * field processing is finished.
     *
     * \param params the HE-SIG-A parameters
     */
    void NotifyEndOfHeSigA(HeSigAParameters params);

    /**
     * Initialize all HE modes.
     */
    static void InitializeModes();
    /**
     * Return the HE MCS corresponding to
     * the provided index.
     *
     * \param index the index of the MCS
     * \return an HE MCS
     */
    static WifiMode GetHeMcs(uint8_t index);

    /**
     * Return MCS 0 from HE MCS values.
     *
     * \return MCS 0 from HE MCS values
     */
    static WifiMode GetHeMcs0();
    /**
     * Return MCS 1 from HE MCS values.
     *
     * \return MCS 1 from HE MCS values
     */
    static WifiMode GetHeMcs1();
    /**
     * Return MCS 2 from HE MCS values.
     *
     * \return MCS 2 from HE MCS values
     */
    static WifiMode GetHeMcs2();
    /**
     * Return MCS 3 from HE MCS values.
     *
     * \return MCS 3 from HE MCS values
     */
    static WifiMode GetHeMcs3();
    /**
     * Return MCS 4 from HE MCS values.
     *
     * \return MCS 4 from HE MCS values
     */
    static WifiMode GetHeMcs4();
    /**
     * Return MCS 5 from HE MCS values.
     *
     * \return MCS 5 from HE MCS values
     */
    static WifiMode GetHeMcs5();
    /**
     * Return MCS 6 from HE MCS values.
     *
     * \return MCS 6 from HE MCS values
     */
    static WifiMode GetHeMcs6();
    /**
     * Return MCS 7 from HE MCS values.
     *
     * \return MCS 7 from HE MCS values
     */
    static WifiMode GetHeMcs7();
    /**
     * Return MCS 8 from HE MCS values.
     *
     * \return MCS 8 from HE MCS values
     */
    static WifiMode GetHeMcs8();
    /**
     * Return MCS 9 from HE MCS values.
     *
     * \return MCS 9 from HE MCS values
     */
    static WifiMode GetHeMcs9();
    /**
     * Return MCS 10 from HE MCS values.
     *
     * \return MCS 10 from HE MCS values
     */
    static WifiMode GetHeMcs10();
    /**
     * Return MCS 11 from HE MCS values.
     *
     * \return MCS 11 from HE MCS values
     */
    static WifiMode GetHeMcs11();

    /**
     * Return the coding rate corresponding to
     * the supplied HE MCS index. This function is used
     * as a callback for WifiMode operation.
     *
     * \param mcsValue the MCS index
     * \return the coding rate.
     */
    static WifiCodeRate GetCodeRate(uint8_t mcsValue);
    /**
     * Return the constellation size corresponding
     * to the supplied HE MCS index. This function is used
     * as a callback for WifiMode operation.
     *
     * \param mcsValue the MCS index
     * \return the size of modulation constellation.
     */
    static uint16_t GetConstellationSize(uint8_t mcsValue);
    /**
     * Return the PHY rate corresponding to the supplied HE MCS
     * index, channel width, guard interval, and number of
     * spatial stream. This function calls HtPhy::CalculatePhyRate
     * and is mainly used as a callback for WifiMode operation.
     *
     * \param mcsValue the HE MCS index
     * \param channelWidth the considered channel width in MHz
     * \param guardInterval the considered guard interval duration in nanoseconds
     * \param nss the considered number of stream
     *
     * \return the physical bit rate of this signal in bps.
     */
    static uint64_t GetPhyRate(uint8_t mcsValue,
                               uint16_t channelWidth,
                               uint16_t guardInterval,
                               uint8_t nss);
    /**
     * Return the PHY rate corresponding to
     * the supplied TXVECTOR for the STA-ID.
     *
     * \param txVector the TXVECTOR used for the transmission
     * \param staId the station ID for MU (unused if SU)
     * \return the physical bit rate of this signal in bps.
     */
    static uint64_t GetPhyRateFromTxVector(const WifiTxVector& txVector,
                                           uint16_t staId = SU_STA_ID);
    /**
     * Return the data rate corresponding to
     * the supplied TXVECTOR for the STA-ID.
     *
     * \param txVector the TXVECTOR used for the transmission
     * \param staId the station ID for MU (unused if SU)
     * \return the data bit rate in bps.
     */
    static uint64_t GetDataRateFromTxVector(const WifiTxVector& txVector,
                                            uint16_t staId = SU_STA_ID);
    /**
     * Return the data rate corresponding to
     * the supplied HE MCS index, channel width,
     * guard interval, and number of spatial
     * streams.
     *
     * \param mcsValue the MCS index
     * \param channelWidth the channel width in MHz
     * \param guardInterval the guard interval duration in nanoseconds
     * \param nss the number of spatial streams
     * \return the data bit rate in bps.
     */
    static uint64_t GetDataRate(uint8_t mcsValue,
                                uint16_t channelWidth,
                                uint16_t guardInterval,
                                uint8_t nss);
    /**
     * Calculate the rate in bps of the non-HT Reference Rate corresponding
     * to the supplied HE MCS index. This function calls CalculateNonHtReferenceRate
     * and is used as a callback for WifiMode operation.
     *
     * \param mcsValue the HE MCS index
     * \return the rate in bps of the non-HT Reference Rate.
     */
    static uint64_t GetNonHtReferenceRate(uint8_t mcsValue);
    /**
     * Check whether the combination in TXVECTOR is allowed.
     * This function is used as a callback for WifiMode operation.
     *
     * \param txVector the TXVECTOR
     * \returns true if this combination is allowed, false otherwise.
     */
    static bool IsAllowed(const WifiTxVector& txVector);

    /**
     * Create and return the HE MCS corresponding to
     * the provided index.
     * This method binds all the callbacks used by WifiMode.
     *
     * \param index the index of the MCS
     * \return an HE MCS
     */
    static WifiMode CreateHeMcs(uint8_t index);

    /**
     * \param bandWidth the width (MHz) of the band used for the OFDMA transmission. Must be
     *                  a multiple of 20 MHz
     * \param guardBandwidth width of the guard band (MHz)
     * \param subcarrierSpacing the subcarrier spacing (MHz)
     * \param subcarrierRange the subcarrier range of the HE RU
     * \param bandIndex the index (starting at 0) of the band within the operating channel
     * \return the converted subcarriers
     *
     * This is a helper function to convert HE RU subcarriers, which are relative to the center
     * frequency subcarrier, to the indexes used by the Spectrum model.
     */
    static WifiSpectrumBandIndices ConvertHeRuSubcarriers(uint16_t bandWidth,
                                                          uint16_t guardBandwidth,
                                                          uint32_t subcarrierSpacing,
                                                          HeRu::SubcarrierRange subcarrierRange,
                                                          uint8_t bandIndex = 0);

  protected:
    PhyFieldRxStatus ProcessSig(Ptr<Event> event,
                                PhyFieldRxStatus status,
                                WifiPpduField field) override;
    Ptr<Event> DoGetEvent(Ptr<const WifiPpdu> ppdu, RxPowerWattPerChannelBand& rxPowersW) override;
    bool IsConfigSupported(Ptr<const WifiPpdu> ppdu) const override;
    Time DoStartReceivePayload(Ptr<Event> event) override;
    std::pair<uint16_t, WifiSpectrumBandInfo> GetChannelWidthAndBand(const WifiTxVector& txVector,
                                                                     uint16_t staId) const override;
    void RxPayloadSucceeded(Ptr<const WifiPsdu> psdu,
                            RxSignalInfo rxSignalInfo,
                            const WifiTxVector& txVector,
                            uint16_t staId,
                            const std::vector<bool>& statusPerMpdu) override;
    void RxPayloadFailed(Ptr<const WifiPsdu> psdu,
                         double snr,
                         const WifiTxVector& txVector) override;
    void DoEndReceivePayload(Ptr<const WifiPpdu> ppdu) override;
    void DoResetReceive(Ptr<Event> event) override;
    void DoAbortCurrentReception(WifiPhyRxfailureReason reason) override;
    uint64_t ObtainNextUid(const WifiTxVector& txVector) override;
    Time GetMaxDelayPpduSameUid(const WifiTxVector& txVector) override;
    Ptr<SpectrumValue> GetTxPowerSpectralDensity(double txPowerW,
                                                 Ptr<const WifiPpdu> ppdu) const override;
    uint32_t GetMaxPsduSize() const override;
    WifiConstPsduMap GetWifiConstPsduMap(Ptr<const WifiPsdu> psdu,
                                         const WifiTxVector& txVector) const override;
    void HandleRxPpduWithSameContent(Ptr<Event> event,
                                     Ptr<const WifiPpdu> ppdu,
                                     RxPowerWattPerChannelBand& rxPower) override;

    /**
     * Process SIG-A, perform amendment-specific actions, and
     * provide an updated status of the reception.
     *
     * \param event the event holding incoming PPDU's information
     * \param status the status of the reception of the correctly received SIG-A after the
     * configuration support check
     * \return the updated status of the reception of the SIG-A
     */
    virtual PhyFieldRxStatus ProcessSigA(Ptr<Event> event, PhyFieldRxStatus status);

    /**
     * Process SIG-B, perform amendment-specific actions, and
     * provide an updated status of the reception.
     *
     * \param event the event holding incoming PPDU's information
     * \param status the status of the reception of the correctly received SIG-A after the
     * configuration support check
     * \return the updated status of the reception of the SIG-B
     */
    virtual PhyFieldRxStatus ProcessSigB(Ptr<Event> event, PhyFieldRxStatus status);

    /**
     * \param txVector the transmission parameters
     * \return the number of bits of the HE-SIG-B
     */
    virtual uint32_t GetSigBSize(const WifiTxVector& txVector) const;

    /**
     * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived) of an MU
     * transmission. This function is called upon the RX event corresponding to the HE portion of
     * the MU PPDU.
     *
     * \param event the event holding incoming HE portion of the PPDU's information
     */
    void StartReceiveMuPayload(Ptr<Event> event);

    /**
     * Return the rate (in bps) of the non-HT Reference Rate
     * which corresponds to the supplied code rate and
     * constellation size.
     *
     * \param codeRate the convolutional coding rate
     * \param constellationSize the size of modulation constellation
     * \returns the rate in bps.
     *
     * To convert an HE MCS to its corresponding non-HT Reference Rate
     * use the modulation and coding rate of the HT MCS
     * and lookup in Table 10-10 of IEEE P802.11ax/D6.0.
     */
    static uint64_t CalculateNonHtReferenceRate(WifiCodeRate codeRate, uint16_t constellationSize);

    /**
     * \param channelWidth the channel width in MHz
     * \return the number of usable subcarriers for data
     */
    static uint16_t GetUsableSubcarriers(uint16_t channelWidth);

    /**
     * \param guardInterval the guard interval duration
     * \return the symbol duration
     */
    static Time GetSymbolDuration(Time guardInterval);

    uint64_t m_previouslyTxPpduUid; //!< UID of the previously sent PPDU, used by AP to recognize
                                    //!< response HE TB PPDUs
    uint64_t m_currentMuPpduUid;    //!< UID of the HE MU or HE TB PPDU being received

    std::map<uint16_t /* STA-ID */, EventId>
        m_beginMuPayloadRxEvents; //!< the beginning of the MU payload reception events (indexed by
                                  //!< STA-ID)

    EndOfHeSigACallback m_endOfHeSigACallback;      //!< end of HE-SIG-A callback
    std::optional<WifiTxVector> m_trigVector;       //!< the TRIGVECTOR
    std::optional<Time> m_trigVectorExpirationTime; //!< expiration time of the TRIGVECTOR
    std::optional<WifiTxVector> m_currentTxVector;  //!< If the STA is an AP STA, this holds the
                                                    //!< TXVECTOR of the PPDU that has been sent

  private:
    void BuildModeList() override;
    uint8_t GetNumberBccEncoders(const WifiTxVector& txVector) const override;
    Time GetSymbolDuration(const WifiTxVector& txVector) const override;

    /**
     * This is a helper function to create the TX PSD of the non-HE and HE portions.
     *
     * \param txPowerW power in W to spread across the bands
     * \param ppdu the PPDU that will be transmitted
     * \param flag flag indicating whether the PSD is for non-HE portion or HE portion
     * \return Pointer to SpectrumValue
     */
    Ptr<SpectrumValue> GetTxPowerSpectralDensity(double txPowerW,
                                                 Ptr<const WifiPpdu> ppdu,
                                                 HePpdu::TxPsdFlag flag) const;

    /**
     * Start the transmission of the HE portion of the MU PPDU.
     *
     * \param ppdu the PPDU
     * \param txPowerDbm the total TX power in dBm
     * \param txPowerSpectrum the TX PSD
     * \param hePortionDuration the duration of the HE portion
     */
    void StartTxHePortion(Ptr<const WifiPpdu> ppdu,
                          double txPowerDbm,
                          Ptr<SpectrumValue> txPowerSpectrum,
                          Time hePortionDuration);

    /**
     * Notify PHY state helper to switch to CCA busy state,
     *
     * \param duration the duration of the CCA state
     * \param channelType the channel type for which the CCA busy state is reported.
     * \param per20MHzDurations the per-20 MHz CCA durations vector
     */
    void NotifyCcaBusy(Time duration,
                       WifiChannelListType channelType,
                       const std::vector<Time>& per20MHzDurations);

    /**
     * Compute the per-20 MHz CCA durations vector that indicates
     * for how long each 20 MHz subchannel (corresponding to the
     * index of the element in the vector) is busy and where a zero duration
     * indicates that the subchannel is idle. The vector is non-empty if the
     * operational channel width is larger than 20 MHz.
     *
     * \param ppdu the incoming PPDU or nullptr for any signal
     * \return the per-20 MHz CCA durations vector
     */
    std::vector<Time> GetPer20MHzDurations(const Ptr<const WifiPpdu> ppdu);

    /**
     * Given a PPDU duration value, the TXVECTOR used to transmit the PPDU and
     * the PHY band, compute a valid PPDU duration considering the number and
     * duration of symbols, the preamble duration and the guard interval.
     *
     * \param ppduDuration the given PPDU duration
     * \param txVector the given TXVECTOR
     * \param band the PHY band
     * \return a valid PPDU duration
     */
    static Time GetValidPpduDuration(Time ppduDuration,
                                     const WifiTxVector& txVector,
                                     WifiPhyBand band);

    static const PpduFormats m_hePpduFormats; //!< HE PPDU formats

    std::size_t m_rxHeTbPpdus;                 //!< Number of successfully received HE TB PPDUS
    Ptr<ObssPdAlgorithm> m_obssPdAlgorithm;    //!< OBSS-PD algorithm
    std::vector<Time> m_lastPer20MHzDurations; //!< Hold the last per-20 MHz CCA durations vector
};                                             // class HePhy

} // namespace ns3

#endif /* HE_PHY_H */
