/*
 * Copyright (c) 2015 Università di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Adnan Rashid <adnanrashidpk@gmail.com>
 */

#ifndef SIXLOWPAN_ND_HEADER_H
#define SIXLOWPAN_ND_HEADER_H

#include "ns3/header.h"
#include "ns3/icmpv6-header.h"
#include "ns3/ipv6-address.h"
#include "ns3/mac64-address.h"

#include <vector>

namespace ns3
{

/**
 * @ingroup sixlowpan
 * @brief ICMPv6 Extended Duplicate Address Request or Confirmation header (see \RFC{8505}).
 */
class Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf : public Icmpv6Header
{
  public:
    /**
     * @brief Constructor.
     */
    Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf();

    /**
     * @brief Constructor.
     * @param request duplicate address request or duplicate address confirm
     */
    Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(bool request);

    /**
     * @brief Constructor (DAR).
     * @param time the registration lifetime (units of 60 seconds)
     * @param rovr the ROVR value.
     * @param address the registered address
     */
    Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(uint16_t time,
                                                     std::vector<uint8_t> rovr,
                                                     Ipv6Address address);

    /**
     * @brief Constructor (DAC).
     * @param status the status (DAC)
     * @param time the registration lifetime (units of 60 seconds)
     * @param rovr the ROVR value.
     * @param address the registered address
     */
    Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(uint8_t status,
                                                     uint16_t time,
                                                     std::vector<uint8_t> rovr,
                                                     Ipv6Address address);

    /**
     * @brief Destructor.
     */
    ~Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf() override;

    /**
     * @brief Get the UID of this class.
     * @return UID
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the status field.
     * @return status value
     */
    uint8_t GetStatus() const;

    /**
     * @brief Set the status field.
     * @param status the status value
     */
    void SetStatus(uint8_t status);

    /**
     * @brief Get the transaction ID field.
     * @return Transaction ID value
     */
    uint8_t GetTransaction_ID() const;

    /**
     * @brief Set the transaction ID field.
     * @param tid the transaction ID value
     */
    void SetTransaction_ID(uint8_t tid);

    /**
     * @brief Get the registration lifetime field.
     * @return registration lifetime value (units of 60 seconds)
     */
    uint16_t GetRegTime() const;

    /**
     * @brief Set the registration lifetime field.
     * @param time the registration lifetime value (units of 60 seconds)
     */
    void SetRegTime(uint16_t time);

    /**
     * @brief Get the ROVR field.
     * @return the ROVR
     */
    std::vector<uint8_t> GetRovr() const;

    /**
     * @brief Set the ROVR field.
     * @param rovr the ROVR value
     */
    void SetRovr(const std::vector<uint8_t>& rovr);

    /**
     * @brief Get the registered address field.
     * @return registered address value
     */
    Ipv6Address GetRegAddress() const;

    /**
     * @brief Set the registered address field.
     * @param registered the registered address value
     */
    void SetRegAddress(Ipv6Address registered);

    /**
     * @brief Print information.
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size.
     * @return serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the packet.
     * @param start start offset
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the packet.
     * @param start start offset
     * @return length of packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief The status value.
     */
    uint8_t m_status;

    /**
     * @brief The Transaction ID value.
     */
    uint16_t m_tid;

    /**
     * @brief The registration lifetime value (units of 60 seconds).
     */
    uint16_t m_regTime;
    /**
     * @brief The ROVR value.
     */
    std::vector<uint8_t> m_rovr;

    /**
     * @brief The registered address value.
     */
    Ipv6Address m_regAddress;
};

/**
 * @ingroup sixlowpan
 * @brief ICMPv6 Extended Address Registration Option header \RFC{8505}.
 *
  @verbatim
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Type      |     Length    |    Status     |    Opaque     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |  Rsvd | I |R|T|     TID       |     Registration Lifetime     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                                                               |
    ...            Registration Ownership Verifier (ROVR)           ...
     |                                                               |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   @endverbatim
 */
class Icmpv6OptionSixLowPanExtendedAddressRegistration : public Icmpv6OptionHeader
{
  public:
    /**
     * @brief Constructor.
     */
    Icmpv6OptionSixLowPanExtendedAddressRegistration();

