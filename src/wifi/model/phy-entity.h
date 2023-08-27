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
 * spectrum-wifi-phy) Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from
 * wifi-phy)
 */

#ifndef PHY_ENTITY_H
#define PHY_ENTITY_H

#include "wifi-mpdu-type.h"
#include "wifi-phy-band.h"
#include "wifi-ppdu.h"
#include "wifi-tx-vector.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/simple-ref-count.h"

#include <list>
#include <map>
#include <optional>
#include <tuple>
#include <utility>

/**
 * \file
 * \ingroup wifi
 * Declaration of:
 * - ns3::PhyEntity class
 * - ns3::SignalNoiseDbm, ns3::MpduInfo, and ns3::RxSignalInfo structs
 * - ns3::RxPowerWattPerChannelBand typedef
 */

namespace ns3
{

/// SignalNoiseDbm structure
struct SignalNoiseDbm
{
    double signal; ///< signal strength in dBm
    double noise;  ///< noise power in dBm
};

/// MpduInfo structure
struct MpduInfo
{
    MpduType type;          ///< type of MPDU
    uint32_t mpduRefNumber; ///< MPDU ref number
};

/// RxSignalInfo structure containing info on the received signal
struct RxSignalInfo
{
    double snr;  ///< SNR in linear scale
    double rssi; ///< RSSI in dBm
};

/**
 * A map of the received power (Watts) for each band
 */
using RxPowerWattPerChannelBand = std::map<WifiSpectrumBandInfo, double>;

class WifiPsdu;
class WifiPhy;
class InterferenceHelper;
class Event;
class WifiPhyStateHelper;
class WifiPsdu;
class WifiPpdu;

/**
 * \brief Abstract class for PHY entities
 * \ingroup wifi
 *
 * This class enables to have a unique set of APIs
 * to be used by each PHY entity, corresponding to
 * the different amendments of the IEEE 802.11 standard.
 */
class PhyEntity : public SimpleRefCount<PhyEntity>
{
  public:
    /**
     * Action to perform in case of RX failure.
     */
    enum PhyRxFailureAction
    {
        DROP = 0, //!< drop PPDU and set CCA_BUSY
        ABORT,    //!< abort reception of PPDU
        IGNORE    //!< ignore the reception
    };

    /**
     * Status of the reception of the PPDU field.
     */
    struct PhyFieldRxStatus
    {
        bool isSuccess{true};                   //!< outcome (\c true if success) of the reception
        WifiPhyRxfailureReason reason{UNKNOWN}; //!< failure reason
        PhyRxFailureAction actionIfFailure{
            DROP}; //!< action to perform in case of failure \see PhyRxFailureAction

        /**
         * Constructor setting outcome of reception.
         *
         * \param s \c true if success
         */
        PhyFieldRxStatus(bool s)
            : isSuccess(s)
        {
        }

        /**
         * Constructor.
         *
         * \param s \c true if success
         * \param r reason of failure
         * \param a action to perform in case of failure
         */
        PhyFieldRxStatus(bool s, WifiPhyRxfailureReason r, PhyRxFailureAction a)
            : isSuccess(s),
              reason(r),
              actionIfFailure(a)
        {
        }
    };

    /**
     * A struct for both SNR and PER
     */
    struct SnrPer
    {
        double snr{0.0}; ///< SNR in linear scale
        double per{1.0}; ///< PER

        /**
         * Default constructor.
         */
        SnrPer()
        {
        }

        /**
         * Constructor for SnrPer.
         *
         * \param s the SNR in linear scale
         * \param p the PER
         */
        SnrPer(double s, double p)
            : snr(s),
              per(p)
        {
        }
    };

    /**
     * Destructor for PHY entity
     */
    virtual ~PhyEntity();

    /**
     * Set the WifiPhy owning this PHY entity.
     *
     * \param wifiPhy the WifiPhy owning this PHY entity
     */
    void SetOwner(Ptr<WifiPhy> wifiPhy);

    /**
     * Check if the WifiMode is supported.
     *
     * \param mode the WifiMode to check
     * \return true if the WifiMode is supported,
     *         false otherwise
     */
    virtual bool IsModeSupported(WifiMode mode) const;
    /**
     * \return the number of WifiModes supported by this entity
     */
    virtual uint8_t GetNumModes() const;

