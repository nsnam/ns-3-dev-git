/*
 * Copyright (c) 2023 Tokushima University, Japan.
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
 *  Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "lr-wpan-mac-base.h"

#include <ns3/log.h>
#include <ns3/uinteger.h>

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("LrWpanMacBase");
NS_OBJECT_ENSURE_REGISTERED(LrWpanMacBase);

TypeId
LrWpanMacBase::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LrWpanMacBase").SetParent<Object>().SetGroupName("LrWpan");
    return tid;
}

LrWpanMacBase::~LrWpanMacBase()
{
}

void
LrWpanMacBase::SetMcpsDataConfirmCallback(McpsDataConfirmCallback c)
{
    m_mcpsDataConfirmCallback = c;
}

void
LrWpanMacBase::SetMcpsDataIndicationCallback(McpsDataIndicationCallback c)
{
    m_mcpsDataIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeAssociateIndicationCallback(MlmeAssociateIndicationCallback c)
{
    m_mlmeAssociateIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeCommStatusIndicationCallback(MlmeCommStatusIndicationCallback c)
{
    m_mlmeCommStatusIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeOrphanIndicationCallback(MlmeOrphanIndicationCallback c)
{
    m_mlmeOrphanIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeStartConfirmCallback(MlmeStartConfirmCallback c)
{
    m_mlmeStartConfirmCallback = c;
}

void
LrWpanMacBase::SetMlmeScanConfirmCallback(MlmeScanConfirmCallback c)
{
    m_mlmeScanConfirmCallback = c;
}

void
LrWpanMacBase::SetMlmeAssociateConfirmCallback(MlmeAssociateConfirmCallback c)
{
    m_mlmeAssociateConfirmCallback = c;
}

void
LrWpanMacBase::SetMlmeBeaconNotifyIndicationCallback(MlmeBeaconNotifyIndicationCallback c)
{
    m_mlmeBeaconNotifyIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeSyncLossIndicationCallback(MlmeSyncLossIndicationCallback c)
{
    m_mlmeSyncLossIndicationCallback = c;
}

void
LrWpanMacBase::SetMlmeSetConfirmCallback(MlmeSetConfirmCallback c)
{
    m_mlmeSetConfirmCallback = c;
}

void
LrWpanMacBase::SetMlmeGetConfirmCallback(MlmeGetConfirmCallback c)
{
    m_mlmeGetConfirmCallback = c;
}

void
LrWpanMacBase::SetMlmePollConfirmCallback(MlmePollConfirmCallback c)
{
    m_mlmePollConfirmCallback = c;
}

} // namespace lrwpan
} // namespace ns3