    /**
     * @brief Constructor.
     * @param time the registration lifetime (units of 60 seconds)
     * @param rovr the ROVR value
     * @param tid the TID value
     */
    Icmpv6OptionSixLowPanExtendedAddressRegistration(uint16_t time,
                                                     const std::vector<uint8_t>& rovr,
                                                     uint8_t tid);

    /**
     * @brief Constructor.
     * @param status the status value
     * @param time the registration lifetime (units of 60 seconds)
     * @param rovr the ROVR value
     * @param tid the TID value
     */
    Icmpv6OptionSixLowPanExtendedAddressRegistration(uint8_t status,
                                                     uint16_t time,
                                                     const std::vector<uint8_t>& rovr,
                                                     uint8_t tid);

    /**
     * @brief Destructor.
     */
    ~Icmpv6OptionSixLowPanExtendedAddressRegistration() override;

    /**
     * @brief Get the UID of this class.
     * @return UID
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the status field.
     * @return status value
     */
    uint8_t GetStatus() const;

    /**
     * @brief Set the status field.
     * @param status the status value
     */
    void SetStatus(uint8_t status);

    /**
     * @brief Get the opaque field.
     * @return opaque value
     */
    uint8_t GetOpaque() const;

    /**
     * @brief Set the opaque field.
     * @param opaque the opaque value
     */
    void SetOpaque(uint8_t opaque);

    /**
     * @brief Get the I-TwoBit field.
     * @return I-TwoBit value
     */
    uint8_t GetI() const;

    /**
     * @brief Set the I-TwoBit  field.
     * @param twobits the TwoBit  value
     */
    void SetI(uint8_t twobits);

    /**
     * @brief Get the R flag.
     * @return R flag
     */
    bool GetFlagR() const;

    /**
     * @brief Set the R flag.
     * @param r value
     */
    void SetFlagR(bool r);

    /**
     * @brief Get the transaction ID field.
     * @return Transaction ID value
     */
    uint8_t GetTransactionId() const;

    /**
     * @brief Set the transaction ID field.
     * @param tid the transaction ID value
     */
    void SetTransactionId(uint8_t tid);

    /**
     * @brief Get the registration lifetime field.
     * @return registration lifetime value (units of 60 seconds)
     */
    uint16_t GetRegTime() const;

    /**
     * @brief Set the registration lifetime field.
     * @param time the registration lifetime value (units of 60 seconds)
     */
    void SetRegTime(uint16_t time);

    /**
     * @brief Get the ROVR field.
     * @return the ROVR
     */
    std::vector<uint8_t> GetRovr() const;

    /**
     * @brief Set the ROVR field.
     * @param rovr the ROVR value
     */
    void SetRovr(const std::vector<uint8_t>& rovr);

    /**
     * @brief Print information.
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size.
     * @return serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the packet.
     * @param start start offset
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the packet.
     * @param start start offset
     * @return length of packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief The status value.
     */
    uint8_t m_status;

    /**
     * @brief The opaque value.
     */
    uint8_t m_opaque;

    /**
     * @brief The I Two bit value.
     */
    uint8_t m_i;

    /**
     * @brief The R flag.
     */
    bool m_flagR;

    /**
     * @brief The Transaction ID value.
     */
    uint16_t m_tid;

    /**
     * @brief The registration lifetime value (units of 60 seconds).
     */
    uint16_t m_regTime;

    /**
     * @brief The ROVR value.
     */
    std::vector<uint8_t> m_rovr;
};

/**
 * @ingroup sixlowpan
 * @brief ICMPv6 SixLowPan Context Option header (see \RFC{8505}).
 */
class Icmpv6OptionSixLowPanContext : public Icmpv6OptionHeader
{
  public:
    /**
     * @brief Constructor.
     */
    Icmpv6OptionSixLowPanContext();

    /**
     * @brief Constructor.
     * @param c the c flag
     * @param cid the context identifier
     * @param time the valid lifetime (units of 60 seconds)
     * @param prefix the context prefix
     */
    Icmpv6OptionSixLowPanContext(bool c, uint8_t cid, uint16_t time, Ipv6Prefix prefix);

    /**
     * @brief Destructor.
     */
    ~Icmpv6OptionSixLowPanContext() override;