    /**
     * Get the WifiMode corresponding to the given MCS index.
     *
     * \param index the index of the MCS
     * \return the WifiMode corresponding to the MCS index
     *
     * This method should be used only for HtPhy and child classes.
     */
    virtual WifiMode GetMcs(uint8_t index) const;
    /**
     * Check if the WifiMode corresponding to the given MCS index is supported.
     *
     * \param index the index of the MCS
     * \return true if the WifiMode corresponding to the MCS index is supported,
     *         false otherwise
     *
     * Will return false for non-MCS modes.
     */
    virtual bool IsMcsSupported(uint8_t index) const;
    /**
     * Check if the WifiModes handled by this PHY are MCSs.
     *
     * \return true if the handled WifiModes are MCSs,
     *         false if they are non-MCS modes
     */
    virtual bool HandlesMcsModes() const;

    /**
     * Get the WifiMode for the SIG field specified by the PPDU field.
     *
     * \param field the PPDU field
     * \param txVector the transmission parameters
     *
     * \return the WifiMode used for the SIG field
     */
    virtual WifiMode GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const;

    /**
     * \brief Return a const iterator to the first WifiMode
     *
     * \return a const iterator to the first WifiMode.
     */
    std::list<WifiMode>::const_iterator begin() const;
    /**
     * \brief Return a const iterator to past-the-last WifiMode
     *
     * \return a const iterator to past-the-last WifiMode.
     */
    std::list<WifiMode>::const_iterator end() const;

    /**
     * Return the field following the provided one.
     *
     * \param currentField the considered PPDU field
     * \param preamble the preamble indicating the PPDU format
     * \return the PPDU field following the reference one
     */
    WifiPpduField GetNextField(WifiPpduField currentField, WifiPreamble preamble) const;

    /**
     * Get the duration of the PPDU field (or group of fields)
     * used by this entity for the given transmission parameters.
     *
     * \param field the PPDU field (or group of fields)
     * \param txVector the transmission parameters
     *
     * \return the duration of the PPDU field
     */
    virtual Time GetDuration(WifiPpduField field, const WifiTxVector& txVector) const;
    /**
     * \param txVector the transmission parameters
     *
     * \return the total duration of the PHY preamble and PHY header.
     */
    Time CalculatePhyPreambleAndHeaderDuration(const WifiTxVector& txVector) const;

    /**
     * \param size the number of bytes in the packet to send
     * \param txVector the TXVECTOR used for the transmission of this packet
     * \param band the frequency band
     * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
     * \param incFlag this flag is used to indicate that the variables need to be update or not
     * This function is called a couple of times for the same packet so variables should not be
     * increased each time.
     * \param totalAmpduSize the total size of the previously transmitted MPDUs for the concerned
     * A-MPDU. If incFlag is set, this parameter will be updated.
     * \param totalAmpduNumSymbols the number of symbols previously transmitted for the MPDUs in the
     * concerned A-MPDU, used for the computation of the number of symbols needed for the last MPDU.
     * If incFlag is set, this parameter will be updated.
     * \param staId the STA-ID of the PSDU (only used for MU PPDUs)
     *
     * \return the duration of the PSDU
     */
    virtual Time GetPayloadDuration(uint32_t size,
                                    const WifiTxVector& txVector,
                                    WifiPhyBand band,
                                    MpduType mpdutype,
                                    bool incFlag,
                                    uint32_t& totalAmpduSize,
                                    double& totalAmpduNumSymbols,
                                    uint16_t staId) const = 0;

    /**
     * Get a WifiConstPsduMap from a PSDU and the TXVECTOR to use to send the PSDU.
     * The STA-ID value is properly determined based on whether the given PSDU has
     * to be transmitted as a DL or UL frame.
     *
     * \param psdu the given PSDU
     * \param txVector the TXVECTOR to use to send the PSDU
     * \return a WifiConstPsduMap built from the given PSDU and the given TXVECTOR
     */
    virtual WifiConstPsduMap GetWifiConstPsduMap(Ptr<const WifiPsdu> psdu,
                                                 const WifiTxVector& txVector) const;

    /**
     * Get the maximum PSDU size in bytes.
     *
     * \return the maximum PSDU size in bytes
     */
    virtual uint32_t GetMaxPsduSize() const = 0;

