/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef DEFAULT_DSO_MANAGER_H
#define DEFAULT_DSO_MANAGER_H

#include "dso-manager.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * Default DSO manager.
 */
class DefaultDsoManager : public DsoManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DefaultDsoManager();
    ~DefaultDsoManager() override;

    Time GetSwitchingDelayToDsoSubband() const override;

  private:
    Time m_chSwitchToDsoBandDelay; //!< Delay to switch the channel to the DSO subband
};

} // namespace ns3

#endif /* DEFAULT_DSO_MANAGER_H */
