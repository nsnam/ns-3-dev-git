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
#include "ns3/wifi-phy-band.h"

namespace ns3
{

class ApWifiMac;
class EhtFrameExchangeManager;
class Time;
class WifiPsdu;
class WifiTxVector;

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

    /**
     * This method is called when the reception of a PSDU succeeds on the given link.
     *
     * \param linkId the ID of the given link
     * \param psdu the PSDU whose reception succeeded
     */
    virtual void NotifyPsduRxOk(uint8_t linkId, Ptr<const WifiPsdu> psdu);

    /**
     * This method is called when the reception of a PSDU fails on the given link.
     *
     * \param linkId the ID of the given link
     * \param psdu the PSDU whose reception failed
     */
    virtual void NotifyPsduRxError(uint8_t linkId, Ptr<const WifiPsdu> psdu);

    /**
     * This method is intended to be called when the AP MLD starts transmitting an SU frame that
     * is not addressed to EMLSR clients that were previously involved in the ongoing DL TXOP.
     *
     * \param psdu the PSDU being transmitted
     * \param txVector the TXVECTOR used to transmit the PSDU
     * \param band the PHY band in which the PSDU is being transmitted
     * \return the delay after which the AP MLD starts the transition delay for the EMLSR client
     */
    virtual Time GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                             const WifiTxVector& txVector,
                                             WifiPhyBand band) = 0;

  protected:
    void DoDispose() override;

    /**
     * Allow subclasses to take actions when the MAC is set.
     *
     * \param mac the wifi MAC
     */
    virtual void DoSetWifiMac(Ptr<ApWifiMac> mac);

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
