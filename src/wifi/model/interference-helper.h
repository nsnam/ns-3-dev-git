/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef INTERFERENCE_HELPER_H
#define INTERFERENCE_HELPER_H

#include "phy-entity.h"

#include "ns3/object.h"

namespace ns3
{

class WifiPpdu;
class WifiPsdu;
class ErrorRateModel;

/**
 * @ingroup wifi
 * @brief handles interference calculations
 * @brief signal event for a PPDU.
 */
class Event : public SimpleRefCount<Event>
{
  public:
    /**
     * Create an Event with the given parameters. Note that <i>rxPower</i> will
     * be moved into this object.
     *
     * @param ppdu the PPDU
     * @param duration duration of the PPDU
     * @param rxPower the received power per band (W)
     */
    Event(Ptr<const WifiPpdu> ppdu, Time duration, RxPowerWattPerChannelBand&& rxPower);

    /**
     * Return the PPDU.
     *
     * @return the PPDU
     */
    Ptr<const WifiPpdu> GetPpdu() const;
    /**
     * Return the start time of the signal.
     *
     * @return the start time of the signal
     */
    Time GetStartTime() const;
    /**
     * Return the end time of the signal.
     *
     * @return the end time of the signal
     */
    Time GetEndTime() const;
    /**
     * Return the duration of the signal.
     *
     * @return the duration of the signal
     */
    Time GetDuration() const;
    /**
     * Return the total received power.
     *
     * @return the total received power
     */
    Watt_u GetRxPower() const;
    /**
     * Return the received power for a given band.
     *
     * @param band the band for which the power should be returned
     * @return the received power for a given band
     */
    Watt_u GetRxPower(const WifiSpectrumBandInfo& band) const;
    /**
     * Return the received power (W) for all bands.
     *
     * @return the received power (W) for all bands.
     */
    const RxPowerWattPerChannelBand& GetRxPowerPerBand() const;
    /**
     * Update the received power (W) for all bands, i.e. add up the received power
     * to the current received power, for each band.
     *
     * @param rxPower the received power (W) for all bands.
     */
    void UpdateRxPowerW(const RxPowerWattPerChannelBand& rxPower);
    /**
     * Update the PPDU that initially generated the event.
     * This is needed to have the PPDU holding the correct TXVECTOR
     * upon reception of multiple signals carrying the same content
     * but over different channel width (typically non-HT duplicates).
     *
     * @param ppdu the new PPDU to use for this event.
     */
    void UpdatePpdu(Ptr<const WifiPpdu> ppdu);

  private:
    Ptr<const WifiPpdu> m_ppdu;           //!< PPDU
    Time m_startTime;                     //!< start time
    Time m_endTime;                       //!< end time
    RxPowerWattPerChannelBand m_rxPowerW; //!< received power in watts per band
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param event the event
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const Event& event);

/**
 * @ingroup wifi
 * @brief handles interference calculations
 */
class InterferenceHelper : public Object
{
  public:
    InterferenceHelper();
    ~InterferenceHelper() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Add a frequency band.
     *
     * @param band the band to be added
     */
    void AddBand(const WifiSpectrumBandInfo& band);

    /**
     * Remove a frequency band.
     *
     * @param band the band to be removed
     */
    void RemoveBand(const WifiSpectrumBandInfo& band);

    /**
     * Check whether bands are already tracked by this interference helper.
     *
     * @return true if bands are tracked by this interference helper, false otherwise
     */
    bool HasBands() const;

    /**
     * Update the frequency bands that belongs to a given frequency range when the spectrum model is
     * changed.
     *
     * @param bands the bands to be added in the new spectrum model
     * @param freqRange the frequency range the bands belong to
     */
    void UpdateBands(const std::vector<WifiSpectrumBandInfo>& bands,
                     const FrequencyRange& freqRange);

    /**
     * Set the noise figure.
     *
     * @param value noise figure in linear scale
     */
    void SetNoiseFigure(double value);
    /**
     * Set the error rate model for this interference helper.
     *
     * @param rate Error rate model
     */
    void SetErrorRateModel(const Ptr<ErrorRateModel> rate);

    /**
     * Return the error rate model.
     *
     * @return Error rate model
     */
    Ptr<ErrorRateModel> GetErrorRateModel() const;
    /**
     * Set the number of RX antennas in the receiver corresponding to this
     * interference helper.
     *
     * @param rx the number of RX antennas
     */
    void SetNumberOfReceiveAntennas(uint8_t rx);

