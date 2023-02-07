/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#ifndef EMLSR_MANAGER_H
#define EMLSR_MANAGER_H

#include "ns3/object.h"
#include "ns3/sta-wifi-mac.h"

namespace ns3
{

class EhtFrameExchangeManager;

/**
 * \ingroup wifi
 *
 * EmlsrManager is an abstract base class defining the API that EHT non-AP MLDs
 * with EMLSR activated can use to handle the operations on the EMLSR links
 */
class EmlsrManager : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    EmlsrManager();
    ~EmlsrManager() override;

    /**
     * Set the wifi MAC. Note that it must be the MAC of an EHT non-AP MLD.
     *
     * \param mac the wifi MAC
     */
    void SetWifiMac(Ptr<StaWifiMac> mac);

    /**
     * Set the Transition Timeout advertised by the associated AP with EMLSR activated.
     *
     * \param timeout the advertised Transition Timeout
     */
    void SetTransitionTimeout(Time timeout);

    /**
     * \return the Transition Timeout, if advertised by the associated AP
     */
    std::optional<Time> GetTransitionTimeout() const;

  protected:
    void DoDispose() override;

    /**
     * \return the MAC of the non-AP MLD managed by this EMLSR Manager.
     */
    Ptr<StaWifiMac> GetStaMac() const;

    /**
     * \param linkId the ID of the given link
     * \return the EHT FrameExchangeManager attached to the non-AP STA operating on the given link
     */
    Ptr<EhtFrameExchangeManager> GetEhtFem(uint8_t linkId) const;

    Time m_emlsrPaddingDelay;    //!< EMLSR Padding delay
    Time m_emlsrTransitionDelay; //!< EMLSR Transition delay

  private:
    Ptr<StaWifiMac> m_staMac;                     //!< the MAC of the managed non-AP MLD
    std::optional<Time> m_emlsrTransitionTimeout; /**< Transition timeout advertised by APs with
                                                       EMLSR activated */
};

} // namespace ns3

#endif /* EMLSR_MANAGER_H */