    /**
     * A pair containing information on the PHY header chunk, namely
     * the start and stop times of the chunk and the WifiMode used.
     */
    typedef std::pair<std::pair<Time /* start */, Time /* stop */>, WifiMode> PhyHeaderChunkInfo;
    /**
     * A map of PhyHeaderChunkInfo elements per PPDU field.
     * \see PhyHeaderChunkInfo
     */
    typedef std::map<WifiPpduField, PhyHeaderChunkInfo> PhyHeaderSections;
    /**
     * Return a map of PHY header chunk information per PPDU field.
     * This map will contain the PPDU fields that are actually present based
     * on the \p txVector information.
     *
     * \param txVector the transmission parameters
     * \param ppduStart the time at which the PPDU started
     * \return the list of preamble sections
     *
     * \see PhyHeaderSections
     */
    PhyHeaderSections GetPhyHeaderSections(const WifiTxVector& txVector, Time ppduStart) const;

    /**
     * Build amendment-specific PPDU.
     *
     * \param psdus the PHY payloads (PSDUs)
     * \param txVector the TXVECTOR that was used for the PPDU
     * \param ppduDuration the transmission duration of the PPDU
     *
     * \return the amendment-specific WifiPpdu
     */
    virtual Ptr<WifiPpdu> BuildPpdu(const WifiConstPsduMap& psdus,
                                    const WifiTxVector& txVector,
                                    Time ppduDuration);

    /**
     * Get the duration of the PPDU up to (but excluding) the given field.
     *
     * \param field the considered PPDU field
     * \param txVector the transmission parameters
     * \return the duration from the beginning of the PPDU up to the field
     */
    Time GetDurationUpToField(WifiPpduField field, const WifiTxVector& txVector) const;
    /**
     * Get the remaining duration of the PPDU after the end of the given field.
     *
     * \param field the considered PPDU field
     * \param ppdu the received PPDU
     * \return the remaining duration of the PPDU after the end of to the field
     */
    Time GetRemainingDurationAfterField(Ptr<const WifiPpdu> ppdu, WifiPpduField field) const;

    /**
     * Get the PSDU addressed to that PHY in a PPDU (useful for MU PPDU).
     *
     * \param ppdu the PPDU to extract the PSDU from
     * \return the PSDU addressed to that PHY
     */
    virtual Ptr<const WifiPsdu> GetAddressedPsduInPpdu(Ptr<const WifiPpdu> ppdu) const;

    /**
     * Start receiving the PHY preamble of a PPDU (i.e. the first bit of the preamble has arrived).
     *
     * This method triggers the start of the preamble detection period (\see
     * StartPreambleDetectionPeriod) if the PHY can process the PPDU.
     *
     * \param ppdu the arriving PPDU
     * \param rxPowersW the receive power in W per band
     * \param rxDuration the duration of the PPDU
     */
    virtual void StartReceivePreamble(Ptr<const WifiPpdu> ppdu,
                                      RxPowerWattPerChannelBand& rxPowersW,
                                      Time rxDuration);
    /**
     * Start receiving a given field.
     *
     * This method will call the DoStartReceiveField.
     * EndReceiveField is also scheduled after the duration of the field
     * (except for the special case of preambles \see DoStartReceivePreamble).
     * The PHY is kept in CCA busy during the reception of the field (except for
     * data field which should be in RX).
     *
     * \param field the starting PPDU field
     * \param event the event holding incoming PPDU's information
     */
    void StartReceiveField(WifiPpduField field, Ptr<Event> event);
    /**
     * End receiving a given field.
     *
     * This method will call the DoEndReceiveField  to obtain the outcome of the reception.
     * In case of success, reception of the next field is triggered.
     * In case of failure, the indications in the returned \see PhyFieldRxStatus
     * are performed.
     *
     * \param field the ending PPDU field
     * \param event the event holding incoming PPDU's information
     */
    void EndReceiveField(WifiPpduField field, Ptr<Event> event);

    /**
     * The last symbol of the PPDU has arrived.
     *
     * \param event the event holding incoming PPDU's information
     */
    void EndReceivePayload(Ptr<Event> event);

    /**
     * Reset PHY at the end of the PPDU under reception after it has failed the PHY header.
     *
     * \param event the event holding incoming PPDU's information
     */
    void ResetReceive(Ptr<Event> event);

    /**
     * Cancel and clear all running events.
     */
    virtual void CancelAllEvents();
    /**
     * \return \c true if there is no end preamble detection event running, \c false otherwise
     */
    bool NoEndPreambleDetectionEvents() const;
    /**
     * Cancel and eventually clear all end preamble detection events.
     *
     * \param clear whether to clear the end preamble detection events' list
     */
    void CancelRunningEndPreambleDetectionEvents(bool clear = false);

