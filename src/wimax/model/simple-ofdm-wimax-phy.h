/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#ifndef SIMPLE_OFDM_WIMAX_PHY_H
#define SIMPLE_OFDM_WIMAX_PHY_H

#include "bvec.h"
#include "snr-to-block-error-rate-manager.h"
#include "wimax-connection.h"
#include "wimax-phy.h"

#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

#include <list>
#include <stdint.h>

namespace ns3
{

class WimaxChannel;
class WimaxNetDevice;
class NetDevice;
class Packet;
class SimpleOfdmWimaxChannel;

/**
 * \ingroup wimax
 * \brief SimpleOfdmWimaxPhy class
 */
class SimpleOfdmWimaxPhy : public WimaxPhy
{
  public:
    /// Frame duration code enumeration
    enum FrameDurationCode
    {
        FRAME_DURATION_2_POINT_5_MS,
        FRAME_DURATION_4_MS,
        FRAME_DURATION_5_MS,
        FRAME_DURATION_8_MS,
        FRAME_DURATION_10_MS,
        FRAME_DURATION_12_POINT_5_MS,
        FRAME_DURATION_20_MS
    };

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    SimpleOfdmWimaxPhy();
    /**
     * Constructor
     *
     * \param tracesPath trace path
     */
    SimpleOfdmWimaxPhy(char* tracesPath);
    ~SimpleOfdmWimaxPhy() override;
    /**
     * \brief if called with true it will enable the loss model
     * \param loss set to true to enable the loss model
     */
    void ActivateLoss(bool loss);
    /**
     * \brief Set the path of the repository containing the traces
     * \param tracesPath the path to the repository.
     *  see snr-to-block-error-rate-manager.h for more details.
     */
    void SetSNRToBlockErrorRateTracesPath(char* tracesPath);
    /**
     * Attach the physical layer to a channel.
     * \param channel the channel to which the physical layer will be attached
     */
    void DoAttach(Ptr<WimaxChannel> channel) override;
    /**
     * \brief set the callback function to call when a burst is received
     * \param callback the receive callback function
     */
    void SetReceiveCallback(Callback<void, Ptr<PacketBurst>, Ptr<WimaxConnection>> callback);
    /**
     * \brief Sends a burst on the channel
     * \param burst the burst to send
     * \param modulationType the modulation that will be used to send this burst
     * \param direction set to uplink or downlink
     */
    void Send(Ptr<PacketBurst> burst, WimaxPhy::ModulationType modulationType, uint8_t direction);
    /**
     * \brief Sends a burst on the channel
     * \param params parameters
     * \see SendParams
     */
    void Send(SendParams* params) override;
    /**
     * \brief returns the type this physical layer
     * \return always  WimaxPhy::simpleOfdmWimaxPhy;
     */
    WimaxPhy::PhyType GetPhyType() const override;
    /**
     * \brief start the reception of a fec block
     * \param burstSize the burst size
     * \param isFirstBlock true if this block is the first one, false otherwise
     * \param frequency the frequency in which the fec block is being received
     * \param modulationType the modulation used to transmit this fec Block
     * \param direction set to uplink and downlink
     * \param rxPower the received power.
     * \param burst the burst to be sent
     */

    void StartReceive(uint32_t burstSize,
                      bool isFirstBlock,
                      uint64_t frequency,
                      WimaxPhy::ModulationType modulationType,
                      uint8_t direction,
                      double rxPower,
                      Ptr<PacketBurst> burst);

    /**
     * \return the bandwidth
     */
    uint32_t GetBandwidth() const;
    /**
     * \brief Set the bandwidth
     * \param BW the bandwidth
     */
    void SetBandwidth(uint32_t BW);
    /**
     * \return the transmission power
     */
    double GetTxPower() const;
    /**
     * \brief set the transmission power
     * \param txPower the transmission power
     */
    void SetTxPower(double txPower);
    /**
     * \return the noise figure
     */
    double GetNoiseFigure() const;
    /**
     * \brief set the noise figure of the device
     * \param nf the noise figure
     */
    void SetNoiseFigure(double nf);

    /**
     * Public method used to fire a PhyTxBegin trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyTxBegin(Ptr<PacketBurst> burst);

    /**
     * Public method used to fire a PhyTxEnd trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyTxEnd(Ptr<PacketBurst> burst);

    /**
     * Public method used to fire a PhyTxDrop trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyTxDrop(Ptr<PacketBurst> burst);

    /**
     * Public method used to fire a PhyRxBegin trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyRxBegin(Ptr<PacketBurst> burst);

    /**
     * Public method used to fire a PhyRxEnd trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyRxEnd(Ptr<PacketBurst> burst);

    /**
     * Public method used to fire a PhyRxDrop trace.  Implemented for encapsulation
     * purposes.
     * \param burst the packet burst
     */
    void NotifyRxDrop(Ptr<PacketBurst> burst);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;

