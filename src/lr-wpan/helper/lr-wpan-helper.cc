/*
 * Copyright (c) 2011 The Boeing Company
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
 *
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 */
#include "lr-wpan-helper.h"

#include "ns3/names.h"
#include <ns3/log.h>
#include <ns3/lr-wpan-csmaca.h>
#include <ns3/lr-wpan-error-model.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/mobility-model.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/single-model-spectrum-channel.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LrWpanHelper");

/**
 * @brief Output an ascii line representing the Transmit event (with context)
 * @param stream the output stream
 * @param context the context
 * @param p the packet
 */
static void
AsciiLrWpanMacTransmitSinkWithContext(Ptr<OutputStreamWrapper> stream,
                                      std::string context,
                                      Ptr<const Packet> p)
{
    *stream->GetStream() << "t " << Simulator::Now().As(Time::S) << " " << context << " " << *p
                         << std::endl;
}

/**
 * @brief Output an ascii line representing the Transmit event (without context)
 * @param stream the output stream
 * @param p the packet
 */
static void
AsciiLrWpanMacTransmitSinkWithoutContext(Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p)
{
    *stream->GetStream() << "t " << Simulator::Now().As(Time::S) << " " << *p << std::endl;
}

LrWpanHelper::LrWpanHelper()
{
    m_channel = CreateObject<SingleModelSpectrumChannel>();

    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    m_channel->AddPropagationLossModel(lossModel);

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    m_channel->SetPropagationDelayModel(delayModel);
}

LrWpanHelper::LrWpanHelper(bool useMultiModelSpectrumChannel)
{
    if (useMultiModelSpectrumChannel)
    {
        m_channel = CreateObject<MultiModelSpectrumChannel>();
    }
    else
    {
        m_channel = CreateObject<SingleModelSpectrumChannel>();
    }
    Ptr<LogDistancePropagationLossModel> lossModel =
        CreateObject<LogDistancePropagationLossModel>();
    m_channel->AddPropagationLossModel(lossModel);

    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    m_channel->SetPropagationDelayModel(delayModel);
}

LrWpanHelper::~LrWpanHelper()
{
    m_channel->Dispose();
    m_channel = nullptr;
}

void
LrWpanHelper::EnableLogComponents()
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanErrorModel", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanInterferenceHelper", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanSpectrumSignalParameters", LOG_LEVEL_ALL);
    LogComponentEnable("LrWpanSpectrumValueHelper", LOG_LEVEL_ALL);
}

std::string
LrWpanHelper::LrWpanPhyEnumerationPrinter(lrwpan::PhyEnumeration e)
{
    switch (e)
    {
    case lrwpan::IEEE_802_15_4_PHY_BUSY:
        return std::string("BUSY");
    case lrwpan::IEEE_802_15_4_PHY_BUSY_RX:
        return std::string("BUSY_RX");
    case lrwpan::IEEE_802_15_4_PHY_BUSY_TX:
        return std::string("BUSY_TX");
    case lrwpan::IEEE_802_15_4_PHY_FORCE_TRX_OFF:
        return std::string("FORCE_TRX_OFF");
    case lrwpan::IEEE_802_15_4_PHY_IDLE:
        return std::string("IDLE");
    case lrwpan::IEEE_802_15_4_PHY_INVALID_PARAMETER:
        return std::string("INVALID_PARAMETER");
    case lrwpan::IEEE_802_15_4_PHY_RX_ON:
        return std::string("RX_ON");
    case lrwpan::IEEE_802_15_4_PHY_SUCCESS:
        return std::string("SUCCESS");
    case lrwpan::IEEE_802_15_4_PHY_TRX_OFF:
        return std::string("TRX_OFF");
    case lrwpan::IEEE_802_15_4_PHY_TX_ON:
        return std::string("TX_ON");
    case lrwpan::IEEE_802_15_4_PHY_UNSUPPORTED_ATTRIBUTE:
        return std::string("UNSUPPORTED_ATTRIBUTE");
    case lrwpan::IEEE_802_15_4_PHY_READ_ONLY:
        return std::string("READ_ONLY");
    case lrwpan::IEEE_802_15_4_PHY_UNSPECIFIED:
        return std::string("UNSPECIFIED");
    default:
        return std::string("INVALID");
    }
}

std::string
LrWpanHelper::LrWpanMacStatePrinter(lrwpan::MacState e)
{
    switch (e)
    {
    case lrwpan::MAC_IDLE:
        return std::string("MAC_IDLE");
    case lrwpan::CHANNEL_ACCESS_FAILURE:
        return std::string("CHANNEL_ACCESS_FAILURE");
    case lrwpan::CHANNEL_IDLE:
        return std::string("CHANNEL_IDLE");
    case lrwpan::SET_PHY_TX_ON:
        return std::string("SET_PHY_TX_ON");
    default:
        return std::string("INVALID");
    }
}

