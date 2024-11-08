/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef SUPPORTED_RATES_H
#define SUPPORTED_RATES_H

#include "wifi-information-element.h"

#include <optional>
#include <vector>

namespace ns3
{

/**
 * @brief The Supported Rates Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise the Supported
 * Rates Element that holds the first 8 (non-HT) supported rates.
 *
 * The \c ExtendedSupportedRatesIE class deals with rates beyond the first 8.
 */
class SupportedRates : public WifiInformationElement
{
    friend struct AllSupportedRates;

  public:
    SupportedRates();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    /**
     * Return the rate (converted back to raw value) at the given index.
     *
     * @param i the given index
     * @return the rate in bps
     */
    uint32_t GetRate(uint8_t i) const;

  protected:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    std::vector<uint8_t> m_rates; //!< List of supported bit rates (divided by 500000)
};

/**
 * @brief The Extended Supported Rates Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise the Extended
 * Supported Rates Element that holds (non-HT) rates beyond the 8 that
 * the original Supported Rates element can carry.
 */
class ExtendedSupportedRatesIE : public SupportedRates
{
  public:
    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
};

/**
 * @brief Struct containing all supported rates.
 * @ingroup wifi
 *
 */
struct AllSupportedRates
{
    /**
     * Add the given rate to the supported rates.
     *
     * @param bs the rate to be added in bps
     */
    void AddSupportedRate(uint64_t bs);
    /**
     * Set the given rate to basic rates.
     *
     * @param bs the rate to be set in bps
     */
    void SetBasicRate(uint64_t bs);
    /**
     * Add a special value to the supported rate set, corresponding to
     * a BSS membership selector
     *
     * @param bs the special membership selector value (not a valid rate)
     */
    void AddBssMembershipSelectorRate(uint64_t bs);
    /**
     * Check if the given rate is supported.  The rate is encoded as it is
     * serialized to the Supported Rates Information Element (i.e. as a
     * multiple of 500 Kbits/sec, possibly with MSB set to 1).
     *
     * @param bs the rate to be checked in bps
     *
     * @return true if the rate is supported, false otherwise
     */
    bool IsSupportedRate(uint64_t bs) const;
    /**
     * Check if the given rate is a basic rate.  The rate is encoded as it is
     * serialized to the Supported Rates Information Element (i.e. as a
     * multiple of 500 Kbits/sec, with MSB set to 1).
     *
     * @param bs the rate to be checked in bps
     *
     * @return true if the rate is a basic rate, false otherwise
     */
    bool IsBasicRate(uint64_t bs) const;
    /**
     * Check if the given rate is a BSS membership selector value.  The rate
     * is encoded as it is serialized to the Supporting Rates Information
     * Element (i.e. with the MSB set to 1).
     *
     * @param bs the rate to be checked in bps
     *
     * @return true if the rate is a BSS membership selector, false otherwise
     */
    bool IsBssMembershipSelectorRate(uint64_t bs) const;
    /**
     * Return the number of supported rates.
     *
     * @return the number of supported rates
     */
    uint8_t GetNRates() const;

    SupportedRates rates;                                  //!< supported rates
    std::optional<ExtendedSupportedRatesIE> extendedRates; //!< supported extended rates
};

} // namespace ns3

#endif /* SUPPORTED_RATES_H */
