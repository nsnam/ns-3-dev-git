/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/ipv4-address-generator.h"
#include "ns3/simulation-singleton.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @ingroup internet-test
 *
 * @brief IPv4 network number Test
 */
class NetworkNumberAllocatorTestCase : public TestCase
{
  public:
    NetworkNumberAllocatorTestCase();
    void DoRun() override;
    void DoTeardown() override;
};

NetworkNumberAllocatorTestCase::NetworkNumberAllocatorTestCase()
    : TestCase("Make sure the network number allocator is working on some of network prefixes.")
{
}

void
NetworkNumberAllocatorTestCase::DoTeardown()
{
    Ipv4AddressGenerator::Reset();
}

void
NetworkNumberAllocatorTestCase::DoRun()
{
    Ipv4Address network;
    Ipv4AddressGenerator::Init(Ipv4Address("1.0.0.0"),
                               Ipv4Mask("255.0.0.0"),
                               Ipv4Address("0.0.0.0"));
    network = Ipv4AddressGenerator::GetNetwork(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("1.0.0.0"), "001");
    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("2.0.0.0"), "001");

    Ipv4AddressGenerator::Init(Ipv4Address("0.1.0.0"),
                               Ipv4Mask("255.255.0.0"),
                               Ipv4Address("0.0.0.0"));
    network = Ipv4AddressGenerator::GetNetwork(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.1.0.0"), "003");
    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.2.0.0"), "004");

    Ipv4AddressGenerator::Init(Ipv4Address("0.0.1.0"),
                               Ipv4Mask("255.255.255.0"),
                               Ipv4Address("0.0.0.0"));
    network = Ipv4AddressGenerator::GetNetwork(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.0.1.0"), "005");
    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.0.2.0"), "006");

    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("3.0.0.0"), "007");
    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.3.0.0"), "008");
    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.0.3.0"), "009");
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 address allocator Test
 */
class AddressAllocatorTestCase : public TestCase
{
  public:
    AddressAllocatorTestCase();

  private:
    void DoRun() override;
    void DoTeardown() override;
};

AddressAllocatorTestCase::AddressAllocatorTestCase()
    : TestCase("Sanity check on allocation of addresses")
{
}

void
AddressAllocatorTestCase::DoRun()
{
    Ipv4Address address;

    Ipv4AddressGenerator::Init(Ipv4Address("1.0.0.0"),
                               Ipv4Mask("255.0.0.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("1.0.0.3"), "100");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("1.0.0.4"), "101");

    Ipv4AddressGenerator::Init(Ipv4Address("0.1.0.0"),
                               Ipv4Mask("255.255.0.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.1.0.3"), "102");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.1.0.4"), "103");

    Ipv4AddressGenerator::Init(Ipv4Address("0.0.1.0"),
                               Ipv4Mask("255.255.255.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.0.1.3"), "104");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.0.1.4"), "105");
}

void
AddressAllocatorTestCase::DoTeardown()
{
    Ipv4AddressGenerator::Reset();
    Simulator::Destroy();
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 network and address allocator Test
 */
class NetworkAndAddressTestCase : public TestCase
{
  public:
    NetworkAndAddressTestCase();
    void DoRun() override;
    void DoTeardown() override;
};

NetworkAndAddressTestCase::NetworkAndAddressTestCase()
    : TestCase("Make sure Network and address allocation play together.")
{
}

void
NetworkAndAddressTestCase::DoTeardown()
{
    Ipv4AddressGenerator::Reset();
    Simulator::Destroy();
}

void
NetworkAndAddressTestCase::DoRun()
{
    Ipv4Address address;
    Ipv4Address network;

    Ipv4AddressGenerator::Init(Ipv4Address("3.0.0.0"),
                               Ipv4Mask("255.0.0.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("3.0.0.3"), "200");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("3.0.0.4"), "201");

    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("4.0.0.0"), "202");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.0.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("4.0.0.5"), "203");

    Ipv4AddressGenerator::Init(Ipv4Address("0.3.0.0"),
                               Ipv4Mask("255.255.0.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.3.0.3"), "204");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.3.0.4"), "205");

    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.4.0.0"), "206");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.0.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.4.0.5"), "207");

    Ipv4AddressGenerator::Init(Ipv4Address("0.0.3.0"),
                               Ipv4Mask("255.255.255.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.0.3.3"), "208");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.0.3.4"), "209");

    network = Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(network, Ipv4Address("0.0.4.0"), "210");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("0.0.4.5"), "211");
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 AddressGenerator example (sort of) Test
 */
class ExampleAddressGeneratorTestCase : public TestCase
{
  public:
    ExampleAddressGeneratorTestCase();

  private:
    void DoRun() override;
    void DoTeardown() override;
};

ExampleAddressGeneratorTestCase::ExampleAddressGeneratorTestCase()
    : TestCase("A quick kindof-semi-almost-real example")
{
}

void
ExampleAddressGeneratorTestCase::DoTeardown()
{
    Ipv4AddressGenerator::Reset();
}

void
ExampleAddressGeneratorTestCase::DoRun()
{
    Ipv4Address address;
    //
    // First, initialize our /24 network to 192.168.0.0 and begin
    // allocating with ip address 0.0.0.3 out of that prefix.
    //
    Ipv4AddressGenerator::Init(Ipv4Address("192.168.0.0"),
                               Ipv4Mask("255.255.255.0"),
                               Ipv4Address("0.0.0.3"));
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("192.168.0.3"), "300");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("192.168.0.4"), "301");
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("192.168.0.5"), "302");
    //
    // Allocate the next network out of our /24 network (this should be
    // 192.168.1.0) and begin allocating with IP address 0.0.0.3 out of that
    // prefix.
    //
    Ipv4AddressGenerator::NextNetwork(Ipv4Mask("255.255.255.0"));
    Ipv4AddressGenerator::InitAddress(Ipv4Address("0.0.0.3"), Ipv4Mask("255.255.255.0"));
    //
    // The first address we should get is the previous numbers ORed together, which
    // is 192.168.1.3, of course.
    //
    address = Ipv4AddressGenerator::NextAddress(Ipv4Mask("255.255.255.0"));
    NS_TEST_EXPECT_MSG_EQ(address, Ipv4Address("192.168.1.3"), "304");
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 address collision Test
 */
