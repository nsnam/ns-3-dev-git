/*
 * Copyright (c) 2022 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Bhaskar Kataria <bhaskar.k7920@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

/*
  This test suite executes independent, self-contained test cases for FlentApplication.
  Each test case:
  - Runs a specific Flent configuration (rrul, tcp_upload, tcp_download, ping).
  - Writes a .flent output file to the runner-managed temporary directory.
  - Verifies the File integrity of the generated JSON metadata.
  - Asserts that the calculated metrics (throughput and/or ICMP latency) fall within expected
  baseline bounds.
 */

#include "nlohmann/json.hpp"

#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/flent-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/queue-disc-container.h"
#include "ns3/queue-size.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/uinteger.h"

#include <fstream>

using namespace ns3;

/**
 * @brief Base test class providing Flent file integrity verification.
 */
class FlentTestCase : public TestCase
{
  public:
    /**
     * @brief Constructor
     * @param name The name of the test case
     */
    FlentTestCase(std::string name);

  protected:
    /**
     * @brief Verifies the structural integrity of the generated JSON data.
     * @param root The parsed JSON root object.
     * @param filename The name of the file being verified.
     */
    void VerifyFlentFileIntegrity(const nlohmann::json& root, const std::string& filename);
};

FlentTestCase::FlentTestCase(std::string name)
    : TestCase(name)
{
}

void
FlentTestCase::VerifyFlentFileIntegrity(const nlohmann::json& root, const std::string& filename)
{
    auto itr_name = root.begin();
    NS_TEST_ASSERT_MSG_EQ((itr_name++).key(),
                          "metadata",
                          "in file " + filename + " metadata not available");
    NS_TEST_ASSERT_MSG_EQ((itr_name++).key(),
                          "raw_values",
                          "in file " + filename + " raw_values not available");
    NS_TEST_ASSERT_MSG_EQ((itr_name++).key(),
                          "results",
                          "in file " + filename + " results not available");
    NS_TEST_ASSERT_MSG_EQ((itr_name++).key(),
                          "version",
                          "in file " + filename + " version not available");
    NS_TEST_ASSERT_MSG_EQ((itr_name).key(),
                          "x_values",
                          "in file " + filename + " x_values not available");

    for (auto itrRawValues = root["raw_values"].begin(), itrResults = root["results"].begin();
         itrRawValues != root["raw_values"].end();
         itrRawValues++, itrResults++)
    {
        NS_TEST_ASSERT_MSG_EQ(itrRawValues.key(),
                              itrResults.key(),
                              filename + " number of results and raw values do not match");
        NS_TEST_ASSERT_MSG_EQ(root["results"][itrResults.key()].size(),
                              root["x_values"].size(),
                              filename + " number of results and raw values does not match");
    }
}

/**
 * Flent rrul test: verifies test execution, metadata integrity, throughput limits, and ICMP latency
 * limits.
 */
class FlentApplicationRrul : public FlentTestCase
{
  public:
    FlentApplicationRrul();
    ~FlentApplicationRrul() override;

  private:
    void DoRun() override;
};

FlentApplicationRrul::FlentApplicationRrul()
    : FlentTestCase("Test flent rrul")
{
}

FlentApplicationRrul::~FlentApplicationRrul()
{
}

