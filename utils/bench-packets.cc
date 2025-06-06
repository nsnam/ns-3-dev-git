/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

// This program can be used to benchmark packet serialization/deserialization
// operations using Headers and Tags, for various numbers of packets 'n'
// Sample usage:  ./ns3 run 'bench-packets --n=10000'

#include "ns3/command-line.h"
#include "ns3/packet-metadata.h"
#include "ns3/packet.h"
#include "ns3/system-wall-clock-ms.h"

#include <algorithm>
#include <cstdlib> // for exit ()
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace ns3;

/// BenchHeader class used for benchmarking packet serialization/deserialization
template <int N>
class BenchHeader : public Header
{
  public:
    BenchHeader();
    /**
     * Returns true if the header has been deserialized and the
     * deserialization was correct.  If Deserialize() has not yet been
     * called on the header, will return false.
     *
     * @returns true if success, false if failed or if deserialization not tried
     */
    bool IsOk() const;

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * Get type name function
     * @returns the type name string
     */
    static std::string GetTypeName();
    bool m_ok; ///< variable to track whether deserialization succeeded
};

template <int N>
BenchHeader<N>::BenchHeader()
    : m_ok(false)
{
}

template <int N>
bool
BenchHeader<N>::IsOk() const
{
    return m_ok;
}

template <int N>
std::string
BenchHeader<N>::GetTypeName()
{
    std::ostringstream oss;
    oss << "ns3::BenchHeader<" << N << ">";
    return oss.str();
}

template <int N>
TypeId
BenchHeader<N>::GetTypeId()
{
    static TypeId tid = TypeId(GetTypeName())
                            .SetParent<Header>()
                            .SetGroupName("Utils")
                            .HideFromDocumentation()
                            .AddConstructor<BenchHeader<N>>();
    return tid;
}

template <int N>
TypeId
BenchHeader<N>::GetInstanceTypeId() const
{
    return GetTypeId();
}

template <int N>
void
BenchHeader<N>::Print(std::ostream& os) const
{
    NS_ASSERT(false);
}

template <int N>
uint32_t
BenchHeader<N>::GetSerializedSize() const
{
    return N;
}

template <int N>
void
BenchHeader<N>::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(N, N);
}

template <int N>
uint32_t
BenchHeader<N>::Deserialize(Buffer::Iterator start)
{
    m_ok = true;
    for (int i = 0; i < N; i++)
    {
        if (start.ReadU8() != N)
        {
            m_ok = false;
        }
    }
    return N;
}

/// BenchTag class used for benchmarking packet serialization/deserialization
template <int N>
class BenchTag : public Tag
{
  public:
    /**
     * Get the bench tag name.
     * @return the name.
     */
    static std::string GetName()
    {
        std::ostringstream oss;
        oss << "anon::BenchTag<" << N << ">";
        return oss.str();
    }

