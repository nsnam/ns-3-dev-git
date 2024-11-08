/*
 * Copyright (c) 2018  Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef VHT_CONFIGURATION_H
#define VHT_CONFIGURATION_H

#include "ns3/deprecated.h"
#include "ns3/object.h"
#include "ns3/wifi-units.h"

#include <map>
#include <tuple>

namespace ns3
{

/**
 * @brief VHT configuration
 * @ingroup wifi
 *
 * This object stores VHT configuration information, for use in modifying
 * AP or STA behavior and for constructing VHT-related information elements.
 *
 */
class VhtConfiguration : public Object
{
  public:
    VhtConfiguration();
    ~VhtConfiguration() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Enable or disable 160 MHz operation support.
     *
     * @param enable true if 20 MHz, 40 MHz, 80 MHz and 160 MHz operation is to be supported,
     *               false if 20 MHz, 40 MHz and 80 MHz operation is to be supported
     */
    NS_DEPRECATED_3_44("Set the m_160MHzSupported member variable instead")
    void Set160MHzOperationSupported(bool enable);

    /**
     * @return true if 20 MHz, 40 MHz, 80 MHz and 160 MHz operation is supported,
     *         false if 20 MHz, 40 MHz and 80 MHz operation is supported
     */
    NS_DEPRECATED_3_44("Get the m_160MHzSupported member variable instead")
    bool Get160MHzOperationSupported() const;

    using SecondaryCcaSensitivityThresholds =
        std::tuple<dBm_u, dBm_u, dBm_u>; //!< Tuple identifying CCA sensitivity thresholds for
                                         //!< secondary channels

    /**
     * Sets the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel.
     * The thresholds are defined as a tuple {threshold for 20MHz PPDUs,
     * threshold for 40MHz PPDUs, threshold for 80MHz PPDUs}.
     *
     * @param thresholds the CCA sensitivity thresholds
     */
    void SetSecondaryCcaSensitivityThresholds(const SecondaryCcaSensitivityThresholds& thresholds);
    /**
     * @return the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel
     */
    SecondaryCcaSensitivityThresholds GetSecondaryCcaSensitivityThresholds() const;

    /**
     * @return the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel,
     * indexed by signal bandwidth
     */
    const std::map<MHz_u, dBm_u>& GetSecondaryCcaSensitivityThresholdsPerBw() const;

    bool m_160MHzSupported; ///< whether 160 MHz operation is supported

  private:
    std::map<MHz_u, dBm_u>
        m_secondaryCcaSensitivityThresholds; ///< CCA sensitivity thresholds for signals that do not
                                             ///< occupy the primary channel, indexed by signal
                                             ///< bandwidth
};

} // namespace ns3

#endif /* VHT_CONFIGURATION_H */
