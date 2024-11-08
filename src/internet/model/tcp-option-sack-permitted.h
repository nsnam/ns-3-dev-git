/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 * Copyright (c) 2015 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Original Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 * Documentation, test cases: Truc Anh N. Nguyen   <annguyen@ittc.ku.edu>
 *                            ResiliNets Research Group   https://resilinets.org/
 *                            The University of Kansas
 *                            James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 */

#ifndef TCP_OPTION_SACK_PERMITTED_H
#define TCP_OPTION_SACK_PERMITTED_H

#include "tcp-option.h"

namespace ns3
{

/**
 * @brief Defines the TCP option of kind 4 (selective acknowledgment permitted
 * option) as in \RFC{2018}
 *
 * TCP Sack-Permitted Option is 2-byte in length and sent in a SYN segment by a
 * TCP host that can recognize and process SACK option during the lifetime of a
 * connection.
 */

class TcpOptionSackPermitted : public TcpOption
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpOptionSackPermitted();
    ~TcpOptionSackPermitted() override;

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;
};

} // namespace ns3

#endif /* TCP_OPTION_SACK_PERMITTED */
