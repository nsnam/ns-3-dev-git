/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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

#include "wifi-mac-queue-elem.h"

#include "wifi-mpdu.h"

namespace ns3
{

WifiMacQueueElem::WifiMacQueueElem(Ptr<WifiMpdu> item)
    : mpdu(item),
      expiryTime(0),
      ac(AC_UNDEF),
      expired(false)
{
}

WifiMacQueueElem::~WifiMacQueueElem()
{
    deleter(mpdu);
    inflights.clear();
}

} // namespace ns3
