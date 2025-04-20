/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_OPTIONS_H
#define DHCP6_OPTIONS_H

#include "dhcp6-duid.h"

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

/**
 * @ingroup dhcp6
 *
 * @class Options
 * @brief Implements the functionality of DHCPv6 options
 */
class Options
{
  public:
    /**
     * Enum to identify the status code of the operation.
     * These symbols and values are defined in [RFC 8415,
     * section 21.13](https://datatracker.ietf.org/doc/html/rfc8415#section-21.13)
     */
    enum class StatusCodeValues
    {
        Success = 0,
        UnspecFail = 1,
        NoAddrsAvail = 2,
        NoBinding = 3,
        NotOnLink = 4,
        UseMulticast = 5,
        NoPrefixAvail = 6,
    };

    /**
     * Enum to identify the option type.
     * These symbols and values are defined in [RFC 8415, section 21]
     * (https://datatracker.ietf.org/doc/html/rfc8415#section-21)
     */
    enum class OptionType
    {
        OPTION_INIT = 0, // Added for initialization
        OPTION_CLIENTID = 1,
        OPTION_SERVERID = 2,
        OPTION_IA_NA = 3,
        OPTION_IA_TA = 4,
        OPTION_IAADDR = 5,
        OPTION_ORO = 6,
        OPTION_PREFERENCE = 7,
        OPTION_ELAPSED_TIME = 8,
        OPTION_RELAY_MSG = 9,
        OPTION_AUTH = 11,
        OPTION_UNICAST = 12,
        OPTION_STATUS_CODE = 13,
        OPTION_RAPID_COMMIT = 14,
        OPTION_USER_CLASS = 15,
        OPTION_VENDOR_CLASS = 16,
        OPTION_VENDOR_OPTS = 17,
        OPTION_INTERFACE_ID = 18,
        OPTION_RECONF_MSG = 19,
        OPTION_RECONF_ACCEPT = 20,
        OPTION_IA_PD = 25,
        OPTION_IAPREFIX = 26,
        OPTION_INFORMATION_REFRESH_TIME = 32,
        OPTION_SOL_MAX_RT = 82,
        OPTION_INF_MAX_RT = 83,
    };

    /**
     * @brief Default constructor.
     */
    Options();

    /**
     * @brief Constructor.
     * @param code The option code.
     * @param length The option length.
     */
    Options(OptionType code, uint16_t length);

    /**
     * @brief Get the option code.
     * @return option code
     */
    OptionType GetOptionCode() const;

    /**
     * @brief Set the option code.
     * @param code The option code to be added.
     */
    void SetOptionCode(OptionType code);

    /**
     * @brief Get the option length.
     * @return option length
     */
    uint16_t GetOptionLength() const;

    /**
     * @brief Set the option length.
     * @param length The option length to be parsed.
     */
    void SetOptionLength(uint16_t length);

  private:
    OptionType m_optionCode; //!< Option code
    uint16_t m_optionLength; //!< Option length
};

/**
 * @ingroup dhcp6
 *
 * @class IdentifierOption
 * @brief Implements the client and server identifier options.
 */
class IdentifierOption : public Options
{
  public:
    /**
     * Default constructor.
     */
    IdentifierOption();

    /**
     * @brief Constructor.
     * @param hardwareType The hardware type.
     * @param linkLayerAddress The link-layer address.
     * @param time The time at which the DUID is generated.
     */
    IdentifierOption(uint16_t hardwareType, Address linkLayerAddress, Time time = Time());

    /**
     * @brief Set the DUID.
     * @param duid The DUID.
     */
    void SetDuid(Duid duid);

    /**
     * @brief Get the DUID object.
     * @return the DUID.
     */
    Duid GetDuid() const;

  private:
    Duid m_duid; //!< Unique identifier of the node.
};

/**
 * @ingroup dhcp6
 *
 * @class StatusCodeOption
 * @brief Implements the Status Code option.
 */
class StatusCodeOption : public Options
{
  public:
    /**
     * @brief Default constructor.
     */
    StatusCodeOption();

    /**
     * @brief Get the status code of the operation.
     * @return the status code.
     */
    StatusCodeValues GetStatusCode() const;

    /**
     * @brief Set the status code of the operation.
     * @param statusCode the status code of the performed operation.
     */
    void SetStatusCode(StatusCodeValues statusCode);

    /**
     * @brief Get the status message of the operation.
     * @return the status message
     */
    std::string GetStatusMessage() const;

    /**
     * @brief Set the status message of the operation.
     * @param statusMessage the status message of the operation.
     */
    void SetStatusMessage(std::string statusMessage);

  private:
    /**
     * The status code of an operation involving the IA_NA, IA_TA or
     * IA address.
     */
    StatusCodeValues m_statusCode;

    /**
     * The status message of the operation. This is to be UTF-8 encoded
     * as per RFC 3629.
     */
    std::string m_statusMessage;
};

/**
 * @ingroup dhcp6
 *
 * @class IaAddressOption
 * @brief Implements the IA Address options.
 */
class IaAddressOption : public Options
{
  public:
    /**
     * @brief Default constructor.
     */
    IaAddressOption();

    /**
     * @brief Constructor.
     * @param iaAddress The IA Address.
     * @param preferredLifetime The preferred lifetime of the address.
     * @param validLifetime The valid lifetime of the address.
     */
    IaAddressOption(Ipv6Address iaAddress, uint32_t preferredLifetime, uint32_t validLifetime);

