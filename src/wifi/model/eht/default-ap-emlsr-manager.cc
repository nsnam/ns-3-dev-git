/*
 * Copyright (c) 2023 Universita' di Napoli Federico II
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

#include "default-ap-emlsr-manager.h"

#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultApEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultApEmlsrManager);

TypeId
DefaultApEmlsrManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DefaultApEmlsrManager")
                            .SetParent<ApEmlsrManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultApEmlsrManager>();
    return tid;
}

DefaultApEmlsrManager::DefaultApEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultApEmlsrManager::~DefaultApEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

Time
DefaultApEmlsrManager::GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                                   const WifiTxVector& txVector,
                                                   WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << psdu << txVector << band);
    // EMLSR clients switch back to listening operation at the end of the PPDU
    return WifiPhy::CalculateTxDuration(psdu, txVector, band);
}

} // namespace ns3