    /**
     * @param energy the minimum energy requested
     * @param band identify the requested band
     *
     * @returns the expected amount of time the observed
     *          energy on the medium for a given band will
     *          be higher than the requested threshold.
     */
    Time GetEnergyDuration(Watt_u energy, const WifiSpectrumBandInfo& band);

    /**
     * Add the PPDU-related signal to interference helper.
     *
     * @param ppdu the PPDU
     * @param duration the PPDU duration
     * @param rxPower received power per band (W)
     * @param freqRange the frequency range in which the received signal is detected
     * @param isStartHePortionRxing flag whether the event corresponds to the start of the HE
     * portion reception (only used for MU)
     *
     * @return Event
     */
    Ptr<Event> Add(Ptr<const WifiPpdu> ppdu,
                   Time duration,
                   RxPowerWattPerChannelBand& rxPower,
                   const FrequencyRange& freqRange,
                   bool isStartHePortionRxing = false);

    /**
     * Add a non-Wifi signal to interference helper.
     * @param duration the duration of the signal
     * @param rxPower received power per band (W)
     * @param freqRange the frequency range in which the received signal is detected
     */
    void AddForeignSignal(Time duration,
                          RxPowerWattPerChannelBand& rxPower,
                          const FrequencyRange& freqRange);
    /**
     * Calculate the SNIR at the start of the payload and accumulate
     * all SNIR changes in the SNIR vector for each MPDU of an A-MPDU.
     * This workaround is required in order to provide one PER per MPDU, for
     * reception success/failure evaluation, while hiding aggregation details from
     * this class.
     *
     * @param event the event corresponding to the first time the corresponding PPDU arrives
     * @param channelWidth the channel width used to transmit the PSDU
     * @param band identify the band used by the PSDU
     * @param staId the station ID of the PSDU (only used for MU)
     * @param relativeMpduStartStop the time window (pair of start and end times) of PHY payload to
     * focus on
     *
     * @return struct of SNR and PER (with PER being evaluated over the provided time window)
     */
    PhyEntity::SnrPer CalculatePayloadSnrPer(Ptr<Event> event,
                                             MHz_u channelWidth,
                                             const WifiSpectrumBandInfo& band,
                                             uint16_t staId,
                                             std::pair<Time, Time> relativeMpduStartStop) const;
    /**
     * Calculate the SNIR for the event (starting from now until the event end).
     *
     * @param event the event corresponding to the first time the corresponding PPDU arrives
     * @param channelWidth the channel width
     * @param nss the number of spatial streams
     * @param band identify the band used by the PSDU
     *
     * @return the SNR for the PPDU in linear scale
     */
    double CalculateSnr(Ptr<Event> event,
                        MHz_u channelWidth,
                        uint8_t nss,
                        const WifiSpectrumBandInfo& band) const;
    /**
     * Calculate the SNIR at the start of the PHY header and accumulate
     * all SNIR changes in the SNIR vector.
     *
     * @param event the event corresponding to the first time the corresponding PPDU arrives
     * @param channelWidth the channel width for header measurement
     * @param band identify the band used by the PSDU
     * @param header the PHY header to consider
     *
     * @return struct of SNR and PER
     */
    PhyEntity::SnrPer CalculatePhyHeaderSnrPer(Ptr<Event> event,
                                               MHz_u channelWidth,
                                               const WifiSpectrumBandInfo& band,
                                               WifiPpduField header) const;

    /**
     * Notify that RX has started.
     * @param freqRange the frequency range in which the received signal event is detected
     */
    void NotifyRxStart(const FrequencyRange& freqRange);
    /**
     * Notify that RX has ended.
     *
     * @param endTime the end time of the signal
     * @param freqRange the frequency range in which the received signal event was detected
     */
    void NotifyRxEnd(Time endTime, const FrequencyRange& freqRange);

    /**
     * Update event to scale its received power (W) per band.
     *
     * @param event the event to be updated
     * @param rxPower the received power (W) per band to be added to the current event
     */
    void UpdateEvent(Ptr<Event> event, const RxPowerWattPerChannelBand& rxPower);

  protected:
    void DoDispose() override;

