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

#ifndef AP_EMLSR_MANAGER_H
#define AP_EMLSR_MANAGER_H

#include "ns3/object.h"

namespace ns3
{

class ApWifiMac;
class EhtFrameExchangeManager;

/**
 * \ingroup wifi
 *
 * ApEmlsrManager is an abstract base class defining the API that EHT AP MLDs with
 * EMLSR activated can use to handle the operations on the EMLSR links of EMLSR clients
 */
class ApEmlsrManager : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    ApEmlsrManager();
    ~ApEmlsrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an EHT AP MLD.
     *
     * \param mac the wifi MAC
     */
    void SetWifiMac(Ptr<ApWifiMac> mac);

  protected:
    void DoDispose() override;

    /**
     * \return the MAC of the AP MLD managed by this AP EMLSR Manager.
     */
    Ptr<ApWifiMac> GetApMac() const;

    /**
     * \param linkId the ID of the given link
     * \return the EHT FrameExchangeManager attached to the AP operating on the given link
     */
    Ptr<EhtFrameExchangeManager> GetEhtFem(uint8_t linkId) const;

  private:
    Ptr<ApWifiMac> m_apMac; //!< the MAC of the managed AP MLD
};

} // namespace ns3

#endif /* AP_EMLSR_MANAGER_H */
