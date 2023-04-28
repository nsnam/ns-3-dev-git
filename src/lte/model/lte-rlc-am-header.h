/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_RLC_AM_HEADER_H
#define LTE_RLC_AM_HEADER_H

#include "lte-rlc-sequence-number.h"

#include "ns3/header.h"

#include <list>

namespace ns3
{

/**
 * \ingroup lte
 * \brief The packet header for the AM Radio Link Control (RLC) protocol packets
 *
 * This class has fields corresponding to those in an RLC header as well as
 * methods for serialization to and deserialization from a byte buffer.
 * It follows 3GPP TS 36.322 Radio Link Control (RLC) protocol specification.
 */
class LteRlcAmHeader : public Header
{
  public:
    /**
     * \brief Constructor
     *
     * Creates a null header
     */
    LteRlcAmHeader();
    ~LteRlcAmHeader() override;

    /// Set data PDU function
    void SetDataPdu();
    /**
     * Set control PDU function
     *
     * \param controlPduType
     */
    void SetControlPdu(uint8_t controlPduType);
    /**
     * Is data PDU function
     * \returns true if data PDU
     */
    bool IsDataPdu() const;
    /**
     * Is control PDU function
     * \returns true if control PDU
     */
    bool IsControlPdu() const;

    /// DataControlPdu_t enumeration
    enum DataControlPdu_t
    {
        CONTROL_PDU = 0,
        DATA_PDU = 1
    };

    /// Control PDU type status
    static constexpr uint8_t STATUS_PDU{0};

    //
    // DATA PDU
    //

    /**
     * Set sequence number
     *
     * \param sequenceNumber sequence number
     */
    void SetSequenceNumber(SequenceNumber10 sequenceNumber);
    /**
     * Get sequence number
     *
     * \returns sequence number
     */
    SequenceNumber10 GetSequenceNumber() const;

    /**
     * Set sequence number
     *
     * \param framingInfo framing info
     */
    void SetFramingInfo(uint8_t framingInfo);
    /**
     * Get framing info
     *
     * \returns framing info
     */
    uint8_t GetFramingInfo() const;

    /// FramingInfoByte_t enumeration
    enum FramingInfoByte_t
    {
        FIRST_BYTE = 0x00,
        NO_FIRST_BYTE = 0x02,
        LAST_BYTE = 0x00,
        NO_LAST_BYTE = 0x01
    };

    /**
     * Push extension bit function
     *
     * \param extensionBit the extension bit
     */
    void PushExtensionBit(uint8_t extensionBit);
    /**
     * Push length indicator function
     *
     * \param lengthIndicator the length indicator
     */
    void PushLengthIndicator(uint16_t lengthIndicator);

    /**
     * Pop extension bit function
     *
     * \returns the extension bit
     */
    uint8_t PopExtensionBit();
    /**
     * Pop length indicator function
     *
     * \returns the length indicator
     */
    uint16_t PopLengthIndicator();

    /// ExtensionBit_t typedef
    enum ExtensionBit_t
    {
        DATA_FIELD_FOLLOWS = 0,
        E_LI_FIELDS_FOLLOWS = 1
    };

    /**
     * Pop extension bit function
     *
     * \param resegFlag resegmentation flag
     */
    void SetResegmentationFlag(uint8_t resegFlag);
    /**
     * Get resegmentation flag function
     *
     * \returns resegmentation flag
     */
    uint8_t GetResegmentationFlag() const;

    /// ResegmentationFlag_t typedef
    enum ResegmentationFlag_t
    {
        PDU = 0,
        SEGMENT = 1
    };

    /**
     * Set polling bit function
     *
     * \param pollingBit polling bit
     */
    void SetPollingBit(uint8_t pollingBit);
    /**
     * Get polling bit function
     *
     * \returns polling bit
     */
    uint8_t GetPollingBit() const;

    /// PollingBit_t enumeration
    enum PollingBit_t
    {
        STATUS_REPORT_NOT_REQUESTED = 0,
        STATUS_REPORT_IS_REQUESTED = 1
    };

    /**
     * Set last segment flag function
     *
     * \param lsf last segment flag
     */
    void SetLastSegmentFlag(uint8_t lsf);
    /**
     * Get last segment flag function
     *
     * \returns last segment flag
     */
    uint8_t GetLastSegmentFlag() const;

    /// LastSegmentFlag_t typedef
    enum LastSegmentFlag_t
    {
        NO_LAST_PDU_SEGMENT = 0,
        LAST_PDU_SEGMENT = 1
    };

    /**
     * Set segment offset function
     *
     * \param segmentOffset segment offset
     */
    void SetSegmentOffset(uint16_t segmentOffset);
    /**
     * Get segment offset function
     *
     * \returns segment offset
     */
    uint16_t GetSegmentOffset() const;
    /**
     * Get last offset function
     *
     * \returns last offset
     */
    uint16_t GetLastOffset() const;

    //
    // CONTROL PDU
    //

    /**
     * Set ack sn function
     *
     * \param ackSn ack sn
     */
    void SetAckSn(SequenceNumber10 ackSn);
    /**
     * Get ack sn function
     *
     * \returns sequence number
     */
    SequenceNumber10 GetAckSn() const;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     *
     *
     * \param bytes max allowed CONTROL PDU size
     *
     * \return true if one more NACK would fit in the CONTROL PDU; false otherwise
     */
    bool OneMoreNackWouldFitIn(uint16_t bytes);

    /**
     * Add one more NACK to the CONTROL PDU
     *
     * \param nack
     */
    void PushNack(int nack);

    /**
     *
     *
     * \param nack SN of the NACK
     *
     * \return true if the NACK is present in the STATUS PDU, false otherwise
     */
    bool IsNackPresent(SequenceNumber10 nack);

    /**
     * Retrieve one NACK from the CONTROL PDU
     *
     *
     * \return the SN  >= 0 of the next nack; returns -1 if no NACK is left
     */
    int PopNack();

  private:
    uint16_t m_headerLength;  ///< header length
    uint8_t m_dataControlBit; ///< data control bit

    // Data PDU fields
    uint8_t m_resegmentationFlag;      ///< resegmentation flag
    uint8_t m_pollingBit;              ///< polling bit
    uint8_t m_framingInfo;             ///<  2 bits
    SequenceNumber10 m_sequenceNumber; ///< sequence number
    uint8_t m_lastSegmentFlag;         ///< last segment flag
    uint16_t m_segmentOffset;          ///< segment offset
    uint16_t m_lastOffset;             ///< last offset

    std::list<uint8_t> m_extensionBits;     ///< Includes extensionBit of the fixed part
    std::list<uint16_t> m_lengthIndicators; ///< length indicators

    // Control PDU fields
    uint8_t m_controlPduType; ///< control PDU type

    // Status PDU fields
    SequenceNumber10 m_ackSn;    ///< ack sn
    std::list<int> m_nackSnList; ///< nack sn list

    std::list<uint8_t> m_extensionBits1; ///< Includes E1 after ACK_SN
    std::list<uint8_t> m_extensionBits2; ///< extension bits 2
};

}; // namespace ns3

#endif // LTE_RLC_AM_HEADER_H