    /**
     * Calculate SNR (linear ratio) from the given signal power and noise+interference power.
     *
     * @param signal signal power
     * @param noiseInterference noise and interference power
     * @param channelWidth signal width
     * @param nss the number of spatial streams
     *
     * @return SNR in linear scale
     */
    double CalculateSnr(Watt_u signal,
                        Watt_u noiseInterference,
                        MHz_u channelWidth,
                        uint8_t nss) const;
    /**
     * Calculate the success rate of the chunk given the SINR, duration, and TXVECTOR.
     * The duration and TXVECTOR are used to calculate how many bits are present in the chunk.
     *
     * @param snir the SINR
     * @param duration the duration of the chunk
     * @param mode the WifiMode
     * @param txVector the TXVECTOR
     * @param field the PPDU field to which the chunk belongs to
     *
     * @return the success rate
     */
    double CalculateChunkSuccessRate(double snir,
                                     Time duration,
                                     WifiMode mode,
                                     const WifiTxVector& txVector,
                                     WifiPpduField field) const;
    /**
     * Calculate the success rate of the payload chunk given the SINR, duration, and TXVECTOR.
     * The duration and TXVECTOR are used to calculate how many bits are present in the payload
     * chunk.
     *
     * @param snir the SINR
     * @param duration the duration of the chunk
     * @param txVector the TXVECTOR
     * @param staId the station ID of the PSDU (only used for MU)
     *
     * @return the success rate
     */
    double CalculatePayloadChunkSuccessRate(double snir,
                                            Time duration,
                                            const WifiTxVector& txVector,
                                            uint16_t staId = SU_STA_ID) const;

  protected:
    std::map<FrequencyRange, bool>
        m_rxing; //!< flag whether it is in receiving state for a given FrequencyRange

    /**
     * Noise and Interference (thus Ni) event.
     */
    class NiChange
    {
      public:
        /**
         * Create a NiChange at the given time and the amount of NI change.
         *
         * @param power the power
         * @param event causes this NI change
         */
        NiChange(Watt_u power, Ptr<Event> event);
        ~NiChange();
        /**
         * Return the power
         *
         * @return the power
         */
        Watt_u GetPower() const;
        /**
         * Add a given amount of power.
         *
         * @param power the power to be added to the existing value
         */
        void AddPower(Watt_u power);
        /**
         * Return the event causes the corresponding NI change
         *
         * @return the event
         */
        Ptr<Event> GetEvent() const;

      private:
        Watt_u m_power;     ///< power
        Ptr<Event> m_event; ///< event
    };

    /**
     * typedef for a multimap of NiChange
     */
    using NiChanges = std::multimap<Time, NiChange>;

    /**
     * Map of NiChanges per band
     */
    using NiChangesPerBand = std::map<WifiSpectrumBandInfo, NiChanges>;

    /**
     * Map of first power per band
     */
    using FirstPowerPerBand = std::map<WifiSpectrumBandInfo, Watt_u>;

    NiChangesPerBand m_niChanges; //!< NI Changes for each band

  private:
    /**
     * Check whether a given band is tracked by this interference helper.
     *
     * @param band the band to be checked
     * @return true if the band is already tracked by this interference helper, false otherwise
     */
    bool HasBand(const WifiSpectrumBandInfo& band) const;

    /**
     * Check whether a given band belongs to a given frequency range.
     *
     * @param band the band to be checked
     * @param freqRange the frequency range to check whether the band belong to
     * @return true if the band belongs to the frequency range, false otherwise
     */
    bool IsBandInFrequencyRange(const WifiSpectrumBandInfo& band,
                                const FrequencyRange& freqRange) const;

    /**
     * Append the given Event.
     *
     * @param event the event to be appended
     * @param freqRange the frequency range in which the received signal event is detected
     * @param isStartHePortionRxing flag whether event corresponds to the start of the HE portion
     * reception (only used for MU)
     */
    void AppendEvent(Ptr<Event> event, const FrequencyRange& freqRange, bool isStartHePortionRxing);

    /**
     * Calculate noise and interference power.
     *
     * @param event the event
     * @param nis the NiChanges
     * @param band the band
     *
     * @return noise and interference power
     */
    Watt_u CalculateNoiseInterferenceW(Ptr<Event> event,
                                       NiChangesPerBand& nis,
                                       const WifiSpectrumBandInfo& band) const;

