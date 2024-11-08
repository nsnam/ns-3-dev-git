/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H

#include "ns3/ptr.h"

#include <vector>

namespace ns3
{

class Channel;
class CallbackBase;

/**
 * @ingroup network
 *
 * @brief the list of simulation channels.
 *
 * Every Channel created is automatically added to this list.
 */
class ChannelList
{
  public:
    /// Channel container iterator
    typedef std::vector<Ptr<Channel>>::const_iterator Iterator;

    /**
     * @param channel channel to add
     * @returns index of channel in list.
     *
     * This method is called automatically from Channel::Channel so
     * the user has little reason to call it himself.
     */
    static uint32_t Add(Ptr<Channel> channel);
    /**
     * @returns a C++ iterator located at the beginning of this
     *          list.
     */
    static Iterator Begin();
    /**
     * @returns a C++ iterator located at the end of this
     *          list.
     */
    static Iterator End();
    /**
     * @param n index of requested channel.
     * @returns the Channel associated to index n.
     */
    static Ptr<Channel> GetChannel(uint32_t n);
    /**
     * @returns the number of channels currently in the list.
     */
    static uint32_t GetNChannels();
};

} // namespace ns3

#endif /* CHANNEL_LIST_H */
