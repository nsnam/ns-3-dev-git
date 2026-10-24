/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ipv4-address-helper.h"

#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/simulation-singleton.h"
#include "ns3/simulator.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4AddressHelper");

/**
 * @ingroup address
 *
 * @brief IPv4 allocated address checker
 *
 * This class helps tracking allocated IPv4 addresses, ensuring that
 * no duplication arises.
 *
 * For testing purposes, or if a duplication is explicitly wanted,
 * it is possible to disable the fatal errors by using `AllowDuplicates`.
 */
class Ipv4AddressDuplicateChecker
{
  public:
    /**
     * @brief Add the Ipv4Address to the list of IPv4 entries
     *
     * Typically, this is used by external address allocators that want
     * to make use of this class's ability to track duplicates.
     *
     * @param addr The Ipv4Address to be added to the list of Ipv4 entries
     * @returns true on success
     */
    bool AddAllocated(const Ipv4Address addr);

    /**
     * @brief Check the Ipv4Address allocation in the list of IPv4 entries
     *
     * @param addr The Ipv4Address to be checked in the list of Ipv4 entries
     * @returns true if the address is already allocated
     */
    bool IsAddressAllocated(const Ipv4Address addr);

    /**
     * @brief Used to turn off fatal errors and assertions, for testing or special cases
     * @param allow allow duplicates
     */
    void AllowDuplicates(bool allow = true);

  private:
    /**
     * @brief This class holds the allocated addresses
     */
    class Entry
    {
      public:
        uint32_t addrLow;  //!< the lowest allocated address
        uint32_t addrHigh; //!< the highest allocated address
    };

    std::list<Entry> m_entries;    //!< contained of allocated addresses
    bool m_allowDuplicates{false}; //!< allow duplicates (if true)
};

bool
Ipv4AddressDuplicateChecker::AddAllocated(const Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);

    uint32_t addr = address.Get();

    NS_ABORT_MSG_UNLESS(
        addr,
        "Ipv4AddressDuplicateChecker::Add(): Allocating the broadcast address is not a good idea");

    std::list<Entry>::iterator i;

    for (i = m_entries.begin(); i != m_entries.end(); ++i)
    {
        NS_LOG_LOGIC("examine entry: " << Ipv4Address((*i).addrLow) << " to "
                                       << Ipv4Address((*i).addrHigh));
        //
        // First things first.  Is there an address collision -- that is, does the
        // new address fall in a previously allocated block of addresses.
        //
        if (addr >= (*i).addrLow && addr <= (*i).addrHigh)
        {
            NS_LOG_LOGIC(
                "Ipv4AddressDuplicateChecker::Add(): Address Collision: " << Ipv4Address(addr));
            if (!m_allowDuplicates)
            {
                NS_FATAL_ERROR(
                    "Ipv4AddressDuplicateChecker::Add(): Address Collision: " << Ipv4Address(addr));
            }
            return false;
        }
        //
        // If the new address is less than the lowest address in the current block,
        // and can't be merged into to the current block, then insert it as a new
        // block before the current block.
        //
        if (addr < (*i).addrLow - 1)
        {
            break;
        }
        //
        // If the new address fits at the end of the block, look ahead to the next
        // block and make sure it's not a collision there.  If we won't overlap, then
        // just extend the current block by one address.  We expect that completely
        // filled network ranges will be a fairly rare occurrence, so we don't worry
        // about collapsing address range blocks.
        //
        if (addr == (*i).addrHigh + 1)
        {
            auto j = i;
            ++j;

            if (j != m_entries.end())
            {
                if (addr == (*j).addrLow)
                {
                    NS_LOG_LOGIC("Ipv4AddressDuplicateChecker::Add(): "
                                 "Address Collision: "
                                 << Ipv4Address(addr));
                    if (!m_allowDuplicates)
                    {
                        NS_FATAL_ERROR("Ipv4AddressDuplicateChecker::Add(): Address Collision: "
                                       << Ipv4Address(addr));
                    }
                    return false;
                }
            }

            NS_LOG_LOGIC("New addrHigh = " << Ipv4Address(addr));
            (*i).addrHigh = addr;
            return true;
        }
        //
        // If we get here, we know that the next lower block of addresses couldn't
        // have been extended to include this new address since the code immediately
        // above would have been executed and that next lower block extended upward.
        // So we know it's safe to extend the current block down to include the new
        // address.
        //
        if (addr == (*i).addrLow - 1)
        {
            NS_LOG_LOGIC("New addrLow = " << Ipv4Address(addr));
            (*i).addrLow = addr;
            return true;
        }
    }

    Entry entry;
    entry.addrLow = entry.addrHigh = addr;
    m_entries.insert(i, entry);
    return true;
}