    /**
     * Return the STA ID that has been assigned to the station this PHY belongs to.
     * This is typically called for MU PPDUs, in order to pick the correct PSDU.
     *
     * \param ppdu the PPDU for which the STA ID is requested
     * \return the STA ID
     */
    virtual uint16_t GetStaId(const Ptr<const WifiPpdu> ppdu) const;

    /**
     * Determine whether the PHY shall issue a PHY-RXSTART.indication primitive in response to a
     * given PPDU.
     *
     * \param ppdu the PPDU
     * \return true if the PHY shall issue a PHY-RXSTART.indication primitive in response to a PPDU,
     * false otherwise
     */
    virtual bool CanStartRx(Ptr<const WifiPpdu> ppdu) const;

    /**
     * Check if PHY state should move to CCA busy state based on current
     * state of interference tracker.
     *
     * \param ppdu the incoming PPDU or nullptr for any signal
     */
    virtual void SwitchMaybeToCcaBusy(const Ptr<const WifiPpdu> ppdu);
    /**
     * Notify PHY state helper to switch to CCA busy state,
     *
     * \param ppdu the incoming PPDU or nullptr for any signal
     * \param duration the duration of the CCA state
     * \param channelType the channel type for which the CCA busy state is reported.
     */
    virtual void NotifyCcaBusy(const Ptr<const WifiPpdu> ppdu,
                               Time duration,
                               WifiChannelListType channelType);
    /**
     * This function is called by SpectrumWifiPhy to send
     * the PPDU while performing amendment-specific actions.
     * \see SpectrumWifiPhy::StartTx
     *
     * \param ppdu the PPDU to send
     */
    virtual void StartTx(Ptr<const WifiPpdu> ppdu);

    /**
     * This function prepares most of the WifiSpectrumSignalParameters
     * parameters and invokes SpectrumWifiPhy's Transmit method.
     * \see SpectrumWifiPhy::Transmit
     *
     * \param txDuration the duration of the transmission
     * \param ppdu the PPDU to send
     * \param txPowerDbm the total TX power in dBm
     * \param txPowerSpectrum the TX PSD
     * \param type the type of transmission (for logging)
     */
    void Transmit(Time txDuration,
                  Ptr<const WifiPpdu> ppdu,
                  double txPowerDbm,
                  Ptr<SpectrumValue> txPowerSpectrum,
                  const std::string& type);

    /**
     * \param psduMap the PSDU(s) to transmit indexed by STA-ID
     * \param txVector the TXVECTOR used for the transmission of the PPDU
     * \param band the frequency band being used
     *
     * \return the total amount of time this PHY will stay busy for the transmission of the PPDU
     */
    virtual Time CalculateTxDuration(WifiConstPsduMap psduMap,
                                     const WifiTxVector& txVector,
                                     WifiPhyBand band) const;
    /**
     * Return the CCA threshold in dBm for a given channel type.
     * If the channel type is not provided, the default CCA threshold is returned.
     *
     * \param ppdu the PPDU that is being received
     * \param channelType the channel type
     * \return the CCA threshold in dBm
     */
    virtual double GetCcaThreshold(const Ptr<const WifiPpdu> ppdu,
                                   WifiChannelListType channelType) const;

    /**
     * The WifiPpdu from the TX PHY is received by each RX PHY attached to the same channel.
     * By default and for performance reasons, all RX PHYs will work on the same WifiPpdu instance
     * from TX instead of a copy of it. Child classes can change that behavior and do a copy and/or
     * change the content of the parameters stored in WifiPpdu.
     *
     * \param ppdu the WifiPpdu transmitted by the TX PHY
     * \return the WifiPpdu to be used by the RX PHY
     */
    virtual Ptr<const WifiPpdu> GetRxPpduFromTxPpdu(Ptr<const WifiPpdu> ppdu);

    /**
     * Obtain the next UID for the PPDU to transmit.
     * Note that the global UID counter could be incremented.
     *
     * \param txVector the transmission parameters
     * \return the UID to use for the PPDU to transmit
     */
    virtual uint64_t ObtainNextUid(const WifiTxVector& txVector);

    /**
     * Obtain the maximum time between two PPDUs with the same UID to consider they are identical
     * and their power can be added construtively.
     *
     * \param txVector the TXVECTOR used for the transmission of the PPDUs
     * \return the maximum time between two PPDUs with the same UID to decode them
     */
    virtual Time GetMaxDelayPpduSameUid(const WifiTxVector& txVector);

