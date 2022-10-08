/*
 * Copyright (c) 2018  Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef VHT_CONFIGURATION_H
#define VHT_CONFIGURATION_H

#include "ns3/object.h"

#include <map>
#include <tuple>

namespace ns3
{

/**
 * \brief VHT configuration
 * \ingroup wifi
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
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Enable or disable 160 MHz operation support.
     *
     * \param enable true if 20 MHz, 40 MHz, 80 MHz and 160 MHz operation is to be supported,
     *               false if 20 MHz, 40 MHz and 80 MHz operation is to be supported
     */
    void Set160MHzOperationSupported(bool enable);
    /**
     * \return true if 20 MHz, 40 MHz, 80 MHz and 160 MHz operation is supported,
     *         false if 20 MHz, 40 MHz and 80 MHz operation is supported
     */
    bool Get160MHzOperationSupported() const;

    using SecondaryCcaSensitivityThresholds =
        std::tuple<double, double, double>; //!< Tuple identifying CCA sensitivity thresholds for
                                            //!< secondary channels

    /**
     * Sets the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel.
     * The thresholds are defined as a tuple {threshold for 20MHz PPDUs,
     * threshold for 40MHz PPDUs, threshold for 80MHz PPDUs}.
     *
     * \param thresholds the CCA sensitivity thresholds
     */
    void SetSecondaryCcaSensitivityThresholds(const SecondaryCcaSensitivityThresholds& thresholds);
    /**
     * \return the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel
     */
    SecondaryCcaSensitivityThresholds GetSecondaryCcaSensitivityThresholds() const;

    /**
     * \return the CCA sensitivity thresholds for PPDUs that do not occupy the primary channel,
     * indexed by signal bandwidth (MHz)
     */
    const std::map<uint16_t, double>& GetSecondaryCcaSensitivityThresholdsPerBw() const;

  private:
    bool m_160MHzSupported; ///< whether 160 MHz operation is supported
    std::map<uint16_t, double>
        m_secondaryCcaSensitivityThresholds; ///< CCA sensitivity thresholds for signals that do not
                                             ///< occupy the primary channel, indexed by signal
                                             ///< bandwidth (MHz)
};

} // namespace ns3

#endif /* VHT_CONFIGURATION_H */
