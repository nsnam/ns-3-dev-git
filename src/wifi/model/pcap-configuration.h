/*
 * Copyright (c) 2026 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef PCAP_CONFIGURATION_H
#define PCAP_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3
{

/**
 * @brief PCAP configuration
 * @ingroup wifi
 *
 * This object stores PCAP configuration information.
 *
 */
class PcapConfiguration : public Object
{
  public:
    PcapConfiguration();
    ~PcapConfiguration() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Get whether Radiotap timestamp option is activated
     *
     * @return true if Radiotap timestamp option is activated, false otherwise
     */
    bool GetRadiotapTs() const;

  private:
    bool m_radiotapTs; ///< Radiotap timestamp option
};

} // namespace ns3

#endif /* PCAP_CONFIGURATION_H */