  protected:
    /**
     * A map of PPDU field elements per preamble type.
     * This corresponds to the different PPDU formats introduced by each amendment.
     */
    typedef std::map<WifiPreamble, std::vector<WifiPpduField>> PpduFormats;

    /**
     * A pair to hold modulation information: code rate and constellation size.
     */
    typedef std::pair<WifiCodeRate, uint16_t> CodeRateConstellationSizePair;

    /**
     * A modulation lookup table using unique name of modulation as key.
     */
    typedef std::map<std::string, CodeRateConstellationSizePair> ModulationLookupTable;

    /**
     * Return the PPDU formats of the PHY.
     *
     * \return the PPDU formats of the PHY
     */
    virtual const PpduFormats& GetPpduFormats() const = 0;

    /**
     * Start receiving a given field, perform amendment-specific actions, and
     * signify if it is supported.
     *
     * \param field the starting PPDU field
     * \param event the event holding incoming PPDU's information
     * \return \c true if the field is supported, \c false otherwise
     */
    virtual bool DoStartReceiveField(WifiPpduField field, Ptr<Event> event);
    /**
     * End receiving a given field, perform amendment-specific actions, and
     * provide the status of the reception.
     *
     * \param field the ending PPDU field
     * \param event the event holding incoming PPDU's information
     * \return status of the reception of the PPDU field
     */
    virtual PhyFieldRxStatus DoEndReceiveField(WifiPpduField field, Ptr<Event> event);

    /**
     * Get the event corresponding to the incoming PPDU.
     *
     * We store all incoming preamble events, perform amendment-specific actions,
     * and a decision is made at the end of the preamble detection window.
     *
     * \param ppdu the incoming PPDU
     * \param rxPowersW the receive power in W per band
     * \return the event holding the incoming PPDU's information
     */
    virtual Ptr<Event> DoGetEvent(Ptr<const WifiPpdu> ppdu, RxPowerWattPerChannelBand& rxPowersW);
    /**
     * End receiving the preamble, perform amendment-specific actions, and
     * provide the status of the reception.
     *
     * \param event the event holding incoming PPDU's information
     * \return status of the reception of the preamble
     */
    virtual PhyFieldRxStatus DoEndReceivePreamble(Ptr<Event> event);
    /**
     * Start the preamble detection period.
     *
     * \param event the event holding incoming PPDU's information
     */
    void StartPreambleDetectionPeriod(Ptr<Event> event);
    /**
     * End the preamble detection period.
     *
     * The PHY will focus on the strongest PPDU and drop others.
     * In addition, in case of successful detection, the end of the
     * preamble reception is triggered (\see DoEndReceivePreamble).
     *
     * \param event the event holding incoming PPDU's information
     */
    void EndPreambleDetectionPeriod(Ptr<Event> event);

    /**
     * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived).
     *
     * \param event the event holding incoming PPDU's information
     */
    void StartReceivePayload(Ptr<Event> event);

    /**
     * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived)
     * and perform amendment-specific actions.
     *
     * \param event the event holding incoming PPDU's information
     * \return the payload duration
     */
    virtual Time DoStartReceivePayload(Ptr<Event> event);

    /**
     * Perform amendment-specific actions before resetting PHY at
     * the end of the PPDU under reception after it has failed the PHY header.
     *
     * \param event the event holding incoming PPDU's information
     */
    virtual void DoResetReceive(Ptr<Event> event);

    /**
     * Perform amendment-specific actions before aborting the
     * current reception.
     *
     * \param reason the reason the reception is aborted
     */
    virtual void DoAbortCurrentReception(WifiPhyRxfailureReason reason);

    /**
     * Checks if the signaled configuration (excluding bandwidth)
     * is supported by the PHY.
     *
     * \param ppdu the received PPDU
     * \return \c true if supported, \c false otherwise
     */
    virtual bool IsConfigSupported(Ptr<const WifiPpdu> ppdu) const;

    /**
     * Drop the PPDU and the corresponding preamble detection event, but keep CCA busy
     * state after the completion of the currently processed event.
     *
     * \param ppdu the incoming PPDU
     * \param reason the reason the PPDU is dropped
     * \param endRx the end of the incoming PPDU's reception
     */
    void DropPreambleEvent(Ptr<const WifiPpdu> ppdu, WifiPhyRxfailureReason reason, Time endRx);

