/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/flow-id-tag.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-phy.h"

using namespace ns3;

/// PsrExperiment
class PsrExperiment
{
  public:
    /// Input structure
    struct Input
    {
        Input();
        meter_u distance;     ///< distance
        std::string txMode;   ///< transmit mode
        uint8_t txPowerLevel; ///< transmit power level
        uint32_t packetSize;  ///< packet size
        uint32_t nPackets;    ///< number of packets
    };

    /// Output structure
    struct Output
    {
        uint32_t received; ///< received
    };

    PsrExperiment();

    /**
     * Run function
     * @param input the PSR experiment
     * @returns the PSR experiment output
     */
    PsrExperiment::Output Run(PsrExperiment::Input input);

  private:
    /// Send function
    void Send();
    /**
     * Send receive function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the wifi transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void Receive(Ptr<const WifiPsdu> psdu,
                 RxSignalInfo rxSignalInfo,
                 const WifiTxVector& txVector,
                 const std::vector<bool>& statusPerMpdu);
    Ptr<WifiPhy> m_tx; ///< transmit
    Input m_input;     ///< input
    Output m_output;   ///< output
};

void
PsrExperiment::Send()
{
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(Create<Packet>(m_input.packetSize), WifiMacHeader());
    WifiMode mode = WifiMode(m_input.txMode);
    WifiTxVector txVector;
    txVector.SetTxPowerLevel(m_input.txPowerLevel);
    txVector.SetMode(mode);
    txVector.SetPreambleType(WIFI_PREAMBLE_LONG);
    m_tx->Send(psdu, txVector);
}

void
PsrExperiment::Receive(Ptr<const WifiPsdu> psdu,
                       RxSignalInfo rxSignalInfo,
                       const WifiTxVector& txVector,
                       const std::vector<bool>& statusPerMpdu)
{
    m_output.received++;
}

PsrExperiment::PsrExperiment()
{
}

PsrExperiment::Input::Input()
    : distance(5.0),
      txMode("OfdmRate6Mbps"),
      txPowerLevel(0),
      packetSize(2304),
      nPackets(400)
{
}

PsrExperiment::Output
PsrExperiment::Run(PsrExperiment::Input input)
{
    m_output.received = 0;
    m_input = input;

    Ptr<MobilityModel> posTx = CreateObject<ConstantPositionMobilityModel>();
    posTx->SetPosition(Vector(0.0, 0.0, 0.0));
    Ptr<MobilityModel> posRx = CreateObject<ConstantPositionMobilityModel>();
    posRx->SetPosition(Vector(m_input.distance, 0.0, 0.0));

    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
    channel->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());
    Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel>();
    channel->SetPropagationLossModel(log);

    Ptr<YansWifiPhy> tx = CreateObject<YansWifiPhy>();
    Ptr<YansWifiPhy> rx = CreateObject<YansWifiPhy>();
    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel>();
    tx->SetErrorRateModel(error);
    rx->SetErrorRateModel(error);
    tx->SetChannel(channel);
    rx->SetChannel(channel);
    tx->SetMobility(posTx);
    rx->SetMobility(posRx);

    tx->ConfigureStandard(WIFI_STANDARD_80211a);
    rx->ConfigureStandard(WIFI_STANDARD_80211a);

    rx->SetReceiveOkCallback(MakeCallback(&PsrExperiment::Receive, this));

    for (uint32_t i = 0; i < m_input.nPackets; ++i)
    {
        Simulator::Schedule(Seconds(i), &PsrExperiment::Send, this);
    }
    m_tx = tx;
    Simulator::Run();
    Simulator::Destroy();
    return m_output;
}

/// CollisionExperiment
class CollisionExperiment
{
  public:
    /// Input structure
    struct Input
    {
        Input();
        Time interval;         ///< interval
        double xA;             ///< x A
        double xB;             ///< x B
        std::string txModeA;   ///< transmit mode A
        std::string txModeB;   ///< transmit mode B
        uint8_t txPowerLevelA; ///< transmit power level A
        uint8_t txPowerLevelB; ///< transmit power level B
        uint32_t packetSizeA;  ///< packet size A
        uint32_t packetSizeB;  ///< packet size B
        uint32_t nPackets;     ///< number of packets
    };

    /// Output structure
    struct Output
    {
        uint32_t receivedA; ///< received A
        uint32_t receivedB; ///< received B
    };

    CollisionExperiment();

    /**
     * Run function
     * @param input the collision experiment data
     * @returns the experiment output
     */
    CollisionExperiment::Output Run(CollisionExperiment::Input input);