void
LrWpanHelper::AddMobility(Ptr<lrwpan::LrWpanPhy> phy, Ptr<MobilityModel> m)
{
    phy->SetMobility(m);
}

NetDeviceContainer
LrWpanHelper::Install(NodeContainer c)
{
    NetDeviceContainer devices;
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        Ptr<Node> node = *i;

        Ptr<lrwpan::LrWpanNetDevice> netDevice = CreateObject<lrwpan::LrWpanNetDevice>();
        netDevice->SetChannel(m_channel);
        node->AddDevice(netDevice);
        netDevice->SetNode(node);
        // \todo add the capability to change short address, extended
        // address and panId. Right now they are hardcoded in LrWpanMac::LrWpanMac ()
        devices.Add(netDevice);
    }
    return devices;
}

Ptr<SpectrumChannel>
LrWpanHelper::GetChannel()
{
    return m_channel;
}

void
LrWpanHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
LrWpanHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

int64_t
LrWpanHelper::AssignStreams(NetDeviceContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<NetDevice> netDevice;
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        netDevice = (*i);
        Ptr<lrwpan::LrWpanNetDevice> lrwpan = DynamicCast<lrwpan::LrWpanNetDevice>(netDevice);
        if (lrwpan)
        {
            currentStream += lrwpan->AssignStreams(currentStream);
        }
    }
    return (currentStream - stream);
}

void
LrWpanHelper::CreateAssociatedPan(NetDeviceContainer c, uint16_t panId)
{
    NetDeviceContainer devices;
    uint16_t id = 1;
    uint8_t idBuf[2] = {0, 0};
    uint8_t idBuf2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    Mac16Address address16;
    Mac64Address address64;
    Mac16Address coordShortAddr;
    Mac64Address coordExtAddr;

    for (auto i = c.Begin(); i != c.End(); i++)
    {
        if (id < 0x0001 || id > 0xFFFD)
        {
            NS_ABORT_MSG("Only 65533 addresses supported. Range [00:01]-[FF:FD]");
        }

        Ptr<lrwpan::LrWpanNetDevice> device = DynamicCast<lrwpan::LrWpanNetDevice>(*i);
        if (device)
        {
            idBuf[0] = (id >> 8) & 0xff;
            idBuf[1] = (id >> 0) & 0xff;
            address16.CopyFrom(idBuf);

            idBuf2[6] = (id >> 8) & 0xff;
            idBuf2[7] = (id >> 0) & 0xff;
            address64.CopyFrom(idBuf2);

            if (address64 == Mac64Address("00:00:00:00:00:00:00:01"))
            {
                // We use the first device in the container as coordinator
                coordShortAddr = address16;
                coordExtAddr = address64;
            }

            // TODO: Change this to device->GetAddress() if GetAddress can guarantee a
            //  an extended address (currently only gives 48 address or 16 bits addresses)
            device->GetMac()->SetExtendedAddress(address64);
            device->SetPanAssociation(panId, coordExtAddr, coordShortAddr, address16);

            id++;
        }
    }
}

void
LrWpanHelper::SetExtendedAddresses(NetDeviceContainer c)
{
    NetDeviceContainer devices;
    uint64_t id = 1;
    uint8_t idBuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    Mac64Address address64;

    for (auto i = c.Begin(); i != c.End(); i++)
    {
        Ptr<lrwpan::LrWpanNetDevice> device = DynamicCast<lrwpan::LrWpanNetDevice>(*i);
        if (device)
        {
            idBuf[0] = (id >> 56) & 0xff;
            idBuf[1] = (id >> 48) & 0xff;
            idBuf[2] = (id >> 40) & 0xff;
            idBuf[3] = (id >> 32) & 0xff;
            idBuf[4] = (id >> 24) & 0xff;
            idBuf[5] = (id >> 16) & 0xff;
            idBuf[6] = (id >> 8) & 0xff;
            idBuf[7] = (id >> 0) & 0xff;

            address64.CopyFrom(idBuf);

            // TODO: Change this to device->SetAddress() if GetAddress can guarantee
            //  to set only extended addresses
            device->GetMac()->SetExtendedAddress(address64);

            id++;
        }
    }
}

/**
 * @brief Write a packet in a PCAP file
 * @param file the output file
 * @param packet the packet
 */
static void
PcapSniffLrWpan(Ptr<PcapFileWrapper> file, Ptr<const Packet> packet)
{
    file->Write(Simulator::Now(), packet);
}

