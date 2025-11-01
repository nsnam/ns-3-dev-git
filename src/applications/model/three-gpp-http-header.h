/*
 * Copyright (c) 2015 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef THREE_GPP_HTTP_HEADER_H
#define THREE_GPP_HTTP_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3
{

class Packet;

/**
 * @ingroup http
 * @brief Header used by web browsing applications to transmit information about
 *        content type, content length and timestamps for delay statistics.
 *
 * The header contains the following fields (and their respective size when
 * serialized):
 *   - content type (2 bytes);
 *   - content length (4 bytes);
 *   - client time stamp (8 bytes); and
 *   - server time stamp (8 bytes).
 *
 * The header is attached to every packet transmitted by ThreeGppHttpClient and
 * ThreeGppHttpServer applications. In received, split packets, only the first packet
 * of transmitted object contains the header, which helps to identify how many bytes are
 * left to be received.
 *
 * The last 2 fields allow the applications to compute the propagation delay of
 * each packet. The *client TS* field indicates the time when the request
 * packet is sent by the ThreeGppHttpClient, while the *server TS* field indicates the
 * time when the response packet is sent by the ThreeGppHttpServer.
 */
class ThreeGppHttpHeader : public Header
{
  public:
    /// Creates an empty instance.
    ThreeGppHttpHeader();

    /**
     * Returns the object TypeId.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    // Inherited from ObjectBase base class.
    TypeId GetInstanceTypeId() const override;

    // Inherited from Header base class.
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * @return The string representation of the header.
     */
    std::string ToString() const;

    /// The possible types of content (default = NOT_SET).
    enum ContentType_t
    {
        NOT_SET,        ///< Integer equivalent = 0.
        MAIN_OBJECT,    ///< Integer equivalent = 1.
        EMBEDDED_OBJECT ///< Integer equivalent = 2.
    };

    /**
     * @param contentType The content type.
     */
    void SetContentType(ContentType_t contentType);

    /**
     * @return The content type.
     */
    ContentType_t GetContentType() const;

    /**
     * @param contentLength The content length (in bytes).
     */
    void SetContentLength(uint32_t contentLength);

    /**
     * @return The content length (in bytes).
     */
    uint32_t GetContentLength() const;

    /**
     * @param clientTs The client time stamp.
     */
    void SetClientTs(Time clientTs);

    /**
     * @return The client time stamp.
     */
    Time GetClientTs() const;

    /**
     * @param serverTs The server time stamp.
     */
    void SetServerTs(Time serverTs);

    /**
     * @return The server time stamp.
     */
    Time GetServerTs() const;

  private:
    uint16_t m_contentType{0};   //!< Content type field in integer format.
    uint32_t m_contentLength{0}; //!< Content length field (in bytes unit).
    uint64_t m_clientTs{0};      //!< Client time stamp field (in time step unit).
    uint64_t m_serverTs{0};      //!< Server time stamp field (in time step unit).
};

} // namespace ns3

#endif /* THREE_GPP_HTTP_HEADER_H */