class AddressCollisionTestCase : public TestCase
{
  public:
    AddressCollisionTestCase();

  private:
    void DoRun() override;
    void DoTeardown() override;
};

AddressCollisionTestCase::AddressCollisionTestCase()
    : TestCase("Make sure that the address collision logic works.")
{
}

void
AddressCollisionTestCase::DoTeardown()
{
    Ipv4AddressGenerator::Reset();
    Simulator::Destroy();
}

void
AddressCollisionTestCase::DoRun()
{
    Ipv4AddressGenerator::AddAllocated("0.0.0.5");
    Ipv4AddressGenerator::AddAllocated("0.0.0.10");
    Ipv4AddressGenerator::AddAllocated("0.0.0.15");
    Ipv4AddressGenerator::AddAllocated("0.0.0.20");

    Ipv4AddressGenerator::AddAllocated("0.0.0.4");
    Ipv4AddressGenerator::AddAllocated("0.0.0.3");
    Ipv4AddressGenerator::AddAllocated("0.0.0.2");
    Ipv4AddressGenerator::AddAllocated("0.0.0.1");

    Ipv4AddressGenerator::AddAllocated("0.0.0.6");
    Ipv4AddressGenerator::AddAllocated("0.0.0.7");
    Ipv4AddressGenerator::AddAllocated("0.0.0.8");
    Ipv4AddressGenerator::AddAllocated("0.0.0.9");

    Ipv4AddressGenerator::AddAllocated("0.0.0.11");
    Ipv4AddressGenerator::AddAllocated("0.0.0.12");
    Ipv4AddressGenerator::AddAllocated("0.0.0.13");
    Ipv4AddressGenerator::AddAllocated("0.0.0.14");

    Ipv4AddressGenerator::AddAllocated("0.0.0.19");
    Ipv4AddressGenerator::AddAllocated("0.0.0.18");
    Ipv4AddressGenerator::AddAllocated("0.0.0.17");
    Ipv4AddressGenerator::AddAllocated("0.0.0.16");

    Ipv4AddressGenerator::TestMode();
    bool allocated = Ipv4AddressGenerator::IsAddressAllocated("0.0.0.21");
    NS_TEST_EXPECT_MSG_EQ(allocated, false, "0.0.0.21 should not be already allocated");
    bool added = Ipv4AddressGenerator::AddAllocated("0.0.0.21");
    NS_TEST_EXPECT_MSG_EQ(added, true, "400");

    allocated = Ipv4AddressGenerator::IsAddressAllocated("0.0.0.4");
    NS_TEST_EXPECT_MSG_EQ(allocated, true, "0.0.0.4 should be already allocated");
    added = Ipv4AddressGenerator::AddAllocated("0.0.0.4");
    NS_TEST_EXPECT_MSG_EQ(added, false, "401");

    allocated = Ipv4AddressGenerator::IsAddressAllocated("0.0.0.9");
    NS_TEST_EXPECT_MSG_EQ(allocated, true, "0.0.0.9 should be already allocated");
    added = Ipv4AddressGenerator::AddAllocated("0.0.0.9");
    NS_TEST_EXPECT_MSG_EQ(added, false, "402");

    allocated = Ipv4AddressGenerator::IsAddressAllocated("0.0.0.16");
    NS_TEST_EXPECT_MSG_EQ(allocated, true, "0.0.0.16 should be already allocated");
    added = Ipv4AddressGenerator::AddAllocated("0.0.0.16");
    NS_TEST_EXPECT_MSG_EQ(added, false, "403");

    allocated = Ipv4AddressGenerator::IsAddressAllocated("0.0.0.21");
    NS_TEST_EXPECT_MSG_EQ(allocated, true, "0.0.0.21 should be already allocated");
    added = Ipv4AddressGenerator::AddAllocated("0.0.0.21");
    NS_TEST_EXPECT_MSG_EQ(added, false, "404");
}

/**
 * @ingroup internet-test
 *
 * @brief IPv4 Address Generator TestSuite
 */
class Ipv4AddressGeneratorTestSuite : public TestSuite
{
  public:
    Ipv4AddressGeneratorTestSuite();

  private:
};

Ipv4AddressGeneratorTestSuite::Ipv4AddressGeneratorTestSuite()
    : TestSuite("ipv4-address-generator", Type::UNIT)
{
    AddTestCase(new NetworkNumberAllocatorTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new AddressAllocatorTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new NetworkAndAddressTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new ExampleAddressGeneratorTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new AddressCollisionTestCase(), TestCase::Duration::QUICK);
}

static Ipv4AddressGeneratorTestSuite
    g_ipv4AddressGeneratorTestSuite; //!< Static variable for test initialization
