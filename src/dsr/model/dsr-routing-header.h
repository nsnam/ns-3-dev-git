/*
 * Copyright (c) 2011 Yufei Cheng
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
 * Modified by: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *              Lorenzo Bartolini <l.bartolini02@gmail.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#ifndef DSR_FS_HEADER_H
#define DSR_FS_HEADER_H

#include "dsr-option-header.h"

#include "ns3/header.h"
#include "ns3/ipv4-address.h"

#include <list>
#include <ostream>
#include <vector>

namespace ns3
{
namespace dsr
{
/**
 * @class DsrRoutingHeader
 * @brief Header of Dsr Routing
 */
/**
* @ingroup dsr
* @brief Dsr fixed size header Format
  \verbatim
   |      0        |      1        |      2        |      3        |
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Next Header |F|     Reserved    |       Payload Length       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             Options                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class DsrRoutingHeader : public Header
{
  public:
    /**
     * @brief Get the type identificator.
     * @return type identificator
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Constructor.
     */
    DsrRoutingHeader();

    /**
     * @brief Destructor.
     */
    ~DsrRoutingHeader() override;

    /**
     * @brief Print some information about the packet.
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Set the "Next header" field.
     * @param protocol the next header number
     */
    void SetNextHeader(uint8_t protocol);

    /**
     * @brief Get the next header.
     * @return the next header number
     */
    uint8_t GetNextHeader() const;

    /**
     * @brief Get the serialized size of the packet.
     * @return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the packet.
     * @param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the packet.
     * @param start Buffer iterator
     * @return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Serialize the option, prepending pad1 or padn option as necessary
     * @param option the option header to serialize
     */
    void AddDsrOption(const DsrOptionHeader& option);

    /**
     * @brief Get the payload length of the header.
     * @return the payload length of the header
     */
    uint16_t GetPayloadLength() const;

    /**
     * @brief Get the offset where the options begin, measured from the start of
     * the extension header.
     * @return the offset from the start of the extension header
     */
    uint32_t GetDsrOptionsOffset() const;

  private:
    /**
     * @brief Serialize all added options.
     * @param start Buffer iterator
     */
    void SerializeOptions(Buffer::Iterator start) const;

    /**
     * @brief Deserialize the packet.
     * @param start Buffer iterator
     * @param length length
     * @return size of the packet
     */
    uint32_t DeserializeOptions(Buffer::Iterator start, uint32_t length);

    /**
     * @brief Calculate padding.
     * @param alignment alignment
     * @return the number of bytes required to pad
     */
    uint32_t CalculatePad(DsrOptionHeader::Alignment alignment) const;

    /**
     * @brief Data payload.
     */
    Buffer m_optionData;

    /**
     * @brief The "next header" field.
     */
    uint8_t m_nextHeader;
};

/**
 * @brief Output streamer for DsrRoutingHeader
 * @param os The output stream
 * @param dsr The DsrRoutingHeader
 * @return The stream
 */
static inline std::ostream&
operator<<(std::ostream& os, const DsrRoutingHeader& dsr)
{
    dsr.Print(os);
    return os;
}

} // namespace dsr
} // namespace ns3

#endif /* DSR_FS_HEADER_H */