    /**
     * Calculate power of all other events preceding a given event that belong to the same MU-MIMO
     * transmission.
     *
     * @param event the event
     * @param band the band
     *
     * @return the power of all other events preceding the event that belong to the same MU-MIMO
     * transmission
     */
    Watt_u CalculateMuMimoPowerW(Ptr<const Event> event, const WifiSpectrumBandInfo& band) const;

    /**
     * Calculate the error rate of the given PHY payload only in the provided time
     * window (thus enabling per MPDU PER information). The PHY payload can be divided into
     * multiple chunks (e.g. due to interference from other transmissions).
     *
     * @param event the event
     * @param channelWidth the channel width used to transmit the PSDU
     * @param nis the NiChanges
     * @param band identify the band used by the PSDU
     * @param staId the station ID of the PSDU (only used for MU)
     * @param window time window (pair of start and end times) of PHY payload to focus on
     *
     * @return the error rate of the payload
     */
    double CalculatePayloadPer(Ptr<const Event> event,
                               MHz_u channelWidth,
                               NiChangesPerBand* nis,
                               const WifiSpectrumBandInfo& band,
                               uint16_t staId,
                               std::pair<Time, Time> window) const;
    /**
     * Calculate the error rate of the PHY header. The PHY header
     * can be divided into multiple chunks (e.g. due to interference from other transmissions).
     *
     * @param event the event
     * @param nis the NiChanges
     * @param channelWidth the channel width for header measurement
     * @param band the band
     * @param header the PHY header to consider
     *
     * @return the error rate of the HT PHY header
     */
    double CalculatePhyHeaderPer(Ptr<const Event> event,
                                 NiChangesPerBand* nis,
                                 MHz_u channelWidth,
                                 const WifiSpectrumBandInfo& band,
                                 WifiPpduField header) const;
    /**
     * Calculate the success rate of the PHY header sections for the provided event.
     *
     * @param event the event
     * @param nis the NiChanges
     * @param channelWidth the channel width for header measurement
     * @param band the band
     * @param phyHeaderSections the map of PHY header sections (\see PhyEntity::PhyHeaderSections)
     *
     * @return the success rate of the PHY header sections
     */
    double CalculatePhyHeaderSectionPsr(Ptr<const Event> event,
                                        NiChangesPerBand* nis,
                                        MHz_u channelWidth,
                                        const WifiSpectrumBandInfo& band,
                                        PhyEntity::PhyHeaderSections phyHeaderSections) const;

    double m_noiseFigure;                 //!< noise figure (linear)
    Ptr<ErrorRateModel> m_errorRateModel; //!< error rate model
    uint8_t m_numRxAntennas;         //!< the number of RX antennas in the corresponding receiver
    FirstPowerPerBand m_firstPowers; //!< first power of each band

    /**
     * Returns an iterator to the first NiChange that is later than moment
     *
     * @param moment time to check from
     * @param niIt iterator of the band to check
     * @returns an iterator to the list of NiChanges
     */
    NiChanges::iterator GetNextPosition(Time moment, NiChangesPerBand::iterator niIt);
    /**
     * Returns an iterator to the last NiChange that is before than moment
     *
     * @param moment time to check from
     * @param niIt iterator of the band to check
     * @returns an iterator to the list of NiChanges
     */
    NiChanges::iterator GetPreviousPosition(Time moment, NiChangesPerBand::iterator niIt);

    /**
     * Add NiChange to the list at the appropriate position and
     * return the iterator of the new event.
     *
     * @param moment time to check from
     * @param change the NiChange to add
     * @param niIt iterator of the band to check
     * @returns the iterator of the new event
     */
    NiChanges::iterator AddNiChangeEvent(Time moment,
                                         NiChange change,
                                         NiChangesPerBand::iterator niIt);

    /**
     * Return whether another event is a MU-MIMO event that belongs to the same transmission and to
     * the same RU.
     *
     * @param currentEvent the current event that is being inspected
     * @param otherEvent the other event to compare against
     *
     * @return whether both events belong to the same transmission and to the same RU
     */
    bool IsSameMuMimoTransmission(Ptr<const Event> currentEvent, Ptr<const Event> otherEvent) const;
};

} // namespace ns3

#endif /* INTERFERENCE_HELPER_H */