    /**
     * @brief Get the IA Address.
     * @return the IPv6 address of the Identity Association
     */
    Ipv6Address GetIaAddress() const;

    /**
     * @brief Set the IA Address.
     * @param iaAddress the IPv6 address of this Identity Association.
     */
    void SetIaAddress(Ipv6Address iaAddress);

    /**
     * @brief Get the preferred lifetime.
     * @return the preferred lifetime
     */
    uint32_t GetPreferredLifetime() const;

    /**
     * @brief Set the preferred lifetime.
     * @param preferredLifetime the preferred lifetime for this address.
     */
    void SetPreferredLifetime(uint32_t preferredLifetime);

    /**
     * @brief Get the valid lifetime.
     * @return the lifetime for which the address is valid.
     */
    uint32_t GetValidLifetime() const;

    /**
     * @brief Set the valid lifetime.
     * @param validLifetime the lifetime for which the address is valid.
     */
    void SetValidLifetime(uint32_t validLifetime);

  private:
    Ipv6Address m_iaAddress;      //!< the IPv6 address offered to the client.
    uint32_t m_preferredLifetime; //!< The preferred lifetime of the address, in seconds.
    uint32_t m_validLifetime;     //!< The valid lifetime of the address, in seconds.

    /// (optional) The status code of any operation involving this address
    StatusCodeOption m_statusCodeOption;
};

/**
 * @ingroup dhcp6
 *
 * @class IaOptions
 * @brief Implements the IANA and IATA options.
 */
class IaOptions : public Options
{
  public:
    /**
     * @brief Default constructor.
     */
    IaOptions();

    /**
     * @brief Get the unique identifier for the given IANA or IATA.
     * @return the ID of the IANA or IATA
     */
    uint32_t GetIaid() const;

    /**
     * @brief Set the unique identifier for the given IANA or IATA.
     * @param iaid the unique ID for the IANA or IATA.
     */
    void SetIaid(uint32_t iaid);

    /**
     * @brief Get the time interval in seconds after which the client contacts
     * the server which provided the address to extend the lifetime.
     * @return the time interval T1
     */
    uint32_t GetT1() const;

    /**
     * @brief Set the time interval in seconds after which the client contacts
     * the server which provided the address to extend the lifetime.
     * @param t1 the time interval in seconds.
     */
    void SetT1(uint32_t t1);

    /**
     * @brief Get the time interval in seconds after which the client contacts
     * any available server to extend the address lifetime.
     * @return the time interval T2
     */
    uint32_t GetT2() const;

    /**
     * @brief Set the time interval in seconds after which the client contacts
     * any available server to extend the address lifetime.
     * @param t2 time interval in seconds.
     */
    void SetT2(uint32_t t2);

    /// The list of IA Address options associated with the IANA.
    std::vector<IaAddressOption> m_iaAddressOption;

  private:
    /// The unique identifier for the given IA_NA or IA_TA.
    uint32_t m_iaid;

    /**
     * The time interval in seconds after which the client contacts the
     * server which provided the address to extend the lifetime.
     */
    uint32_t m_t1;

    /**
     * The time interval in seconds after which the client contacts any
     * available server to extend the address lifetime.
     */
    uint32_t m_t2;

    /// (optional) The status code of any operation involving the IANA.
    StatusCodeOption m_statusCodeOption;
};

/**
 * @ingroup dhcp6
 *
 * @class RequestOptions
 * @brief Implements the Option Request option.
 */
class RequestOptions : public Options
{
  public:
    /**
     * @brief Constructor.
     */
    RequestOptions();

    /**
     * @brief Get the option values
     * @return requested option list.
     */
    std::vector<OptionType> GetRequestedOptions() const;

    /**
     * @brief Set the option values.
     * @param requestedOption option to be requested from the server.
     */
    void AddRequestedOption(OptionType requestedOption);

  private:
    std::vector<OptionType> m_requestedOptions; //!< List of requested options.
};

/**
 * @ingroup dhcp6
 *
 * @class IntegerOptions
 * @brief Implements the Preference and Elapsed Time options.
 */
template <typename T>
class IntegerOptions : public Options
{
  public:
    /**
     * @brief Constructor.
     */
    IntegerOptions();

    /**
     * @brief Get the option value
     * @return elapsed time or preference value.
     */
    T GetOptionValue() const;

    /**
     * @brief Set the option value.
     * @param optionValue elapsed time or preference value.
     */
    void SetOptionValue(T optionValue);

  private:
    /// Indicates the elapsed time or preference value.
    T m_optionValue;
};

/**
 * @ingroup dhcp6
 *
 * @class ServerUnicastOption
 * @brief Implements the Server Unicast option.
 */
class ServerUnicastOption : public Options
{
  public:
    ServerUnicastOption();

    /**
     * @brief Get the server address.
     * @return The 128 bit server address.
     */
    Ipv6Address GetServerAddress();

    /**
     * @brief Set the server address.
     * @param serverAddress the 128-bit server address.
     */
    void SetServerAddress(Ipv6Address serverAddress);

  private:
    /**
     * The 128-bit server address to which the client should send
     * unicast messages.
     */
    Ipv6Address m_serverAddress;
};

/**
 * @ingroup dhcp6
 * Create the typedef PreferenceOption with T as uint8_t
 */
typedef IntegerOptions<uint8_t> PreferenceOption;

/**
 * @ingroup dhcp6
 * Create the typedef ElapsedTimeOption with T as uint16_t
 */
typedef IntegerOptions<uint16_t> ElapsedTimeOption;

} // namespace ns3

#endif
