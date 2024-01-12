/*
 * Copyright (c) 2009 University of Washington
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

#include "channel-list.h"

#include "channel.h"

#include "ns3/assert.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/object-vector.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ChannelList");

/**
 * \ingroup network
 *
 * \brief private implementation detail of the ChannelList API.
 */
class ChannelListPriv : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    ChannelListPriv();
    ~ChannelListPriv() override;

    /**
     * \param channel channel to add
     * \returns index of channel in list.
     *
     * This method is called automatically from Channel::Channel so
     * the user has little reason to call it himself.
     */
    uint32_t Add(Ptr<Channel> channel);

    /**
     * \returns a C++ iterator located at the beginning of this
     *          list.
     */
    ChannelList::Iterator Begin() const;
    /**
     * \returns a C++ iterator located at the end of this
     *          list.
     */
    ChannelList::Iterator End() const;

    /**
     * \param n index of requested channel.
     * \returns the Channel associated to index n.
     */
    Ptr<Channel> GetChannel(uint32_t n);

    /**
     * \returns the number of channels currently in the list.
     */
    uint32_t GetNChannels();

    /**
     * \brief Get the channel list object
     * \returns the channel list
     */
    static Ptr<ChannelListPriv> Get();

  private:
    /**
     * \brief Get the channel list object
     * \returns the channel list
     */
    static Ptr<ChannelListPriv>* DoGet();

    /**
     * \brief Delete the channel list object
     */
    static void Delete();

    /**
     * \brief Dispose the channels in the list
     */
    void DoDispose() override;

    std::vector<Ptr<Channel>> m_channels; //!< channel objects container
};

NS_OBJECT_ENSURE_REGISTERED(ChannelListPriv);

TypeId
ChannelListPriv::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ChannelListPriv")
                            .SetParent<Object>()
                            .SetGroupName("Network")
                            .AddAttribute("ChannelList",
                                          "The list of all channels created during the simulation.",
                                          ObjectVectorValue(),
                                          MakeObjectVectorAccessor(&ChannelListPriv::m_channels),
                                          MakeObjectVectorChecker<Channel>());
    return tid;
}

Ptr<ChannelListPriv>
ChannelListPriv::Get()
{
    NS_LOG_FUNCTION_NOARGS();
    return *DoGet();
}

Ptr<ChannelListPriv>*
ChannelListPriv::DoGet()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ptr<ChannelListPriv> ptr = nullptr;
    if (!ptr)
    {
        ptr = CreateObject<ChannelListPriv>();
        Config::RegisterRootNamespaceObject(ptr);
        Simulator::ScheduleDestroy(&ChannelListPriv::Delete);
    }
    return &ptr;
}

void
ChannelListPriv::Delete()
{
    NS_LOG_FUNCTION_NOARGS();
    Config::UnregisterRootNamespaceObject(Get());
    (*DoGet()) = nullptr;
}

ChannelListPriv::ChannelListPriv()
{
    NS_LOG_FUNCTION(this);
}

ChannelListPriv::~ChannelListPriv()
{
    NS_LOG_FUNCTION(this);
}

void
ChannelListPriv::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto i = m_channels.begin(); i != m_channels.end(); i++)
    {
        Ptr<Channel> channel = *i;
        channel->Dispose();
        *i = nullptr;
    }
    m_channels.erase(m_channels.begin(), m_channels.end());
    Object::DoDispose();
}

uint32_t
ChannelListPriv::Add(Ptr<Channel> channel)
{
    NS_LOG_FUNCTION(this << channel);
    uint32_t index = m_channels.size();
    m_channels.push_back(channel);
    Simulator::Schedule(TimeStep(0), &Channel::Initialize, channel);
    return index;
}

ChannelList::Iterator
ChannelListPriv::Begin() const
{
    NS_LOG_FUNCTION(this);
    return m_channels.begin();
}

ChannelList::Iterator
ChannelListPriv::End() const
{
    NS_LOG_FUNCTION(this);
    return m_channels.end();
}

uint32_t
ChannelListPriv::GetNChannels()
{
    NS_LOG_FUNCTION(this);
    return m_channels.size();
}

Ptr<Channel>
ChannelListPriv::GetChannel(uint32_t n)
{
    NS_LOG_FUNCTION(this << n);
    NS_ASSERT_MSG(n < m_channels.size(),
                  "Channel index " << n << " is out of range (only have " << m_channels.size()
                                   << " channels).");
    return m_channels[n];
}

uint32_t
ChannelList::Add(Ptr<Channel> channel)
{
    NS_LOG_FUNCTION_NOARGS();
    return ChannelListPriv::Get()->Add(channel);
}

ChannelList::Iterator
ChannelList::Begin()
{
    NS_LOG_FUNCTION_NOARGS();
    return ChannelListPriv::Get()->Begin();
}

ChannelList::Iterator
ChannelList::End()
{
    NS_LOG_FUNCTION_NOARGS();
    return ChannelListPriv::Get()->End();
}

Ptr<Channel>
ChannelList::GetChannel(uint32_t n)
{
    NS_LOG_FUNCTION(n);
    return ChannelListPriv::Get()->GetChannel(n);
}

uint32_t
ChannelList::GetNChannels()
{
    NS_LOG_FUNCTION_NOARGS();
    return ChannelListPriv::Get()->GetNChannels();
}

} // namespace ns3