  private:
    /// Send A function
    void SendA() const;
    /// Send B function
    void SendB() const;
    /**
     * Receive function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the wifi transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void Receive(Ptr<const WifiPsdu> psdu,
                 RxSignalInfo rxSignalInfo,
                 const WifiTxVector& txVector,
                 const std::vector<bool>& statusPerMpdu);
    Ptr<WifiPhy> m_txA; ///< transmit A
    Ptr<WifiPhy> m_txB; ///< transmit B
    uint32_t m_flowIdA; ///< flow ID A
    uint32_t m_flowIdB; ///< flow ID B
    Input m_input;      ///< input
    Output m_output;    ///< output
};

void
CollisionExperiment::SendA() const
{
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(Create<Packet>(m_input.packetSizeA), WifiMacHeader());
    (*psdu->begin())->GetPacket()->AddByteTag(FlowIdTag(m_flowIdA));
    WifiTxVector txVector;
    txVector.SetTxPowerLevel(m_input.txPowerLevelA);
    txVector.SetMode(WifiMode(m_input.txModeA));
    txVector.SetPreambleType(WIFI_PREAMBLE_LONG);
    m_txA->Send(psdu, txVector);
}

void
CollisionExperiment::SendB() const
{
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(Create<Packet>(m_input.packetSizeB), WifiMacHeader());
    (*psdu->begin())->GetPacket()->AddByteTag(FlowIdTag(m_flowIdB));
    WifiTxVector txVector;
    txVector.SetTxPowerLevel(m_input.txPowerLevelB);
    txVector.SetMode(WifiMode(m_input.txModeB));
    txVector.SetPreambleType(WIFI_PREAMBLE_LONG);
    m_txB->Send(psdu, txVector);
}

void
CollisionExperiment::Receive(Ptr<const WifiPsdu> psdu,
                             RxSignalInfo rxSignalInfo,
                             const WifiTxVector& txVector,
                             const std::vector<bool>& statusPerMpdu)
{
    FlowIdTag tag;
    if ((*psdu->begin())->GetPacket()->FindFirstMatchingByteTag(tag))
    {
        if (tag.GetFlowId() == m_flowIdA)
        {
            m_output.receivedA++;
        }
        else if (tag.GetFlowId() == m_flowIdB)
        {
            m_output.receivedB++;
        }
    }
}

CollisionExperiment::CollisionExperiment()
{
}

CollisionExperiment::Input::Input()
    : interval(),
      xA(-5),
      xB(5),
      txModeA("OfdmRate6Mbps"),
      txModeB("OfdmRate6Mbps"),
      txPowerLevelA(0),
      txPowerLevelB(0),
      packetSizeA(2304),
      packetSizeB(2304),
      nPackets(400)
{
}

CollisionExperiment::Output
CollisionExperiment::Run(CollisionExperiment::Input input)
{
    m_output.receivedA = 0;
    m_output.receivedB = 0;
    m_input = input;

    m_flowIdA = FlowIdTag::AllocateFlowId();
    m_flowIdB = FlowIdTag::AllocateFlowId();

    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel>();
    channel->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());
    Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel>();
    channel->SetPropagationLossModel(log);

    Ptr<MobilityModel> posTxA = CreateObject<ConstantPositionMobilityModel>();
    posTxA->SetPosition(Vector(input.xA, 0.0, 0.0));
    Ptr<MobilityModel> posTxB = CreateObject<ConstantPositionMobilityModel>();
    posTxB->SetPosition(Vector(input.xB, 0.0, 0.0));
    Ptr<MobilityModel> posRx = CreateObject<ConstantPositionMobilityModel>();
    posRx->SetPosition(Vector(0, 0.0, 0.0));

    Ptr<YansWifiPhy> txA = CreateObject<YansWifiPhy>();
    Ptr<YansWifiPhy> txB = CreateObject<YansWifiPhy>();
    Ptr<YansWifiPhy> rx = CreateObject<YansWifiPhy>();

    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel>();
    txA->SetErrorRateModel(error);
    txB->SetErrorRateModel(error);
    rx->SetErrorRateModel(error);
    txA->SetChannel(channel);
    txB->SetChannel(channel);
    rx->SetChannel(channel);
    txA->SetMobility(posTxA);
    txB->SetMobility(posTxB);
    rx->SetMobility(posRx);

    txA->ConfigureStandard(WIFI_STANDARD_80211a);
    txB->ConfigureStandard(WIFI_STANDARD_80211a);
    rx->ConfigureStandard(WIFI_STANDARD_80211a);

    rx->SetReceiveOkCallback(MakeCallback(&CollisionExperiment::Receive, this));

    for (uint32_t i = 0; i < m_input.nPackets; ++i)
    {
        Simulator::Schedule(Seconds(i), &CollisionExperiment::SendA, this);
    }
    for (uint32_t i = 0; i < m_input.nPackets; ++i)
    {
        Simulator::Schedule(Seconds(i) + m_input.interval, &CollisionExperiment::SendB, this);
    }
    m_txA = txA;
    m_txB = txB;
    Simulator::Run();
    Simulator::Destroy();
    return m_output;
}

static void
PrintPsr(int argc, char* argv[])
{
    PsrExperiment experiment;
    PsrExperiment::Input input;

    CommandLine cmd(__FILE__);
    cmd.AddValue("Distance", "The distance between two phys", input.distance);
    cmd.AddValue("PacketSize", "The size of each packet sent", input.packetSize);
    cmd.AddValue("TxMode", "The mode to use to send each packet", input.txMode);
    cmd.AddValue("NPackets", "The number of packets to send", input.nPackets);
    cmd.AddValue("TxPowerLevel",
                 "The power level index to use to send each packet",
                 input.txPowerLevel);
    cmd.Parse(argc, argv);

    PsrExperiment::Output output;
    output = experiment.Run(input);

    double psr = output.received;
    psr /= input.nPackets;

    std::cout << psr << std::endl;
}

double
CalcPsr(PsrExperiment::Output output, PsrExperiment::Input input)
{
    double psr = output.received;
    psr /= input.nPackets;
    return psr;
}

static void
PrintPsrVsDistance(int argc, char* argv[])
{
    PsrExperiment::Input input;
    CommandLine cmd(__FILE__);
    cmd.AddValue("TxPowerLevel",
                 "The power level index to use to send each packet",
                 input.txPowerLevel);
    cmd.AddValue("TxMode", "The mode to use to send each packet", input.txMode);
    cmd.AddValue("NPackets", "The number of packets to send", input.nPackets);
    cmd.AddValue("PacketSize", "The size of each packet sent", input.packetSize);
    cmd.Parse(argc, argv);

    for (input.distance = 1.0; input.distance < 165; input.distance += 2.0)
    {
        std::cout << input.distance;
        PsrExperiment experiment;
        PsrExperiment::Output output;

        input.txMode = "OfdmRate6Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate9Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate12Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate18Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate24Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate36Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate48Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        input.txMode = "OfdmRate54Mbps";
        output = experiment.Run(input);
        std::cout << " " << CalcPsr(output, input);

        std::cout << std::endl;
    }
}

static void
PrintSizeVsRange(int argc, char* argv[])
{
    double targetPsr = 0.05;
    PsrExperiment::Input input;
    CommandLine cmd(__FILE__);
    cmd.AddValue("TxPowerLevel",
                 "The power level index to use to send each packet",
                 input.txPowerLevel);
    cmd.AddValue("TxMode", "The mode to use to send each packet", input.txMode);
    cmd.AddValue("NPackets", "The number of packets to send", input.nPackets);
    cmd.AddValue("TargetPsr", "The psr needed to assume that we are within range", targetPsr);
    cmd.Parse(argc, argv);

    for (input.packetSize = 10; input.packetSize < 3000; input.packetSize += 40)
    {
        double precision = 0.1;
        double low = 1.0;
        double high = 200.0;
        while (high - low > precision)
        {
            double middle = low + (high - low) / 2;
            PsrExperiment::Output output;
            PsrExperiment experiment;
            input.distance = middle;
            output = experiment.Run(input);
            double psr = CalcPsr(output, input);
            if (psr >= targetPsr)
            {
                low = middle;
            }
            else
            {
                high = middle;
            }
        }
        std::cout << input.packetSize << " " << input.distance << std::endl;
    }
}

static void
PrintPsrVsCollisionInterval(int argc, char* argv[])
{
    CollisionExperiment::Input input;
    input.nPackets = 100;
    CommandLine cmd(__FILE__);
    cmd.AddValue("NPackets", "The number of packets to send for each transmitter", input.nPackets);
    cmd.AddValue("xA", "the position of transmitter A", input.xA);
    cmd.AddValue("xB", "the position of transmitter B", input.xB);
    cmd.Parse(argc, argv);

    for (uint32_t i = 0; i < 100; i += 1)
    {
        CollisionExperiment experiment;
        CollisionExperiment::Output output;
        input.interval = MicroSeconds(i);
        output = experiment.Run(input);
        double perA = (output.receivedA + 0.0) / (input.nPackets + 0.0);
        double perB = (output.receivedB + 0.0) / (input.nPackets + 0.0);
        std::cout << i << " " << perA << " " << perB << std::endl;
    }
    for (uint32_t i = 100; i < 4000; i += 50)
    {
        CollisionExperiment experiment;
        CollisionExperiment::Output output;
        input.interval = MicroSeconds(i);
        output = experiment.Run(input);
        double perA = (output.receivedA + 0.0) / (input.nPackets + 0.0);
        double perB = (output.receivedB + 0.0) / (input.nPackets + 0.0);
        std::cout << i << " " << perA << " " << perB << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        std::cout << "Available experiments: "
                  << "Psr "
                  << "SizeVsRange "
                  << "PsrVsDistance "
                  << "PsrVsCollisionInterval " << std::endl;
        return 0;
    }
    std::string type = argv[1];
    argc--;
    argv[1] = argv[0];
    argv++;
    if (type == "Psr")
    {
        PrintPsr(argc, argv);
    }
    else if (type == "SizeVsRange")
    {
        PrintSizeVsRange(argc, argv);
    }
    else if (type == "PsrVsDistance")
    {
        PrintPsrVsDistance(argc, argv);
    }
    else if (type == "PsrVsCollisionInterval")
    {
        PrintPsrVsCollisionInterval(argc, argv);
    }
    else
    {
        std::cout << "Wrong arguments!" << std::endl;
    }

    return 0;
}