bool
Ipv4AddressDuplicateChecker::IsAddressAllocated(const Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);

    uint32_t addr = address.Get();

    NS_ABORT_MSG_UNLESS(addr,
                        "Ipv4AddressDuplicateChecker::IsAddressAllocated(): Don't check for the "
                        "broadcast address...");

    for (auto i = m_entries.begin(); i != m_entries.end(); ++i)
    {
        NS_LOG_LOGIC("examine entry: " << Ipv4Address((*i).addrLow) << " to "
                                       << Ipv4Address((*i).addrHigh));
        if (addr >= (*i).addrLow && addr <= (*i).addrHigh)
        {
            NS_LOG_LOGIC("Ipv4AddressDuplicateChecker::IsAddressAllocated(): Address Collision: "
                         << Ipv4Address(addr));
            return true;
        }
    }
    return false;
}

void
Ipv4AddressDuplicateChecker::AllowDuplicates(bool allow)
{
    NS_LOG_FUNCTION(this << allow);
    m_allowDuplicates = allow;
}

Ipv4AddressHelper::Ipv4AddressHelper()
{
    NS_LOG_FUNCTION_NOARGS();

    //
    // Set the default values to an illegal state.  Do this so the client is
    // forced to think at least briefly about what addresses get used and what
    // is going on here.
    //
    m_network = 0xffffffff;
    m_mask = 0;
    m_address = 0xffffffff;
    m_base = 0xffffffff;
    m_shift = 0;
    m_max = 0xffffffff;
}

Ipv4AddressHelper::Ipv4AddressHelper(const Ipv4Address network,
                                     const Ipv4Mask mask,
                                     const Ipv4Address address)
{
    NS_LOG_FUNCTION_NOARGS();
    SetBase(network, mask, address);
}

void
Ipv4AddressHelper::SetBase(const Ipv4Address network,
                           const Ipv4Mask mask,
                           const Ipv4Address address)
{
    NS_LOG_FUNCTION_NOARGS();

    m_network = network.Get();
    m_mask = mask.Get();
    m_base = m_address = address.Get();

    //
    // Some quick reasonableness testing.
    //
    NS_ASSERT_MSG((m_network & ~m_mask) == 0,
                  "Ipv4AddressHelper::SetBase(): Inconsistent network and mask");

    //
    // Figure out how much to shift network numbers to get them aligned, and what
    // the maximum allowed address is with respect to the current mask.
    //
    m_shift = NumAddressBits(m_mask);
    m_max = (1 << m_shift) - 2;

    NS_ASSERT_MSG(m_shift <= 32, "Ipv4AddressHelper::SetBase(): Unreasonable address length");

    //
    // Shift the network down into the normalized position.
    //
    m_network >>= m_shift;

    NS_LOG_LOGIC("m_network == " << m_network);
    NS_LOG_LOGIC("m_mask == " << m_mask);
    NS_LOG_LOGIC("m_address == " << m_address);
}