void
LrWpanHelper::EnablePcapInternal(std::string prefix,
                                 Ptr<NetDevice> nd,
                                 bool promiscuous,
                                 bool explicitFilename)
{
    NS_LOG_FUNCTION(this << prefix << nd << promiscuous << explicitFilename);
    //
    // All of the Pcap enable functions vector through here including the ones
    // that are wandering through all of devices on perhaps all of the nodes in
    // the system.
    //

    // In the future, if we create different NetDevice types, we will
    // have to switch on each type below and insert into the right
    // NetDevice type
    //
    Ptr<lrwpan::LrWpanNetDevice> device = nd->GetObject<lrwpan::LrWpanNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("LrWpanHelper::EnablePcapInternal(): Device "
                    << device << " not of type ns3::LrWpanNetDevice");
        return;
    }

    PcapHelper pcapHelper;

    std::string filename;
    if (explicitFilename)
    {
        filename = prefix;
    }
    else
    {
        filename = pcapHelper.GetFilenameFromDevice(prefix, device);
    }

    Ptr<PcapFileWrapper> file =
        pcapHelper.CreateFile(filename, std::ios::out, PcapHelper::DLT_IEEE802_15_4);

    if (promiscuous)
    {
        device->GetMac()->TraceConnectWithoutContext("PromiscSniffer",
                                                     MakeBoundCallback(&PcapSniffLrWpan, file));
    }
    else
    {
        device->GetMac()->TraceConnectWithoutContext("Sniffer",
                                                     MakeBoundCallback(&PcapSniffLrWpan, file));
    }
}

void
LrWpanHelper::EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                                  std::string prefix,
                                  Ptr<NetDevice> nd,
                                  bool explicitFilename)
{
    uint32_t nodeid = nd->GetNode()->GetId();
    uint32_t deviceid = nd->GetIfIndex();
    std::ostringstream oss;

    Ptr<lrwpan::LrWpanNetDevice> device = nd->GetObject<lrwpan::LrWpanNetDevice>();
    if (!device)
    {
        NS_LOG_INFO("LrWpanHelper::EnableAsciiInternal(): Device "
                    << device << " not of type ns3::LrWpanNetDevice");
        return;
    }

    //
    // Our default trace sinks are going to use packet printing, so we have to
    // make sure that is turned on.
    //
    Packet::EnablePrinting();

    //
    // If we are not provided an OutputStreamWrapper, we are expected to create
    // one using the usual trace filename conventions and do a Hook*WithoutContext
    // since there will be one file per context and therefore the context would
    // be redundant.
    //
    if (!stream)
    {
        //
        // Set up an output stream object to deal with private ofstream copy
        // constructor and lifetime issues.  Let the helper decide the actual
        // name of the file given the prefix.
        //
        AsciiTraceHelper asciiTraceHelper;

        std::string filename;
        if (explicitFilename)
        {
            filename = prefix;
        }
        else
        {
            filename = asciiTraceHelper.GetFilenameFromDevice(prefix, device);
        }

        Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream(filename);

        // Ascii traces typically have "+", '-", "d", "r", and sometimes "t"
        // The Mac and Phy objects have the trace sources for these
        //

        asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<lrwpan::LrWpanMac>(device->GetMac(),
                                                                                 "MacRx",
                                                                                 theStream);

        device->GetMac()->TraceConnectWithoutContext(
            "MacTx",
            MakeBoundCallback(&AsciiLrWpanMacTransmitSinkWithoutContext, theStream));

        asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<lrwpan::LrWpanMac>(device->GetMac(),
                                                                                 "MacTxEnqueue",
                                                                                 theStream);
        asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<lrwpan::LrWpanMac>(device->GetMac(),
                                                                                 "MacTxDequeue",
                                                                                 theStream);
        asciiTraceHelper.HookDefaultDropSinkWithoutContext<lrwpan::LrWpanMac>(device->GetMac(),
                                                                              "MacTxDrop",
                                                                              theStream);

        return;
    }

    //
    // If we are provided an OutputStreamWrapper, we are expected to use it, and
    // to provide a context.  We are free to come up with our own context if we
    // want, and use the AsciiTraceHelper Hook*WithContext functions, but for
    // compatibility and simplicity, we just use Config::Connect and let it deal
    // with the context.
    //
    // Note that we are going to use the default trace sinks provided by the
    // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
    // but the default trace sinks are actually publicly available static
    // functions that are always there waiting for just such a case.
    //

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::LrWpanNetDevice/Mac/MacRx";
    device->GetMac()->TraceConnect(
        "MacRx",
        oss.str(),
        MakeBoundCallback(&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::LrWpanNetDevice/Mac/MacTx";
    device->GetMac()->TraceConnect(
        "MacTx",
        oss.str(),
        MakeBoundCallback(&AsciiLrWpanMacTransmitSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::LrWpanNetDevice/Mac/MacTxEnqueue";
    device->GetMac()->TraceConnect(
        "MacTxEnqueue",
        oss.str(),
        MakeBoundCallback(&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::LrWpanNetDevice/Mac/MacTxDequeue";
    device->GetMac()->TraceConnect(
        "MacTxDequeue",
        oss.str(),
        MakeBoundCallback(&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));

    oss.str("");
    oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid
        << "/$ns3::LrWpanNetDevice/Mac/MacTxDrop";
    device->GetMac()->TraceConnect(
        "MacTxDrop",
        oss.str(),
        MakeBoundCallback(&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
}

} // namespace ns3