void
FlentApplicationRrul::DoRun()
{
    std::string testName = "rrul";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(60);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::string outputFile = CreateTempDirFilename("rrul.flent");

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));
    flentHelper.SetAttribute("OutputFilename", StringValue(outputFile));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    std::ifstream testFile(outputFile, std::ifstream::binary);
    NS_TEST_ASSERT_MSG_EQ(testFile.is_open(), true, "rrul.flent was not created in temp dir");
    nlohmann::json root;
    testFile >> root;

    VerifyFlentFileIntegrity(root, "rrul.flent");

    int count = 0;
    double throughput = 0.0;
    double pingLatency = 0.0;

    for (auto itrTcpDownload = root["results"]["TCP download BE"].begin();
         itrTcpDownload != root["results"]["TCP download BE"].end();
         itrTcpDownload++)
    {
        if (itrTcpDownload->is_null() || itrTcpDownload->get<double>() == 0.0)
        {
            continue;
        }
        count += 1;
        throughput += itrTcpDownload->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(throughput / count,
                          12,
                          "TCP Download BE throughput should be less than 12");
    NS_TEST_ASSERT_MSG_GT(throughput / count,
                          11,
                          "TCP Download BE throughput should be greater than 11");

    count = 0;
    throughput = 0.0;
    for (auto itrTcpDownload = root["results"]["TCP download BK"].begin();
         itrTcpDownload != root["results"]["TCP download BK"].end();
         itrTcpDownload++)
    {
        if (itrTcpDownload->is_null() || itrTcpDownload->get<double>() == 0.0)
        {
            continue;
        }
        count += 1;
        throughput += itrTcpDownload->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(throughput / count,
                          12,
                          "TCP Download BK throughput should be less than 12");
    NS_TEST_ASSERT_MSG_GT(throughput / count,
                          11,
                          "TCP Download BK throughput should be greater than 11");

    count = 0;
    for (auto itrPing = root["results"]["Ping (ms) ICMP"].begin();
         itrPing != root["results"]["Ping (ms) ICMP"].end();
         itrPing++)
    {
        if (itrPing->is_null() || itrPing->get<double>() == 0.0)
        {
            continue;
        }
        count += 1;
        pingLatency += itrPing->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(pingLatency / count, 84, "Ping latency should be less than 84");
    NS_TEST_ASSERT_MSG_GT(pingLatency / count,
                          80,
                          "Ping latency throughput should be greater than 80");
}

/**
 * Flent tcp_upload test: verifies test execution, metadata integrity, throughput limits, and ICMP
 * latency limits.
 */
class FlentApplicationTcpUpload : public FlentTestCase
{
  public:
    FlentApplicationTcpUpload();
    ~FlentApplicationTcpUpload() override;

  private:
    void DoRun() override;
};

FlentApplicationTcpUpload::FlentApplicationTcpUpload()
    : FlentTestCase("Test flent TCP Upload")
{
}

FlentApplicationTcpUpload::~FlentApplicationTcpUpload()
{
}

void
FlentApplicationTcpUpload::DoRun()
{
    std::string testName = "tcp_upload";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(60);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::string outputFile = CreateTempDirFilename("tcp_upload.flent");

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));
    flentHelper.SetAttribute("OutputFilename", StringValue(outputFile));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    std::ifstream testFile(outputFile, std::ifstream::binary);
    NS_TEST_ASSERT_MSG_EQ(testFile.is_open(), true, "tcp_upload.flent was not created in temp dir");
    nlohmann::json root;
    testFile >> root;

    VerifyFlentFileIntegrity(root, "tcp_upload.flent");

    int count = 0;
    double throughput = 0.0;
    double pingLatency = 0.0;

    for (auto itr = root["results"]["TCP upload"].begin();
         itr != root["results"]["TCP upload"].end();
         itr++)
    {
        if (itr->is_null() || itr->get<double>() == 0.0)
        {
            continue;
        }
        count++;
        throughput += itr->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(throughput / count, 50, "TCP upload throughput should be less than 50");
    NS_TEST_ASSERT_MSG_GT(throughput / count,
                          43,
                          "TCP upload throughput should be greater than 43");

    count = 0;
    for (auto itr = root["results"]["Ping (ms) ICMP"].begin();
         itr != root["results"]["Ping (ms) ICMP"].end();
         itr++)
    {
        if (itr->is_null() || itr->get<double>() == 0.0)
        {
            continue;
        }
        count++;
        pingLatency += itr->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(pingLatency / count, 82, "Ping latency should be less than 82");
    NS_TEST_ASSERT_MSG_GT(pingLatency / count, 80, "Ping latency should be greater than 80");
}

/**
 * Flent tcp_download test: verifies test execution, metadata integrity, throughput limits, and ICMP
 * latency limits.
 */
class FlentApplicationTcpDownload : public FlentTestCase
{
  public:
    FlentApplicationTcpDownload();
    ~FlentApplicationTcpDownload() override;

  private:
    void DoRun() override;
};

FlentApplicationTcpDownload::FlentApplicationTcpDownload()
    : FlentTestCase("Test flent TCP Download")
{
}

FlentApplicationTcpDownload::~FlentApplicationTcpDownload()
{
}

void
FlentApplicationTcpDownload::DoRun()
{
    std::string testName = "tcp_download";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(60);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::string outputFile = CreateTempDirFilename("tcp_download.flent");

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));
    flentHelper.SetAttribute("OutputFilename", StringValue(outputFile));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    std::ifstream testFile(outputFile, std::ifstream::binary);
    NS_TEST_ASSERT_MSG_EQ(testFile.is_open(),
                          true,
                          "tcp_download.flent was not created in temp dir");
    nlohmann::json root;
    testFile >> root;

    VerifyFlentFileIntegrity(root, "tcp_download.flent");

    int count = 0;
    double throughput = 0.0;
    double pingLatency = 0.0;

    for (auto itr = root["results"]["TCP download"].begin();
         itr != root["results"]["TCP download"].end();
         itr++)
    {
        if (itr->is_null() || itr->get<double>() == 0.0)
        {
            continue;
        }
        count++;
        throughput += itr->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(throughput / count, 50, "TCP Download throughput should be less than 50");
    NS_TEST_ASSERT_MSG_GT(throughput / count,
                          42,
                          "TCP Download throughput should be greater than 42");

    count = 0;
    for (auto itr = root["results"]["Ping (ms) ICMP"].begin();
         itr != root["results"]["Ping (ms) ICMP"].end();
         itr++)
    {
        if (itr->is_null() || itr->get<double>() == 0.0)
        {
            continue;
        }
        count++;
        pingLatency += itr->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(pingLatency / count, 82, "Ping latency should be less than 82");
    NS_TEST_ASSERT_MSG_GT(pingLatency / count, 80, "Ping latency should be greater than 80");
}

/**
 * Flent ping test: verifies test execution, metadata integrity, throughput limits, and ICMP latency
 * limits.
 */
class FlentApplicationPing : public FlentTestCase
{
  public:
    FlentApplicationPing();
    ~FlentApplicationPing() override;

  private:
    void DoRun() override;
};

FlentApplicationPing::FlentApplicationPing()
    : FlentTestCase("Test flent Ping")
{
}

FlentApplicationPing::~FlentApplicationPing()
{
}

void
FlentApplicationPing::DoRun()
{
    std::string testName = "ping";
    Time rtt = MilliSeconds(80);
    DataRate bw("50Mbps");
    Time length = Seconds(60);
    Time delay = Seconds(0);

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::string outputFile = CreateTempDirFilename("ping.flent");

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetAttribute("StartTime", TimeValue(delay));
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));
    flentHelper.SetAttribute("Length", TimeValue(length));
    flentHelper.SetAttribute("OutputFilename", StringValue(outputFile));

    ApplicationContainer flent = flentHelper.Install(n.Get(0));
    flent.Start(delay);
    flent.Stop(delay + length + Seconds(10));

    Simulator::Stop(delay + length + Seconds(10) + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    std::ifstream testFile(outputFile, std::ifstream::binary);
    NS_TEST_ASSERT_MSG_EQ(testFile.is_open(), true, "ping.flent was not created in temp dir");
    nlohmann::json root;
    testFile >> root;

    VerifyFlentFileIntegrity(root, "ping.flent");

    int count = 0;
    double pingLatency = 0.0;

    for (auto itr = root["results"]["Ping (ms) ICMP"].begin();
         itr != root["results"]["Ping (ms) ICMP"].end();
         itr++)
    {
        if (itr->is_null() || itr->get<double>() == 0.0)
        {
            continue;
        }
        count++;
        pingLatency += itr->get<double>();
    }
    NS_TEST_ASSERT_MSG_LT(pingLatency / count, 82, "Ping latency should be less than 82");
    NS_TEST_ASSERT_MSG_GT(pingLatency / count, 80, "Ping latency should be greater than 80");
}

/**
 * @ingroup applications
 * @brief Flent Application Test Suite
 */
class FlentApplicationTestSuite : public TestSuite
{
  public:
    /**
     * @brief Constructor
     */
    FlentApplicationTestSuite();
};

FlentApplicationTestSuite::FlentApplicationTestSuite()
    : TestSuite("flent-application", TestSuite::Type::UNIT)
{
    AddTestCase(new FlentApplicationRrul, TestCase::Duration::EXTENSIVE);
    AddTestCase(new FlentApplicationTcpUpload, TestCase::Duration::EXTENSIVE);
    AddTestCase(new FlentApplicationTcpDownload, TestCase::Duration::EXTENSIVE);
    AddTestCase(new FlentApplicationPing, TestCase::Duration::EXTENSIVE);
}

static FlentApplicationTestSuite
    g_FlentApplicationTestSuite; //!< Static variable for test initialization
