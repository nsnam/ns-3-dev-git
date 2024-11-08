/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#ifndef DL_FRAME_PREFIX_IE_H
#define DL_FRAME_PREFIX_IE_H

#include "ns3/header.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup wimax
 * @brief This class implements  the DL Frame Prefix IE as described by IEEE-802.16 standard
 * @brief The DL Frame Prefix IE is contained in DLFP (Downlink Frame Prefix) in OFDM PHY
 */
class DlFramePrefixIe
{
  public:
    DlFramePrefixIe();
    ~DlFramePrefixIe();

    /**
     * Set rate ID field
     * @param rateId the rate ID
     */
    void SetRateId(uint8_t rateId);
    /**
     * Set DIUC field
     * @param diuc the DIUC
     */
    void SetDiuc(uint8_t diuc);
    /**
     * Set preamble present field
     * @param preamblePresent the preambel present
     */
    void SetPreamblePresent(uint8_t preamblePresent);
    /**
     * Set length field
     * @param length the length
     */
    void SetLength(uint16_t length);
    /**
     * Set start time field
     * @param startTime the start time
     */
    void SetStartTime(uint16_t startTime);

    /**
     * Get rate ID field
     * @returns the rate ID
     */
    uint8_t GetRateId() const;
    /**
     * Get DIUC field
     * @returns the DIUC
     */
    uint8_t GetDiuc() const;
    /**
     * Get preamble present field
     * @returns the preamble present
     */
    uint8_t GetPreamblePresent() const;
    /**
     * Get length field
     * @returns the length
     */
    uint16_t GetLength() const;
    /**
     * Get start time field
     * @returns the start time
     */
    uint16_t GetStartTime() const;

    /**
     * Get size field
     * @returns the size
     */
    uint16_t GetSize() const;

    /**
     * Write item function
     * @param start the iterator
     * @returns the updated iterator
     */
    Buffer::Iterator Write(Buffer::Iterator start) const;
    /**
     * Read item function
     * @param start the iterator
     * @returns the updated iterator
     */
    Buffer::Iterator Read(Buffer::Iterator start);

  private:
    uint8_t m_rateId;          ///< rate ID
    uint8_t m_diuc;            ///< DIUC
    uint8_t m_preamblePresent; ///< preamble present
    uint16_t m_length;         ///< length
    uint16_t m_startTime;      ///< start time

    // shall actually contain m_startTime if DIUC is 0. see Table 225, page 452
};

} // namespace ns3

#endif /* DL_FRAME_PREFIX_IE_H */

#ifndef OFDM_DOWNLINK_FRAME_PREFIX_H
#define OFDM_DOWNLINK_FRAME_PREFIX_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"

#include <stdint.h>

namespace ns3
{

/**
 * OfdmDownlinkFramePrefix
 */
class OfdmDownlinkFramePrefix : public Header
{
  public:
    OfdmDownlinkFramePrefix();
    ~OfdmDownlinkFramePrefix() override;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set base station ID field
     * @param baseStationId the base station ID
     */
    void SetBaseStationId(Mac48Address baseStationId);
    /**
     * Set frame number field
     * @param frameNumber the frame number
     */
    void SetFrameNumber(uint32_t frameNumber);
    /**
     * Set configuration change count field
     * @param configurationChangeCount the configuration change count
     */
    void SetConfigurationChangeCount(uint8_t configurationChangeCount);
    /**
     * Add DL frame prefix element field
     * @param dlFramePrefixElement the DL frame prefix element
     */
    void AddDlFramePrefixElement(DlFramePrefixIe dlFramePrefixElement);
    /**
     * Set HCS field
     * @param hcs the HCS
     */
    void SetHcs(uint8_t hcs);

    /**
     * Get base station ID field
     * @returns the base station ID
     */
    Mac48Address GetBaseStationId() const;
    /**
     * Get frame number field
     * @returns the frame number
     */
    uint32_t GetFrameNumber() const;
    /**
     * Get configuration change count field
     * @returns the configuration change count
     */
    uint8_t GetConfigurationChangeCount() const;
    /**
     * Get DL frame prefix elements
     * @returns the DL frame prefix elements
     */
    std::vector<DlFramePrefixIe> GetDlFramePrefixElements() const;
    /**
     * Get HCS field
     * @returns the HCS
     */
    uint8_t GetHcs() const;

    /**
     * Get name field
     * @returns the name
     */
    std::string GetName() const;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    Mac48Address m_baseStationId; ///< base station ID
    uint32_t m_frameNumber; ///< shall actually be only 4 LSB of the same field in OFDM DCD Channel
                            ///< Encodings
    uint8_t m_configurationChangeCount; ///< shall actually be only 4 LSB of the same field in DCD
    std::vector<DlFramePrefixIe> m_dlFramePrefixElements; ///< vector of dl frame prefix elements
    uint8_t m_hcs;                                        ///< Header Check Sequence
};

} // namespace ns3

#endif /* OFDM_DOWNLINK_FRAME_PREFIX_H */
