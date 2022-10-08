/*
 * Copyright (c) 2008-2009 Strasbourg University
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
 * Author: David Gross <gdavid.devel@gmail.com>
 */

#ifndef IPV6_OPTION_HEADER_H
#define IPV6_OPTION_HEADER_H

#include "ns3/header.h"

#include <ostream>

namespace ns3
{

/**
 * \ingroup ipv6HeaderExt
 *
 * \brief Header for IPv6 Option.
 */
class Ipv6OptionHeader : public Header
{
  public:
    /**
     * \struct Alignment
     * \brief represents the alignment requirements of an option header
     *
     * Represented as factor*n+offset (eg. 8n+2) See \RFC{2460}.
     * No alignment is represented as 1n+0.
     */
    struct Alignment
    {
        uint8_t factor; /**< Factor */
        uint8_t offset; /**< Offset */
    };

    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief Constructor.
     */
    Ipv6OptionHeader();

    /**
     * \brief Destructor.
     */
    ~Ipv6OptionHeader() override;

    /**
     * \brief Set the type of the option.
     * \param type the type of the option
     */
    void SetType(uint8_t type);

    /**
     * \brief Get the type of the option.
     * \return the type of the option
     */
    uint8_t GetType() const;

    /**
     * \brief Set the option length.
     * \param length the option length
     */
    void SetLength(uint8_t length);

    /**
     * \brief Get the option length.
     * \return the option length
     */
    uint8_t GetLength() const;

    /**
     * \brief Print some information about the packet.
     * \param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Get the Alignment requirement of this option header
     * \return The required alignment
     *
     * Subclasses should only implement this method, if special alignment is
     * required. Default is no alignment (1n+0).
     */
    virtual Alignment GetAlignment() const;

  private:
    /**
     * \brief The type of the option.
     */
    uint8_t m_type;

    /**
     * \brief The option length.
     */
    uint8_t m_length;

    /**
     * \brief The anonymous data of this option
     */
    Buffer m_data;
};

/**
 * \ingroup ipv6HeaderExt
 *
 * \brief Header of IPv6 Option Pad1
 */
class Ipv6OptionPad1Header : public Ipv6OptionHeader
{
  public:
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief Constructor.
     */
    Ipv6OptionPad1Header();

    /**
     * \brief Destructor.
     */
    ~Ipv6OptionPad1Header() override;

    /**
     * \brief Print some information about the packet.
     * \param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;
};

/**
 * \ingroup ipv6HeaderExt
 *
 * \brief Header of IPv6 Option Padn
 */
class Ipv6OptionPadnHeader : public Ipv6OptionHeader
{
  public:
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief Constructor.
     * \param pad Number of bytes to pad (>=2)
     */
    Ipv6OptionPadnHeader(uint32_t pad = 2);

    /**
     * \brief Destructor.
     */
    ~Ipv6OptionPadnHeader() override;

    /**
     * \brief Print some information about the packet.
     * \param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;
};

/**
 * \ingroup ipv6HeaderExt
 *
 * \brief Header of IPv6 Option Jumbogram
 */
class Ipv6OptionJumbogramHeader : public Ipv6OptionHeader
{
  public:
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief Constructor.
     */
    Ipv6OptionJumbogramHeader();

    /**
     * \brief Destructor.
     */
    ~Ipv6OptionJumbogramHeader() override;

    /**
     * \brief Set the data length.
     * \param dataLength the data length
     */
    void SetDataLength(uint32_t dataLength);

    /**
     * \brief Get the data length.
     * \return the data length
     */
    uint32_t GetDataLength() const;

    /**
     * \brief Print some information about the packet.
     * \param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Get the Alignment requirement of this option header
     * \return The required alignment
     */
    Alignment GetAlignment() const override;

  private:
    /**
     * \brief The data length.
     */
    uint32_t m_dataLength;
};

/**
 * \ingroup ipv6HeaderExt
 *
 * \brief Header of IPv6 Option Router Alert
 */
class Ipv6OptionRouterAlertHeader : public Ipv6OptionHeader
{
  public:
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief Constructor.
     */
    Ipv6OptionRouterAlertHeader();

    /**
     * \brief Destructor.
     */
    ~Ipv6OptionRouterAlertHeader() override;

    /**
     * \brief Set the field "value".
     * \param value the value to be set.
     */
    void SetValue(uint16_t value);

    /**
     * \brief Get the field "value".
     * \return the field "value"
     */
    uint16_t GetValue() const;

    /**
     * \brief Print some information about the packet.
     * \param os output stream
     */
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Get the Alignment requirement of this option header
     * \return The required alignment
     */
    Alignment GetAlignment() const override;

  private:
    /**
     * \brief The value.
     */
    uint16_t m_value;
};

} // namespace ns3

#endif /* IPV6_OPTION_HEADER_H */