    /**
     * Erase the event corresponding to the PPDU from the list of preamble events,
     * but consider it as noise after the completion of the current event.
     *
     * \param ppdu the incoming PPDU
     * \param rxDuration the duration of the PPDU
     */
    void ErasePreambleEvent(Ptr<const WifiPpdu> ppdu, Time rxDuration);

    /**
     * Get the reception status for the provided MPDU and notify.
     *
     * \param psdu the arriving MPDU formatted as a PSDU
     * \param event the event holding incoming PPDU's information
     * \param staId the station ID of the PSDU (only used for MU)
     * \param relativeMpduStart the relative start time of the MPDU within the A-MPDU.
     * 0 for normal MPDUs
     * \param mpduDuration the duration of the MPDU
     *
     * \return information on MPDU reception: status, signal power (dBm), and noise power (in dBm)
     */
    std::pair<bool, SignalNoiseDbm> GetReceptionStatus(Ptr<const WifiPsdu> psdu,
                                                       Ptr<Event> event,
                                                       uint16_t staId,
                                                       Time relativeMpduStart,
                                                       Time mpduDuration);
    /**
     * The last symbol of an MPDU in an A-MPDU has arrived.
     *
     * \param event the event holding incoming PPDU's information
     * \param psdu the arriving MPDU formatted as a PSDU containing a normal MPDU
     * \param mpduIndex the index of the MPDU within the A-MPDU
     * \param relativeStart the relative start time of the MPDU within the A-MPDU.
     * \param mpduDuration the duration of the MPDU
     */
    void EndOfMpdu(Ptr<Event> event,
                   Ptr<const WifiPsdu> psdu,
                   size_t mpduIndex,
                   Time relativeStart,
                   Time mpduDuration);

    /**
     * Schedule end of MPDUs events.
     *
     * \param event the event holding incoming PPDU's information
     */
    void ScheduleEndOfMpdus(Ptr<Event> event);

    /**
     * Perform amendment-specific actions when the payload is successfully received.
     *
     * \param psdu the successfully received PSDU
     * \param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * \param txVector TXVECTOR of the PSDU
     * \param staId the station ID of the PSDU (only used for MU)
     * \param statusPerMpdu reception status per MPDU
     */
    virtual void RxPayloadSucceeded(Ptr<const WifiPsdu> psdu,
                                    RxSignalInfo rxSignalInfo,
                                    const WifiTxVector& txVector,
                                    uint16_t staId,
                                    const std::vector<bool>& statusPerMpdu);
    /**
     * Perform amendment-specific actions when the payload is unsuccessfuly received.
     *
     * \param psdu the PSDU that we failed to received
     * \param snr the SNR of the received PSDU in linear scale
     * \param txVector TXVECTOR of the PSDU
     */
    virtual void RxPayloadFailed(Ptr<const WifiPsdu> psdu,
                                 double snr,
                                 const WifiTxVector& txVector);

    /**
     * Perform amendment-specific actions at the end of the reception of
     * the payload.
     *
     * \param ppdu the incoming PPDU
     */
    virtual void DoEndReceivePayload(Ptr<const WifiPpdu> ppdu);

    /**
     * Get the channel width and band to use (will be overloaded by child classes).
     *
     * \param txVector the transmission parameters
     * \param staId the station ID of the PSDU
     * \return a pair of channel width (MHz) and band
     */
    virtual std::pair<uint16_t, WifiSpectrumBandInfo> GetChannelWidthAndBand(
        const WifiTxVector& txVector,
        uint16_t staId) const;

    /**
     * Abort the current reception.
     *
     * \param reason the reason the reception is aborted
     */
    void AbortCurrentReception(WifiPhyRxfailureReason reason);

