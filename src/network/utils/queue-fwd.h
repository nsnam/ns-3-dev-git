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
 */

#ifndef QUEUE_FWD_H
#define QUEUE_FWD_H

#include "ns3/ptr.h"

#include <list>

/**
 * \file
 * \ingroup queue
 * Forward declaration of template class Queue.
 */

namespace ns3
{

// Forward declaration of template class Queue specifying
// the default value for the template template parameter Container
template <typename Item, typename Container = std::list<Ptr<Item>>>
class Queue;

} // namespace ns3

#endif /* QUEUE_FWD_H */
