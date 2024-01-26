/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADVANCED_AP_EMLSR_MANAGER_H
#define ADVANCED_AP_EMLSR_MANAGER_H

#include "default-ap-emlsr-manager.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * AdvancedApEmlsrManager is an advanced AP EMLSR manager.
 */
class AdvancedApEmlsrManager : public DefaultApEmlsrManager
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    AdvancedApEmlsrManager();
    ~AdvancedApEmlsrManager() override;
};

} // namespace ns3

#endif /* ADVANCED_AP_EMLSR_MANAGER_H */
