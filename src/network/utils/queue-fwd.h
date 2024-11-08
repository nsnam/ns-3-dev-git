/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef QUEUE_FWD_H
#define QUEUE_FWD_H

#include "ns3/ptr.h"

#include <list>

/**
 * @file
 * @ingroup queue
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