    /**
     * Obtain a random value from the WifiPhy's generator.
     * Wrapper used by child classes.
     *
     * \return a uniform random value
     */
    double GetRandomValue() const;
    /**
     * Obtain the SNR and PER of the PPDU field from the WifiPhy's InterferenceHelper class.
     * Wrapper used by child classes.
     *
     * \param field the PPDU field
     * \param event the event holding incoming PPDU's information
     * \return the SNR and PER
     */
    SnrPer GetPhyHeaderSnrPer(WifiPpduField field, Ptr<Event> event) const;
    /**
     * Obtain the received power (W) for a given band.
     * Wrapper used by child classes.
     *
     * \param event the event holding incoming PPDU's information
     * \return the received power (W) for the event over a given band
     */
    double GetRxPowerWForPpdu(Ptr<Event> event) const;
    /**
     * Get the pointer to the current event (stored in WifiPhy).
     * Wrapper used by child classes.
     *
     * \return the pointer to the current event
     */
    Ptr<const Event> GetCurrentEvent() const;
    /**
     * Get the map of current preamble events (stored in WifiPhy).
     * Wrapper used by child classes.
     *
     * \return the reference to the map of current preamble events
     */
    const std::map<std::pair<uint64_t, WifiPreamble>, Ptr<Event>>& GetCurrentPreambleEvents() const;
    /**
     * Add an entry to the map of current preamble events (stored in WifiPhy).
     * Wrapper used by child classes.
     *
     * \param event the event holding incoming PPDU's information
     */
    void AddPreambleEvent(Ptr<Event> event);

    /**
     * Create an event using WifiPhy's InterferenceHelper class.
     * Wrapper used by child classes.
     *
     * \param ppdu the PPDU
     * \param duration the PPDU duration
     * \param rxPower received power per band (W)
     * \param isStartHePortionRxing flag whether the event corresponds to the start of the OFDMA
     * payload reception (only used for UL-OFDMA) \return the created event
     */
    Ptr<Event> CreateInterferenceEvent(Ptr<const WifiPpdu> ppdu,
                                       Time duration,
                                       RxPowerWattPerChannelBand& rxPower,
                                       bool isStartHePortionRxing = false);
    /**
     * Handle reception of a PPDU that carries the same content of another PPDU.
     * This is typically called upon reception of preambles of HE MU PPDUs or reception
     * of non-HT duplicate control frames that carries the exact same content sent from different
     * STAs. If the delay between the PPDU and the first PPDU carrying the same content is small
     * enough, PPDU can be decoded and its power is added constructively, and the TXVECTOR is
     * updated accordingly. Otherwise, a new interference event is created and PPDU is dropped by
     * the PHY.
     *
     * \param event the event of the ongoing reception
     * \param ppdu the newly received PPDU (UL MU or non-HT duplicate)
     * \param rxPower the received power (W) per band of the newly received PPDU
     */
    virtual void HandleRxPpduWithSameContent(Ptr<Event> event,
                                             Ptr<const WifiPpdu> ppdu,
                                             RxPowerWattPerChannelBand& rxPower);

    /**
     * Notify WifiPhy's InterferenceHelper of the end of the reception,
     * clear maps and end of MPDU event, and eventually reset WifiPhy.
     *
     * \param reset whether to reset WifiPhy
     */
    void NotifyInterferenceRxEndAndClear(bool reset);

    /**
     * \param txPowerW power in W to spread across the bands
     * \param ppdu the PPDU that will be transmitted
     * \return Pointer to SpectrumValue
     *
     * This is a helper function to create the right TX PSD corresponding
     * to the amendment of this PHY.
     */
    virtual Ptr<SpectrumValue> GetTxPowerSpectralDensity(double txPowerW,
                                                         Ptr<const WifiPpdu> ppdu) const = 0;

    /**
     * Get the center frequency of the channel corresponding the current TxVector rather than
     * that of the supported channel width.
     * Consider that this "primary channel" is on the lower part for the time being.
     *
     * \param txVector the TXVECTOR that has the channel width that is to be used
     * \return the center frequency in MHz corresponding to the channel width to be used
     */
    uint16_t GetCenterFrequencyForChannelWidth(const WifiTxVector& txVector) const;

    /**
     * Fire the trace indicating that the PHY is starting to receive the payload of a PPDU.
     *
     * \param txVector the TXVECTOR used to transmit the PPDU
     * \param payloadDuration the TX duration of the PPDU payload
     */
    void NotifyPayloadBegin(const WifiTxVector& txVector, const Time& payloadDuration);

    /**
     * If the operating channel width is a multiple of 20 MHz, return the info
     * corresponding to the primary channel of the given
     * bandwidth (which must be a multiple of 20 MHz and not exceed the operating
     * channel width). Otherwise, this call is equivalent to GetBand with
     * <i>bandIndex</i> equal to zero.
     *
     * \param bandWidth the width of the band to be returned (MHz)
     *
     * \return the info corresponding to the band
     */
    WifiSpectrumBandInfo GetPrimaryBand(uint16_t bandWidth) const;
    /**
     * If the channel bonding is used, return the info corresponding to
     * the secondary channel of the given bandwidth (which must be a multiple of 20 MHz
     * and not exceed the operating channel width).
     *
     * \param bandWidth the width of the band to be returned (MHz)
     *
     * \return the info corresponding to the band
     */
    WifiSpectrumBandInfo GetSecondaryBand(uint16_t bandWidth) const;