    /**
     * Register this type.
     * @return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId(GetName())
                                .SetParent<Tag>()
                                .SetGroupName("Utils")
                                .HideFromDocumentation()
                                .AddConstructor<BenchTag<N>>();
        return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
        return N;
    }

    void Serialize(TagBuffer buf) const override
    {
        for (uint32_t i = 0; i < N; ++i)
        {
            buf.WriteU8(N);
        }
    }

    void Deserialize(TagBuffer buf) override
    {
        for (uint32_t i = 0; i < N; ++i)
        {
            buf.ReadU8();
        }
    }

    void Print(std::ostream& os) const override
    {
        os << "N=" << N;
    }

    BenchTag()
        : Tag()
    {
    }
};

static void
benchD(uint32_t n)
{
    BenchHeader<25> ipv4;
    BenchHeader<8> udp;
    BenchTag<16> tag1;
    BenchTag<17> tag2;

    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        p->AddPacketTag(tag1);
        p->AddHeader(udp);
        p->RemovePacketTag(tag1);
        p->AddPacketTag(tag2);
        p->AddHeader(ipv4);
        Ptr<Packet> o = p->Copy();
        o->RemoveHeader(ipv4);
        p->RemovePacketTag(tag2);
        o->RemoveHeader(udp);
    }
}

static void
benchA(uint32_t n)
{
    BenchHeader<25> ipv4;
    BenchHeader<8> udp;

    // The original version of this program did not use BenchHeader::IsOK ()
    // Below are two asserts that suggest how it can be used.
    NS_ASSERT_MSG(ipv4.IsOk() == false, "IsOk() should be false before deserialization");
    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        p->AddHeader(udp);
        p->AddHeader(ipv4);
        Ptr<Packet> o = p->Copy();
        o->RemoveHeader(ipv4);
        o->RemoveHeader(udp);
    }
    NS_ASSERT_MSG(ipv4.IsOk() == true, "IsOk() should be true after deserialization");
}

static void
benchB(uint32_t n)
{
    BenchHeader<25> ipv4;
    BenchHeader<8> udp;

    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        p->AddHeader(udp);
        p->AddHeader(ipv4);
    }
}

static void
C2(Ptr<Packet> p)
{
    BenchHeader<8> udp;

    p->RemoveHeader(udp);
}

static void
C1(Ptr<Packet> p)
{
    BenchHeader<25> ipv4;
    p->RemoveHeader(ipv4);
    C2(p);
}

static void
benchC(uint32_t n)
{
    BenchHeader<25> ipv4;
    BenchHeader<8> udp;

    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        p->AddHeader(udp);
        p->AddHeader(ipv4);
        C1(p);
    }
}

static void
benchFragment(uint32_t n)
{
    BenchHeader<25> ipv4;
    BenchHeader<8> udp;

    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        p->AddHeader(udp);
        p->AddHeader(ipv4);

        Ptr<Packet> frag0 = p->CreateFragment(0, 250);
        Ptr<Packet> frag1 = p->CreateFragment(250, 250);
        Ptr<Packet> frag2 = p->CreateFragment(500, 500);
        Ptr<Packet> frag3 = p->CreateFragment(1000, 500);
        Ptr<Packet> frag4 = p->CreateFragment(1500, 500);

        /* Mix fragments in different order */
        frag2->AddAtEnd(frag3);
        frag4->AddAtEnd(frag1);
        frag2->AddAtEnd(frag4);
        frag0->AddAtEnd(frag2);

        frag0->RemoveHeader(ipv4);
        frag0->RemoveHeader(udp);
    }
}

static void
benchByteTags(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        Ptr<Packet> p = Create<Packet>(2000);
        for (uint32_t j = 0; j < 100; j++)
        {
            BenchTag<0> tag;
            p->AddByteTag(tag);
        }
        Ptr<Packet> q = Create<Packet>(1000);

        // This should trigger adjustment of all byte tags
        q->AddAtEnd(p);
    }
}

static uint64_t
runBenchOneIteration(void (*bench)(uint32_t), uint32_t n)
{
    SystemWallClockMs time;
    time.Start();
    (*bench)(n);
    uint64_t deltaMs = time.End();
    return deltaMs;
}

static void
runBench(void (*bench)(uint32_t), uint32_t n, uint32_t minIterations, const char* name)
{
    uint64_t minDelay = std::numeric_limits<uint64_t>::max();
    for (uint32_t i = 0; i < minIterations; i++)
    {
        uint64_t delay = runBenchOneIteration(bench, n);
        minDelay = std::min(minDelay, delay);
    }
    double ps = n;
    ps *= 1000;
    ps /= minDelay;
    std::cout << ps << " packets/s"
              << " (" << minDelay << " ms elapsed)\t" << name << std::endl;
}

int
main(int argc, char* argv[])
{
    uint32_t n = 0;
    uint32_t minIterations = 1;
    bool enablePrinting = false;

    CommandLine cmd(__FILE__);
    cmd.Usage("Benchmark Packet class");
    cmd.AddValue("n", "number of iterations", n);
    cmd.AddValue("min-iterations",
                 "number of subiterations to minimize iteration time over",
                 minIterations);
    cmd.AddValue("enable-printing", "enable packet printing", enablePrinting);
    cmd.Parse(argc, argv);

    if (n == 0)
    {
        std::cerr << "Error-- number of packets must be specified "
                  << "by command-line argument --n=(number of packets)" << std::endl;
        exit(1);
    }
    std::cout << "Running bench-packets with n=" << n << std::endl;
    std::cout << "All tests begin by adding UDP and IPv4 headers." << std::endl;

    runBench(&benchA, n, minIterations, "Copy packet, remove headers");
    runBench(&benchB, n, minIterations, "Just add headers");
    runBench(&benchC, n, minIterations, "Remove by func call");
    runBench(&benchD, n, minIterations, "Intermixed add/remove headers and tags");
    runBench(&benchFragment, n, minIterations, "Fragmentation and concatenation");
    runBench(&benchByteTags, n, minIterations, "Benchmark byte tags");

    return 0;
}