  private:
    /**
     * Get transmission time
     * \param size the size
     * \param modulationType the modulation type
     * \returns the transmission time
     */
    Time DoGetTransmissionTime(uint32_t size,
                               WimaxPhy::ModulationType modulationType) const override;
    /**
     * Get number of symbols
     * \param size the size
     * \param modulationType the modulation type
     * \returns the number of symbols
     */
    uint64_t DoGetNrSymbols(uint32_t size, WimaxPhy::ModulationType modulationType) const override;
    /**
     * Get number of bytes
     * \param symbols the symbols
     * \param modulationType the modulation type
     * \returns the number of bytes
     */
    uint64_t DoGetNrBytes(uint32_t symbols, WimaxPhy::ModulationType modulationType) const override;
    /**
     * Convert burst to bits
     * \param burst the packet burst
     * \returns the BVEC
     */
    Bvec ConvertBurstToBits(Ptr<const PacketBurst> burst);
    /**
     * Convert bits to burst
     * \param buffer the BVEC
     * \returns the packet burst
     */
    Ptr<PacketBurst> ConvertBitsToBurst(Bvec buffer);
    /**
     * Create FEC blocks
     * \param buffer the BVEC
     * \param modulationType the modulation type
     */
    void CreateFecBlocks(const Bvec& buffer, WimaxPhy::ModulationType modulationType);
    /**
     * Recreate buffer
     * \returns BVEC
     */
    Bvec RecreateBuffer();
    /**
     * Get FEC block size
     * \param type the modulation type
     * \returns the FEC block size
     */
    uint32_t GetFecBlockSize(WimaxPhy::ModulationType type) const;
    /**
     * Get coded FEC block size
     * \param modulationType the modulation type
     * \returns the coded FEC block size
     */
    uint32_t GetCodedFecBlockSize(WimaxPhy::ModulationType modulationType) const;
    /**
     * Set block parameters
     * \param burstSize the burst size
     * \param modulationType the modulation type
     */
    void SetBlockParameters(uint32_t burstSize, WimaxPhy::ModulationType modulationType);
    /**
     * Get number of blocks
     * \param burstSize the burst size
     * \param modulationType the modulation type
     * \returns the number of blocks
     */
    uint16_t GetNrBlocks(uint32_t burstSize, WimaxPhy::ModulationType modulationType) const;
    void DoDispose() override;
    /// End send
    void EndSend();
    /**
     * End send FEC block
     * \param modulationType the modulation type
     * \param direction the direction
     */
    void EndSendFecBlock(WimaxPhy::ModulationType modulationType, uint8_t direction);
    /**
     * End receive
     * \param burst
     */
    void EndReceive(Ptr<const PacketBurst> burst);
    /**
     * End receive FEC block
     * \param burstSize the burst size
     * \param modulationType the modulation type
     * \param direction the direction
     * \param drop whether to drop
     * \param burst the burst
     */
    void EndReceiveFecBlock(uint32_t burstSize,
                            WimaxPhy::ModulationType modulationType,
                            uint8_t direction,
                            bool drop,
                            Ptr<PacketBurst> burst);
    /**
     * Start end dummy FEC block
     * \param isFirstBlock is the first block?
     * \param modulationType the modulation type
     * \param direction the direction
     */
    void StartSendDummyFecBlock(bool isFirstBlock,
                                WimaxPhy::ModulationType modulationType,
                                uint8_t direction);
    /**
     * Get block transmission time
     * \param modulationType the modulation type
     * \returns the block transmission time
     */
    Time GetBlockTransmissionTime(WimaxPhy::ModulationType modulationType) const;
    /// Set data rates
    void DoSetDataRates() override;
    /// Initialize simple OFDM WIMAX Phy
    void InitSimpleOfdmWimaxPhy();

    /**
     * Get moduleation FEC parameters
     * \param modulationType the modulation type
     * \param bitsPerSymbol the number of bits per symbol
     * \param fecCode the FEC code
     */
    void GetModulationFecParams(WimaxPhy::ModulationType modulationType,
                                uint8_t& bitsPerSymbol,
                                double& fecCode) const;
    /**
     * Calculate data rate
     * \param modulationType the modulation type
     * \returns the data rate
     */
    uint32_t CalculateDataRate(WimaxPhy::ModulationType modulationType) const;
    /**
     * Get data rate
     * \param modulationType the modulation type
     * \returns the data rate
     */
    uint32_t DoGetDataRate(WimaxPhy::ModulationType modulationType) const override;
    /**
     * Get TTG
     * \returns the TTG
     */
    uint16_t DoGetTtg() const override;
    /**
     * Get RTG
     * \returns the RTG
     */
    uint16_t DoGetRtg() const override;
    /**
     * Get frame duration code
     * \returns the frame duration code
     */
    uint8_t DoGetFrameDurationCode() const override;
    /**
     * Get frame duration
     * \param frameDurationCode the frame duration code
     * \returns the frame duration
     */
    Time DoGetFrameDuration(uint8_t frameDurationCode) const override;
    /// Set Phy parameters
    void DoSetPhyParameters() override;
    /**
     * Get NFFT
     * \returns the NFFT
     */
    uint16_t DoGetNfft() const override;
    /**
     * Set NFFT
     * \param nfft the NFFT
     */
    void DoSetNfft(uint16_t nfft);
    /**
     * Get sampling factor
     * \returns the sampling factor
     */
    double DoGetSamplingFactor() const override;
    /**
     * Get sampling frequency
     * \returns the sampling frequency
     */
    double DoGetSamplingFrequency() const override;
    /**
     * Get G value
     * \returns the G value
     */
    double DoGetGValue() const override;
    /**
     * Set G value
     * \param g the G value
     */
    void DoSetGValue(double g);