Ipv4Address
Ipv4AddressHelper::NewAddress()
{
    //
    // The way this is expected to be used is that an address and network number
    // are initialized, and then NewAddress() is called repeatedly to allocate and
    // get new addresses on a given subnet.  The client will expect that the first
    // address she gets back is the one she used to initialize the generator with.
    // This implies that this operation is a post-increment.
    //
    NS_ASSERT_MSG(m_address <= m_max, "Ipv4AddressHelper::NewAddress(): Address overflow");

    Ipv4Address addr((m_network << m_shift) | m_address);
    ++m_address;
    //
    // The Ipv4AddressDuplicateChecker allows us to keep track of the addresses we have
    // allocated and will assert if we accidentally generate a duplicate.  This
    // avoids some really hard to debug problems.
    //
    SimulationSingleton<Ipv4AddressDuplicateChecker>::Get()->AddAllocated(addr);
    return addr;
}

Ipv4Address
Ipv4AddressHelper::NewNetwork()
{
    NS_LOG_FUNCTION_NOARGS();
    ++m_network;
    m_address = m_base;
    return Ipv4Address(m_network << m_shift);
}

Ipv4InterfaceContainer
Ipv4AddressHelper::Assign(const NetDeviceContainer& c)
{
    NS_LOG_FUNCTION_NOARGS();
    Ipv4InterfaceContainer retval;
    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        Ptr<NetDevice> device = c.Get(i);

        Ptr<Node> node = device->GetNode();
        NS_ASSERT_MSG(node,
                      "Ipv4AddressHelper::Assign(): NetDevice is not not associated "
                      "with any node -> fail");

        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        NS_ASSERT_MSG(ipv4,
                      "Ipv4AddressHelper::Assign(): NetDevice is associated"
                      " with a node without IPv4 stack installed -> fail "
                      "(maybe need to use InternetStackHelper?)");

        int32_t interface = ipv4->GetInterfaceForDevice(device);
        if (interface == -1)
        {
            interface = ipv4->AddInterface(device);
        }
        NS_ASSERT_MSG(interface >= 0,
                      "Ipv4AddressHelper::Assign(): "
                      "Interface index not found");

        Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress(NewAddress(), m_mask);
        ipv4->AddAddress(interface, ipv4Addr);
        ipv4->SetMetric(interface, 1);
        ipv4->SetUp(interface);
        retval.Add(ipv4, interface);

        // Install the default traffic control configuration if the traffic
        // control layer has been aggregated, if this is not
        // a loopback interface, and there is no queue disc installed already
        Ptr<TrafficControlLayer> tc = node->GetObject<TrafficControlLayer>();
        if (tc && !DynamicCast<LoopbackNetDevice>(device) && !tc->GetRootQueueDiscOnDevice(device))
        {
            Ptr<NetDeviceQueueInterface> ndqi = device->GetObject<NetDeviceQueueInterface>();
            // It is useless to install a queue disc if the device has no
            // NetDeviceQueueInterface attached: the device queue is never
            // stopped and every packet enqueued in the queue disc is
            // immediately dequeued, hence there will never be backlog
            if (ndqi)
            {
                std::size_t nTxQueues = ndqi->GetNTxQueues();
                NS_LOG_LOGIC("Installing default traffic control configuration ("
                             << nTxQueues << " device queue(s))");
                TrafficControlHelper tcHelper = TrafficControlHelper::Default(nTxQueues);
                tcHelper.Install(device);
            }
        }
    }
    return retval;
}

const uint32_t N_BITS = 32; //!< number of bits in a IPv4 address

uint32_t
Ipv4AddressHelper::NumAddressBits(uint32_t maskbits) const
{
    NS_LOG_FUNCTION_NOARGS();
    for (uint32_t i = 0; i < N_BITS; ++i)
    {
        if (maskbits & 1)
        {
            NS_LOG_LOGIC("NumAddressBits -> " << i);
            return i;
        }
        maskbits >>= 1;
    }

    NS_ASSERT_MSG(false, "Ipv4AddressHelper::NumAddressBits(): Bad Mask");
    return 0;
}

void
Ipv4AddressHelper::AllowDuplicates(bool allow)
{
    SimulationSingleton<Ipv4AddressDuplicateChecker>::Get()->AllowDuplicates(allow);
}

} // namespace ns3
