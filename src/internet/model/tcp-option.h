/*
 * Copyright (c) 2011 Adrian Sai-wah Tam
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#ifndef TCP_OPTION_H
#define TCP_OPTION_H

#include "ns3/buffer.h"
#include "ns3/object-factory.h"
#include "ns3/object.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup tcp
 *
 * Base class for all kinds of TCP options
 */
class TcpOption : public Object
{
  public:
    TcpOption();
    ~TcpOption() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * The option Kind, as defined in the respective RFCs.
     */
    enum Kind
    {
        // Remember to extend IsKindKnown() with new value, when adding values here
        //
        END = 0,           //!< END
        NOP = 1,           //!< NOP
        MSS = 2,           //!< MSS
        WINSCALE = 3,      //!< WINSCALE
        SACKPERMITTED = 4, //!< SACKPERMITTED
        SACK = 5,          //!< SACK
        TS = 8,            //!< TS
        UNKNOWN = 255      //!< not a standardized value; for unknown recv'd options
    };

    /**
     * @brief Print the Option contents
     * @param os the output stream
     */
    virtual void Print(std::ostream& os) const = 0;
    /**
     * @brief Serialize the Option to a buffer iterator
     * @param start the buffer iterator
     */
    virtual void Serialize(Buffer::Iterator start) const = 0;

    /**
     * @brief Deserialize the Option from a buffer iterator
     * @param start the buffer iterator
     * @returns the number of deserialized bytes
     */
    virtual uint32_t Deserialize(Buffer::Iterator start) = 0;

    /**
     * @brief Get the `kind` (as in \RFC{793}) of this option
     * @return the Option Kind
     */
    virtual uint8_t GetKind() const = 0;
    /**
     * @brief Returns number of bytes required for Option
     * serialization.
     *
     * @returns number of bytes required for Option
     * serialization
     */
    virtual uint32_t GetSerializedSize() const = 0;

    /**
     * @brief Creates an option
     * @param kind the option kind
     * @return the requested option or an ns3::UnknownOption if the option is not supported
     */
    static Ptr<TcpOption> CreateOption(uint8_t kind);

    /**
     * @brief Check if the option is implemented
     * @param kind the Option kind
     * @return true if the option is known
     */
    static bool IsKindKnown(uint8_t kind);
};

/**
 * @ingroup tcp
 *
 * @brief An unknown TCP option.
 *
 * An unknown option can be deserialized and (only if deserialized previously)
 * serialized again.
 */
class TcpOptionUnknown : public TcpOption
{
  public:
    TcpOptionUnknown();
    ~TcpOptionUnknown() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

  private:
    uint8_t m_kind;        //!< The unknown option kind
    uint32_t m_size;       //!< The unknown option size
    uint8_t m_content[40]; //!< The option data
};

} // namespace ns3

#endif /* TCP_OPTION */
