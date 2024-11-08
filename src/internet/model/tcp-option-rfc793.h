/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */
#ifndef TCPOPTIONRFC793_H
#define TCPOPTIONRFC793_H

#include "tcp-option.h"

namespace ns3
{

/**
 * @ingroup tcp
 *
 * Defines the TCP option of kind 0 (end of option list) as in \RFC{793}
 */
class TcpOptionEnd : public TcpOption
{
  public:
    TcpOptionEnd();
    ~TcpOptionEnd() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;
};

/**
 * Defines the TCP option of kind 1 (no operation) as in \RFC{793}
 */
class TcpOptionNOP : public TcpOption
{
  public:
    TcpOptionNOP();
    ~TcpOptionNOP() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;
};

/**
 * Defines the TCP option of kind 2 (maximum segment size) as in \RFC{793}
 */
class TcpOptionMSS : public TcpOption
{
  public:
    TcpOptionMSS();
    ~TcpOptionMSS() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

    /**
     * @brief Get the Maximum Segment Size stored in the Option
     * @return The Maximum Segment Size
     */
    uint16_t GetMSS() const;
    /**
     * @brief Set the Maximum Segment Size stored in the Option
     * @param mss The Maximum Segment Size
     */
    void SetMSS(uint16_t mss);

  protected:
    uint16_t m_mss; //!< maximum segment size
};

} // namespace ns3

#endif // TCPOPTIONRFC793_H