    /**
     * Get receive gain
     * \returns the receive gain
     */
    double GetRxGain() const;
    /**
     * Set receive gsain
     * \param rxgain the receive gain
     */
    void SetRxGain(double rxgain);

    /**
     * Get transmit gain
     * \returns the transmit gain
     */
    double GetTxGain() const;
    /**
     * Set transmit gain
     * \param txgain the transmit gain
     */
    void SetTxGain(double txgain);

    /**
     * Get trace file path
     * \returns the trace file path name
     */
    std::string GetTraceFilePath() const;
    /**
     * Set trace file path
     * \param path the trace file path
     */
    void SetTraceFilePath(std::string path);

    uint16_t m_fecBlockSize;     ///< in bits, size of FEC block transmitted after PHY operations
    uint32_t m_currentBurstSize; ///< current burst size

    std::list<Bvec>* m_receivedFecBlocks; ///< a list of received FEC blocks until they are combined
                                          ///< to recreate the full burst buffer
    uint32_t m_nrFecBlocksSent;   ///< counting the number of FEC blocks sent (within a burst)
    std::list<Bvec>* m_fecBlocks; ///< the FEC blocks
    Time m_blockTime;             ///< block time

    TracedCallback<Ptr<const PacketBurst>> m_traceRx; ///< trace receive callback
    TracedCallback<Ptr<const PacketBurst>> m_traceTx; ///< trace transmit callback

    // data rates for this Phy
    uint32_t m_dataRateBpsk12;   ///< data rate
    uint32_t m_dataRateQpsk12;   ///< data rate
    uint32_t m_dataRateQpsk34;   ///< data rate
    uint32_t m_dataRateQam16_12; ///< data rate
    uint32_t m_dataRateQam16_34; ///< data rate
    uint32_t m_dataRateQam64_23; ///< data rate
    uint32_t m_dataRateQam64_34; ///< data rate

    // parameters to store for a per burst life-time
    uint16_t m_nrBlocks;                ///< number of blocks
    uint16_t m_nrRemainingBlocksToSend; ///< number of remaining blocks to send
    Ptr<PacketBurst> m_currentBurst;    ///< current burst
    uint16_t m_blockSize;               ///< block size
    uint32_t m_paddingBits;             ///< padding bits
    uint16_t m_nbErroneousBlock;        ///< erroneous blocks
    uint16_t m_nrReceivedFecBlocks;     ///< number received FEC blocks
    uint16_t m_nfft;                    ///< NFFT
    double m_g;                         ///< G value
    double m_bandWidth;                 ///< bandwidth
    double m_txPower;                   ///< transmit power
    double m_noiseFigure;               ///< noise figure
    double m_txGain;                    ///< transmit gain
    double m_rxGain;                    ///< receive gain
    /**
     * The trace source fired when a packet begins the transmission process on
     * the medium.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyTxBeginTrace;

    /**
     * The trace source fired when a packet ends the transmission process on
     * the medium.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyTxEndTrace;

    /**
     * The trace source fired when the phy layer drops a packet as it tries
     * to transmit it.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyTxDropTrace;

    /**
     * The trace source fired when a packet begins the reception process from
     * the medium.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyRxBeginTrace;

    /**
     * The trace source fired when a packet ends the reception process from
     * the medium.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyRxEndTrace;

    /**
     * The trace source fired when the phy layer drops a packet it has received.
     *
     * \see class CallBackTraceSource
     * \deprecated The non-const \c Ptr<PacketBurst> argument is deprecated
     * and will be changed to \c Ptrc<PacketBurst> in a future release.
     */
    TracedCallback<Ptr<PacketBurst>> m_phyRxDropTrace;

    SNRToBlockErrorRateManager* m_snrToBlockErrorRateManager; ///< SNR to block error rate manager

    /// Provides uniform random variables.
    Ptr<UniformRandomVariable> m_URNG; ///< URNG
};

} // namespace ns3

#endif /* OFDM_WIMAX_PHY_H */