    /**
     * @brief Get the UID of this class.
     * @return UID
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the context length field.
     * @return context length value
     */
    uint8_t GetContextLen() const;

    /**
     * @brief Is compression flag ?
     * @return true if context is valid for use in compression, false otherwise
     */
    bool IsFlagC() const;

    /**
     * @brief Set the C flag.
     * @param c the C flag
     */
    void SetFlagC(bool c);

    /**
     * @brief Get the context identifier field.
     * @return context identifier value
     */
    uint8_t GetCid() const;

    /**
     * @brief Set the context identifier field.
     * @param cid the context identifier value
     */
    void SetCid(uint8_t cid);

    /**
     * @brief Get the valid lifetime field.
     * @return valid lifetime value (units of 60 seconds)
     */
    uint16_t GetValidTime() const;

    /**
     * @brief Set the valid lifetime field.
     * @param time the valid lifetime value (units of 60 seconds)
     */
    void SetValidTime(uint16_t time);

    /**
     * @brief Get the context prefix field.
     * @return context prefix value
     */
    Ipv6Prefix GetContextPrefix() const;

    /**
     * @brief Set the context prefix field.
     * @param prefix the context prefix value
     */
    void SetContextPrefix(Ipv6Prefix prefix);

    /**
     * @brief Print information.
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size.
     * @return serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the packet.
     * @param start start offset
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the packet.
     * @param start start offset
     * @return length of packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief The context length value.
     */
    uint8_t m_contextLen;

    /**
     * @brief The compression flag, indicates that this context is valid for use in compression.
     */
    bool m_c;

    /**
     * @brief The context identifier value.
     */
    uint8_t m_cid;

    /**
     * @brief The valid lifetime value (units of 60 seconds).
     */
    uint16_t m_validTime;

    /**
     * @brief The context prefix value.
     */
    Ipv6Prefix m_prefix;
};

/**
 * @ingroup sixlowpan
 * @brief ICMPv6 Authoritative Border Router Option header (see \RFC{8505}).
 */
class Icmpv6OptionSixLowPanAuthoritativeBorderRouter : public Icmpv6OptionHeader
{
  public:
    /**
     * @brief Constructor.
     */
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter();

    /**
     * @brief Constructor.
     * @param version the version value
     * @param time the valid lifetime (units of 60 seconds)
     * @param address the 6LBR address
     */
    Icmpv6OptionSixLowPanAuthoritativeBorderRouter(uint32_t version,
                                                   uint16_t time,
                                                   Ipv6Address address);

    /**
     * @brief Destructor.
     */
    ~Icmpv6OptionSixLowPanAuthoritativeBorderRouter() override;

    /**
     * @brief Get the UID of this class.
     * @return UID
     */
    static TypeId GetTypeId();

    /**
     * @brief Get the instance type ID.
     * @return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * @brief Get the version field.
     * @return version value
     */
    uint32_t GetVersion() const;

    /**
     * @brief Set the version field.
     * @param version the version value
     */
    void SetVersion(uint32_t version);

    /**
     * @brief Get the valid lifetime field.
     * @return valid lifetime value (units of 60 seconds)
     */
    uint16_t GetValidLifeTime() const;

    /**
     * @brief Set the valid lifetime field.
     * @param time the valid lifetime value (units of 60 seconds)
     */
    void SetValidLifeTime(uint16_t time);

    /**
     * @brief Get the 6LBR address field.
     * @return 6LBR address value
     */
    Ipv6Address GetRouterAddress() const;

    /**
     * @brief Set the 6LBR address field.
     * @param router the 6LBR address value
     */
    void SetRouterAddress(Ipv6Address router);

    /**
     * @brief Print information.
     * @param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * @brief Get the serialized size.
     * @return serialized size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Serialize the packet.
     * @param start start offset
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief Deserialize the packet.
     * @param start start offset
     * @return length of packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief The version value.
     */
    uint32_t m_version;

    /**
     * @brief The valid lifetime value (units of 60 seconds).
     */
    uint16_t m_validTime;

    /**
     * @brief The 6LBR address value.
     */
    Ipv6Address m_routerAddress;
};

} /* namespace ns3 */

#endif /* SIXLOWPAN_ND_HEADER_H */
