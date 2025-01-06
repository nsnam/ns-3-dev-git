/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_CONFIGURATION_H
#define UHR_CONFIGURATION_H

#include "ns3/object.h"

namespace ns3
{

/**
 * @brief UHR configuration
 * @ingroup wifi
 *
 * This object stores UHR configuration information, for use in modifying
 * AP or STA behavior and for constructing UHR-related information elements.
 *
 */
class UhrConfiguration : public Object
{
  public:
    UhrConfiguration();
    ~UhrConfiguration() override;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Get whether DSO option is activated
     *
     * @return true if DSO option is activated, false otherwise
     */
    bool GetDsoActivated() const;

  private:
    bool m_dsoActivated; //!< whether DSO option is activated
};

} // namespace ns3

#endif /* UHR_CONFIGURATION_H */
