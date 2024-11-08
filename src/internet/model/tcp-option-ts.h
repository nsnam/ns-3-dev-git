/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef TCP_OPTION_TS_H
#define TCP_OPTION_TS_H

#include "tcp-option.h"

#include "ns3/timer.h"

namespace ns3
{

/**
 * @ingroup tcp
 *
 * Defines the TCP option of kind 8 (timestamp option) as in \RFC{1323}
 */

class TcpOptionTS : public TcpOption
{
  public:
    TcpOptionTS();
    ~TcpOptionTS() override;

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
     * @brief Get the timestamp stored in the Option
     * @return the timestamp
     */
    uint32_t GetTimestamp() const;
    /**
     * @brief Get the timestamp echo stored in the Option
     * @return the timestamp echo
     */
    uint32_t GetEcho() const;
    /**
     * @brief Set the timestamp stored in the Option
     * @param ts the timestamp
     */
    void SetTimestamp(uint32_t ts);
    /**
     * @brief Set the timestamp echo stored in the Option
     * @param ts the timestamp echo
     */
    void SetEcho(uint32_t ts);

    /**
     * @brief Return an uint32_t value which represent "now"
     *
     * The value returned is usually used as Timestamp option for the
     * TCP header; when the value will be echoed back, calculating the RTT
     * will be an easy matter.
     *
     * The RFC does not mention any units for this value; following what
     * is implemented in OS, we use milliseconds. Any change to this must be
     * reflected to EstimateRttFromTs.
     *
     * @see EstimateRttFromTs
     * @return The Timestamp value to use
     */
    static uint32_t NowToTsValue();

    /**
     * @brief Estimate the Time elapsed from a TS echo value
     *
     * The echoTime should be a value returned from NowToTsValue.
     *
     * @param echoTime Echoed value from other side
     * @see NowToTsValue
     * @return The measured RTT
     */
    static Time ElapsedTimeFromTsValue(uint32_t echoTime);

  protected:
    uint32_t m_timestamp; //!< local timestamp
    uint32_t m_echo;      //!< echo timestamp
};

} // namespace ns3

#endif /* TCP_OPTION_TS */