    /**
     * Return the channel width used to measure the RSSI.
     *
     * \param ppdu the PPDU that is being received
     * \return the channel width (in MHz) used for RSSI measurement
     */
    virtual uint16_t GetMeasurementChannelWidth(const Ptr<const WifiPpdu> ppdu) const = 0;

    /**
     * Return the channel width used in the reception spectrum model.
     *
     * \param txVector the TXVECTOR of the PPDU that is being received
     * \return the channel width (in MHz) used for RxSpectrumModel
     */
    virtual uint16_t GetRxChannelWidth(const WifiTxVector& txVector) const;

    /**
     * Return the delay until CCA busy is ended for a given sensitivity threshold (in dBm) and a
     * given band.
     *
     * \param thresholdDbm the CCA sensitivity threshold in dBm
     * \param band identify the requested band
     * \return the delay until CCA busy is ended
     */
    Time GetDelayUntilCcaEnd(double thresholdDbm, const WifiSpectrumBandInfo& band);

    /**
     * \param currentChannelWidth channel width of the current transmission (MHz)
     * \return the width of the guard band (MHz)
     *
     * Wrapper method used by child classes for PSD generation.
     * Note that this method is necessary for testing UL OFDMA.
     */
    uint16_t GetGuardBandwidth(uint16_t currentChannelWidth) const;
    /**
     * \return a tuple containing the minimum rejection (in dBr) for the inner band,
     *                            the minimum rejection (in dBr) for the outer band, and
     *                            the maximum rejection (in dBr) for the outer band
     *                            for the transmit spectrum mask.
     *
     * Wrapper method used by child classes for PSD generation.
     */
    std::tuple<double, double, double> GetTxMaskRejectionParams() const;

    using CcaIndication =
        std::optional<std::pair<Time, WifiChannelListType>>; //!< CCA end time and its corresponding
                                                             //!< channel list type (can be
                                                             //!< std::nullopt if IDLE)

    /**
     * Get CCA end time and its corresponding channel list type when a new signal has been received
     * by the PHY.
     *
     * \param ppdu the incoming PPDU or nullptr for any signal
     * \return CCA end time and its corresponding channel list type when a new signal has been
     * received by the PHY, or std::nullopt if all channel list types are IDLE.
     */
    virtual CcaIndication GetCcaIndication(const Ptr<const WifiPpdu> ppdu);

    Ptr<WifiPhy> m_wifiPhy;          //!< Pointer to the owning WifiPhy
    Ptr<WifiPhyStateHelper> m_state; //!< Pointer to WifiPhyStateHelper of the WifiPhy (to make it
                                     //!< reachable for child classes)

    std::list<WifiMode> m_modeList; //!< the list of supported modes

    std::vector<EventId> m_endPreambleDetectionEvents; //!< the end of preamble detection events
    std::vector<EventId> m_endOfMpduEvents; //!< the end of MPDU events (only used for A-MPDUs)

    std::vector<EventId>
        m_endRxPayloadEvents; //!< the end of receive events (only one unless UL MU reception)

    /**
     * A pair of a UID and STA_ID
     */
    typedef std::pair<uint64_t /* UID */, uint16_t /* STA-ID */> UidStaIdPair;

    std::map<UidStaIdPair, std::vector<bool>>
        m_statusPerMpduMap; //!< Map of the current reception status per MPDU that is filled in as
                            //!< long as MPDUs are being processed by the PHY in case of an A-MPDU
    std::map<UidStaIdPair, SignalNoiseDbm>
        m_signalNoiseMap; //!< Map of the latest signal power and noise power in dBm (noise power
                          //!< includes the noise figure)

    static uint64_t m_globalPpduUid; //!< Global counter of the PPDU UID
};                                   // class PhyEntity

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param action the action to perform in case of failure
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const PhyEntity::PhyRxFailureAction& action);
/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param status the status of the reception of a PPDU field
 * \returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const PhyEntity::PhyFieldRxStatus& status);

} // namespace ns3

#endif /* PHY_ENTITY_H */
