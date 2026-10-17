/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/iana-link-type-numbers.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pcap-file-wrapper.h"
#include "ns3/pcap-file.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"

#include <cstdint>
#include <cstdio>
#include <ios>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("pcap-explicit-filename-test-suite");

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Regression test for @issueid{1150}.
 *
 * Reusing the same pcap filename for more than one interface, e.g. calling
 * EnablePcapIpv4(prefix, ..., explicitFilename = true) for two different
 * interfaces with the same @c prefix, must produce a single well-formed pcap
 * file containing every record written by every interface.
 */
class PcapExplicitFilenameTestCase : public TestCase
{
  public:
    PcapExplicitFilenameTestCase();
    ~PcapExplicitFilenameTestCase() override;

  private:
    void DoSetup() override;
    void DoRun() override;
    void DoTeardown() override;

    /// Magic number of a standard (microsecond) little-endian pcap file.
    static constexpr uint32_t PCAP_MAGIC = 0xa1b2c3d4;
    /// Size, in bytes, of the pcap global header.
    static constexpr uint32_t PCAP_GLOBAL_HEADER_LEN = 24;
    /// Size, in bytes, of a per-packet pcap record header.
    static constexpr uint32_t PCAP_RECORD_HEADER_LEN = 16;
    /// Snap length used when initializing the pcap files.
    static constexpr uint32_t SNAP_LEN = 65535;

    std::string m_testFilename; //!< Explicit pcap file name shared by both "interfaces"
};

PcapExplicitFilenameTestCase::PcapExplicitFilenameTestCase()
    : TestCase("Reusing one explicit pcap filename for two interfaces yields a valid pcap (#1150)")
{
}

PcapExplicitFilenameTestCase::~PcapExplicitFilenameTestCase()
{
}

void
PcapExplicitFilenameTestCase::DoSetup()
{
    m_testFilename = CreateTempDirFilename("pcap-explicit-filename-1150.pcap");
    std::remove(m_testFilename.c_str());
}

void
PcapExplicitFilenameTestCase::DoTeardown()
{
    if (std::remove(m_testFilename.c_str()))
    {
        NS_LOG_ERROR("Failed to delete file " << m_testFilename);
    }
}

void
PcapExplicitFilenameTestCase::DoRun()
{
    PcapHelper pcapHelper;

    Ptr<PcapFileWrapper> fileA =
        pcapHelper.CreateFile(m_testFilename, std::ios::out, iana::linktype::RAW, SNAP_LEN);
    NS_TEST_ASSERT_MSG_EQ(fileA->Fail(),
                          false,
                          "CreateFile() for the first interface should succeed");

    Ptr<PcapFileWrapper> fileB =
        pcapHelper.CreateFile(m_testFilename, std::ios::out, iana::linktype::RAW, SNAP_LEN);
    NS_TEST_ASSERT_MSG_EQ(fileB->Fail(),
                          false,
                          "CreateFile() for the second interface should succeed");

    std::vector<uint8_t> payloadA(40, 0xa1);
    std::vector<uint8_t> payloadB(80, 0xb2);
    Ptr<Packet> pktA = Create<Packet>(payloadA.data(), payloadA.size());
    Ptr<Packet> pktB = Create<Packet>(payloadB.data(), payloadB.size());

    // Interleaved writes, mirroring the trace sinks of the two interfaces.
    fileA->Write(Seconds(1), pktA);
    fileB->Write(Seconds(2), pktB);
    fileA->Write(Seconds(3), pktA);
    fileB->Write(Seconds(4), pktB);

    // Flush and close both file handles.
    fileA = nullptr;
    fileB = nullptr;

    PcapFile reader;
    reader.Open(m_testFilename, std::ios::in);
    NS_TEST_ASSERT_MSG_EQ(reader.Fail(),
                          false,
                          "Resulting pcap file should have a valid global header");
    NS_TEST_ASSERT_MSG_EQ(reader.GetMagic(),
                          PCAP_MAGIC,
                          "Resulting pcap file should carry the standard pcap magic number");

    FILE* raw = std::fopen(m_testFilename.c_str(), "rb");
    NS_TEST_ASSERT_MSG_NE(raw, nullptr, "Should be able to open the resulting pcap file");
    std::fseek(raw, 0, SEEK_END);
    auto fileSize = static_cast<uint64_t>(std::ftell(raw));
    std::fclose(raw);

    // PcapFile::Read() positions the reader just past the global header on
    // Open(), so account for that header up front.
    uint64_t consumed = PCAP_GLOBAL_HEADER_LEN;
    uint32_t recordCount = 0;
    std::vector<uint8_t> buffer(SNAP_LEN);

    while (consumed + PCAP_RECORD_HEADER_LEN <= fileSize)
    {
        uint32_t tsSec = 0;
        uint32_t tsUsec = 0;
        uint32_t inclLen = 0;
        uint32_t origLen = 0;
        uint32_t readLen = 0;

        reader.Read(buffer.data(), buffer.size(), tsSec, tsUsec, inclLen, origLen, readLen);

        if (reader.Eof())
        {
            break;
        }
        NS_TEST_ASSERT_MSG_EQ(reader.Fail(),
                              false,
                              "Record " << recordCount << " should read without error");

        NS_TEST_ASSERT_MSG_LT_OR_EQ(inclLen,
                                    SNAP_LEN,
                                    "Record " << recordCount
                                              << " has an implausible included length");

        NS_TEST_ASSERT_MSG_LT_OR_EQ(consumed + PCAP_RECORD_HEADER_LEN + inclLen,
                                    fileSize,
                                    "Record " << recordCount << " runs past the end of the file");

        consumed += PCAP_RECORD_HEADER_LEN + inclLen;
        recordCount++;
    }

    NS_TEST_ASSERT_MSG_EQ(consumed,
                          fileSize,
                          "Record stream should end exactly at the end of the file");

    NS_TEST_ASSERT_MSG_EQ(recordCount, 4, "Resulting pcap should contain all four written records");

    reader.Close();
}

/**
 * @ingroup network-test
 * @ingroup tests
 *
 * @brief Test suite for the @issueid{1150} explicit-pcap-filename regression test.
 */
class PcapExplicitFilenameTestSuite : public TestSuite
{
  public:
    PcapExplicitFilenameTestSuite();
};

PcapExplicitFilenameTestSuite::PcapExplicitFilenameTestSuite()
    : TestSuite("pcap-explicit-filename", Type::UNIT)
{
    AddTestCase(new PcapExplicitFilenameTestCase, TestCase::Duration::QUICK);
}

static PcapExplicitFilenameTestSuite
    g_pcapExplicitFilenameTestSuite; //!< Static variable for test initialization
